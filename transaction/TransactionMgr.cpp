#include "Transaction.h"
#include "TransactionMgr.h"

#include <iostream>
#include <thread>
#include <chrono>

TransactionMgr::TransactionMgr()
    : _mainDb("leaderboard_main"), _statsDb("leaderboard_stats")
{
}

bool TransactionMgr::CheckForDeadlock(TransactionId txId)
{
    auto graph = _lockMgr.GetWaitForGraph();
    auto cycle = _deadlockDetector.DetectDeadlock(graph);

    if (!cycle.empty())
    {
        for (auto id : cycle)
        {
            if (id == txId)
            {
                TransactionId victim = _deadlockDetector.ChooseVictim(cycle);
                if (victim == txId)
                    return true;
            }
        }
    }

    return false;
}

void TransactionMgr::ExecuteOperation(SqlOperation const &op)
{
    try
    {
        if (op.db == DatabaseType::Postgres)
            _mainDb.Execute(op.sqlStr);
        else
            _statsDb.Execute(op.sqlStr);
    }
    catch (std::exception const& e)
    {
        std::cout << "Operation failed: " << e.what() << "\n";
        throw;
    }
}

void TransactionMgr::Rollback(Transaction &tx)
{
    std::cout << "TX" << tx.GetId() << " rolling back...\n";
    auto undoOps = tx.GetUndoLog().GetUndoOperations();

    for (auto const& op : undoOps)
    {
        if (op.db == DatabaseType::Postgres)
            _mainDb.Execute(op.undoSqlStr);
        else
            _statsDb.Execute(op.undoSqlStr);
    }

    tx.SetState(TransactionState::Aborted);
    tx.GetUndoLog().Clear();
    _lockMgr.ReleaseLocks(tx.GetId());

    std::cout << "TX" << tx.GetId() << " rolled back\n";
}

std::string TransactionMgr::BuildUndoOperation(SqlOperation const &op)
{
    std::string sql = op.sqlStr;
    
    if (sql.find("UPDATE players SET score") != std::string::npos)
    {
        size_t wherePos = sql.find("WHERE username = '");
        if (wherePos != std::string::npos)
        {
            size_t start = wherePos + 18;
            size_t end = sql.find("'", start);
            std::string username = sql.substr(start, end - start);
            
            auto rows = _mainDb.Query("SELECT score FROM players WHERE username = '" + username + "'");
            if (!rows.empty())
            {
                std::string oldScore = rows[0].at("score");
                return "UPDATE players SET score = " + oldScore + " WHERE username = '" + username + "'";
            }
        }
        
        wherePos = sql.find("WHERE id = ");
        if (wherePos != std::string::npos)
        {
            size_t start = wherePos + 11;
            std::string playerId;
            for (size_t i = start; i < sql.length() && isdigit(sql[i]); ++i)
                playerId += sql[i];
            
            auto rows = _mainDb.Query("SELECT score FROM players WHERE id = " + playerId);
            if (!rows.empty())
            {
                std::string oldScore = rows[0].at("score");
                return "UPDATE players SET score = " + oldScore + " WHERE id = " + playerId;
            }
        }
    }
    
    if (sql.find("UPDATE player_stats") != std::string::npos)
    {
        size_t wherePos = sql.find("WHERE player_id = ");
        if (wherePos != std::string::npos)
        {
            size_t start = wherePos + 18;
            std::string playerId;
            for (size_t i = start; i < sql.length() && isdigit(sql[i]); ++i)
                playerId += sql[i];
            
            auto rows = _statsDb.Query("SELECT games_played, total_score FROM player_stats WHERE player_id = " + playerId);
            if (!rows.empty())
            {
                if (sql.find("games_played") != std::string::npos)
                {
                    std::string oldGames = rows[0].at("games_played");
                    return "UPDATE player_stats SET games_played = " + oldGames + " WHERE player_id = " + playerId;
                }
                if (sql.find("total_score") != std::string::npos)
                {
                    std::string oldScore = rows[0].at("total_score");
                    return "UPDATE player_stats SET total_score = " + oldScore + " WHERE player_id = " + playerId;
                }
            }
        }
    }
    
    if (sql.find("INSERT INTO matches") != std::string::npos)
        return "-- Cant INSERT for now";
    
    return "-- Undo for: " + sql;
}

bool TransactionMgr::ExecuteTransaction(Transaction &tx)
{
    std::cout << "TX" << tx.GetId() << " acquiring locks...\n";

    auto resources = tx.GetAllResources();
    for (auto const& res : resources)
    {
        LockType lockType = LockType::Exclusive;

        for (auto const& op : tx.GetOperations())
        {
            for (auto const& opRes : op.resources)
                if (opRes == res)
                {
                    lockType = op.lockType;
                    break;
                }
        }

        _lockMgr.AcquireLock(tx.GetId(), res, lockType);

        if (CheckForDeadlock(tx.GetId()))
        {
            std::cout << "TX" << tx.GetId() << " deadlock detected, aborting\n";
            Rollback(tx);
            return false;
        }
    }
    std::cout << "TX" << tx.GetId() << " acquired all locks\n";
    //std::cout << "TX" << tx.GetId() << " holding locks for 2 seconds...\n";
    //std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "TX" << tx.GetId() << " executing operations...\n";

    try
    {
        for (auto const& op : tx.GetOperations())
        {
            std::string undoSql = BuildUndoOperation(op);
            tx.GetUndoLog().Record(UndoOperation{ op.db, undoSql});
            ExecuteOperation(op);
        }
    }
    catch (...)
    {
        std::cout << "TX" << tx.GetId() << " operation failed, rolling back...\n";
        Rollback(tx);
        return false;
    }

    std::cout << "TX" << tx.GetId() << " committing...\n";

    tx.SetState(TransactionState::Committed);
    tx.GetUndoLog().Clear();

    _lockMgr.ReleaseLocks(tx.GetId());

    std::cout << "TX" << tx.GetId() << " committed successfully\n";
    return true;
}
