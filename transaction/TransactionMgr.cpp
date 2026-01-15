#include "Transaction.h"
#include "TransactionMgr.h"

#include <iostream>
#include <thread>
#include <chrono>

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
    // mock
    std::cout << "exec on " << (op.db == DatabaseType::Postgres ? "Postgres" : "Cassandra") << ": " << op.sqlStr << "\n";
}

void TransactionMgr::Rollback(Transaction &tx)
{
    std::cout << "TX" << tx.GetId() << " rolling back...\n";
    auto undoOps = tx.GetUndoLog().GetUndoOperations();

    for (auto const& op : undoOps)
    {
        std::cout << "UNDOING on " << (op.db == DatabaseType::Postgres ? "Postgres" : "Cassandra") << ": " << op.undoSqlStr << "\n";
    }

    tx.SetState(TransactionState::Aborted);
    tx.GetUndoLog().Clear();

    _lockMgr.ReleaseLocks(tx.GetId());

    std::cout << "TX" << tx.GetId() << " rolled back\n";
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

    std::cout << "TX" << tx.GetId() << " executing operations...\n";
    for (auto const& op : tx.GetOperations())
    {
        tx.GetUndoLog().Record(UndoOperation{ op.db, "UNDO: " + op.sqlStr });
        ExecuteOperation(op);
    }

    std::cout << "TX" << tx.GetId() << " committing...\n";

    tx.SetState(TransactionState::Committed);
    tx.GetUndoLog().Clear();

    _lockMgr.ReleaseLocks(tx.GetId());

    std::cout << "TX" << tx.GetId() << " committed successfully\n";
    return true;
}
