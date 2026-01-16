#include "SocketServer.h"
#include "RequestParser.h"
#include "TransactionMgr.h"
#include "PostgresConnector.h"

#include <iostream>
#include <atomic>
#include <thread>

std::atomic<TransactionId> nextTxId{ 1 };
std::string HandleRequest(TransactionMgr&, PostgresConnector&, std::string const&);

int main()
{
    std::cout << "TRANSACTION SERVER STARTIN...\n";
    TransactionMgr txMgr;
    PostgresConnector mainDb("leaderboard_main");

    SocketServer server(8080);

    server.SetRequestHandler([&](std::string const& req)
    {
        return HandleRequest(txMgr, mainDb, req);
    });

    std::cout << "SERVER READYY\n";
    server.Start();
}

std::string HandleRequest(TransactionMgr& txMgr, PostgresConnector& mainDb, std::string const& message)
{
    Request req = RequestParser::Parse(message);
    
    std::cout << "CMD: " << req.command << "\n";
    
    if (req.command == "SUBMIT_SCORE")
    {
        std::string player = req.params["player"];
        int score = std::stoi(req.params["score"]);

        auto rows = mainDb.Query("SELECT id FROM players WHERE username = '" + player + "'");
        if (rows.empty())
            return RequestParser::BuildResponse(false, "PLayer not found");

        std::string playerId = rows[0].at("id");
        
        Transaction tx(nextTxId++);
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Postgres,
            "UPDATE players SET score = score + " + std::to_string(score) + " WHERE username = '" + player + "'",
            {"player:" + player},
            LockType::Exclusive
        });
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Cassandra,
            "UPDATE player_stats SET games_played = games_played + 1 WHERE player_id = " + playerId,
            {"stats:" + player},
            LockType::Exclusive
        });
        
        bool success = txMgr.ExecuteTransaction(tx);
        
        return RequestParser::BuildResponse(success, success ? "Score submitted" : "Failed");
    }
    else if (req.command == "GET_PLAYER")
    {
        std::string player = req.params["player"];
        
        auto rows = mainDb.Query("SELECT * FROM players WHERE username = '" + player + "'");
        
        if (rows.empty())
            return RequestParser::BuildResponse(false, "Player not found");
        
        std::string response = "Player: " + rows[0].at("username") + ", Score: " + rows[0].at("score");
        
        return RequestParser::BuildResponse(true, response);
    }
    else if (req.command == "TRANSFER_POINTS")
    {
        std::string from = req.params["from"];
        std::string to = req.params["to"];
        int amount = std::stoi(req.params["amount"]);
        
        Transaction tx(nextTxId++);
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Postgres,
            "UPDATE players SET score = score - " + std::to_string(amount) + " WHERE username = '" + from + "'",
            {"player:" + from},
            LockType::Exclusive
        });
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Postgres,
            "UPDATE players SET score = score + " + std::to_string(amount) + " WHERE username = '" + to + "'",
            {"player:" + to},
            LockType::Exclusive
        });
        
        bool success = txMgr.ExecuteTransaction(tx);
        
        return RequestParser::BuildResponse(success, success ? "Points transferred" : "Failed");
    }
    else if (req.command == "GET_LEADERBOARD")
    {
        int limit = 10;
        if (req.params.count("limit"))
            limit = std::stoi(req.params["limit"]);
        
        auto rows = mainDb.Query("SELECT username, score FROM players ORDER BY score DESC LIMIT " + std::to_string(limit));
        
        std::string result;
        for (auto const& row : rows)
            result += row.at("username") + ":" + row.at("score") + ";";
        
        return RequestParser::BuildResponse(true, result);
    }
    else if (req.command == "GET_STATS")
    {
        std::string player = req.params["player"];
        
        auto playerRows = mainDb.Query("SELECT id FROM players WHERE username = '" + player + "'");
        if (playerRows.empty())
            return RequestParser::BuildResponse(false, "Player not found");
        
        std::string playerId = playerRows[0].at("id");
        
        PostgresConnector statsDb("leaderboard_stats");
        auto statsRows = statsDb.Query("SELECT * FROM player_stats WHERE player_id = " + playerId);
        
        if (statsRows.empty())
            return RequestParser::BuildResponse(false, "Stats not found");
        
        std::string result = "Games: " + statsRows[0].at("games_played") + ", Total: " + statsRows[0].at("total_score");
        
        return RequestParser::BuildResponse(true, result);
    }
    else if (req.command == "UPDATE_TOTAL_SCORE")
    {
        std::string player = req.params["player"];
        
        auto playerRows = mainDb.Query("SELECT id, score FROM players WHERE username = '" + player + "'");
        if (playerRows.empty())
            return RequestParser::BuildResponse(false, "Player not found");
        
        std::string playerId = playerRows[0].at("id");
        std::string currentScore = playerRows[0].at("score");
        
        Transaction tx(nextTxId++);
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Cassandra,
            "UPDATE player_stats SET total_score = " + currentScore + " WHERE player_id = " + playerId,
            {"stats:" + player},
            LockType::Exclusive
        });
        
        bool success = txMgr.ExecuteTransaction(tx);
        
        return RequestParser::BuildResponse(success, success ? "Total score updated" : "Failed");
    }
    else if (req.command == "TEST_ROLLBACK")
    {
        std::string player = req.params["player"];
        
        auto rows = mainDb.Query("SELECT id, score FROM players WHERE username = '" + player + "'");
        if (rows.empty())
            return RequestParser::BuildResponse(false, "Player not found");
        
        std::string playerId = rows[0].at("id");
        
        Transaction tx(nextTxId++);
        
        tx.AddOperation(SqlOperation
        {
            DatabaseType::Postgres,
            "UPDATE players SET score = score + 1000 WHERE username = '" + player + "'",
            {"player:" + player},
            LockType::Exclusive
        });
        
        tx.AddOperation(SqlOperation{
            DatabaseType::Postgres,
            "UPDATE nonexistent_table SET x = 1",
            {"fake"},
            LockType::Exclusive
        });
        
        bool success = txMgr.ExecuteTransaction(tx);
        
        return RequestParser::BuildResponse(success, success ? "Should not see this" : "ROLLED BACK");
    }
    
    return RequestParser::BuildResponse(false, "Unk command");
}