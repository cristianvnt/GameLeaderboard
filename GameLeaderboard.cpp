#include "LockMgr.h"
#include "DeadlockDetector.h"
#include "TransactionMgr.h"

#include <thread>
#include <chrono>
#include <iostream>

#pragma region TestBasicLocking
void RunTx(LockMgr& lockMgr, TransactionId txId, ResourceId resId, int sleepMs)
{
    lockMgr.AcquireLock(txId, resId, LockType::Exclusive);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));

    lockMgr.ReleaseLocks(txId);
}

void TestBasicLocking()
{
    LockMgr lockMgr;

    std::thread t1(RunTx, std::ref(lockMgr), 1, "player:alice", 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::thread t2(RunTx, std::ref(lockMgr), 2, "player:alice", 1000);

    t1.join();
    t2.join();

    std::cout << "\nTest complet - TX2 shouldve waited for TX1\n";
}
#pragma endregion

#pragma region TestDeadlockDetection
void Tx1Deadlock(LockMgr& lockMgr)
{
    lockMgr.AcquireLock(1, "alice", LockType::Exclusive);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    lockMgr.AcquireLock(1, "bob", LockType::Exclusive);
    lockMgr.ReleaseLocks(1);
}

void Tx2Deadlock(LockMgr& lockMgr)
{
    lockMgr.AcquireLock(2, "bob", LockType::Exclusive);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    lockMgr.AcquireLock(2, "alice", LockType::Exclusive);
    lockMgr.ReleaseLocks(2);
}

void TestDeadlockDetection()
{
    LockMgr mgr;
    DeadlockDetector detector;

    std::thread t1(Tx1Deadlock, std::ref(mgr));
    std::thread t2(Tx2Deadlock, std::ref(mgr));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    auto graph = mgr.GetWaitForGraph();
    auto cycle = detector.DetectDeadlock(graph);

    if (!cycle.empty())
    {
        std::cout << "Detected cycle: ";
        for (auto txId : cycle)
            std::cout << txId << " ";
        std::cout << "\nVictim: " << detector.ChooseVictim(cycle) << "\n";
    }
    std::cout << "Test complete\n";
}
#pragma endregion

#pragma region TestUndoLog
void TestUndoLog()
{
    UndoLog log;
    
    // Simulate operations
    log.Record(UndoOperation{ DatabaseType::Postgres, "UPDATE players SET score = 100 WHERE id = 1" });
    log.Record(UndoOperation{ DatabaseType::Postgres, "DELETE FROM matches WHERE id = 5"});
    log.Record(UndoOperation{ DatabaseType::Cassandra, "UPDATE stats SET games = 10" });
    
    std::cout << "Recorded 3 undo operations\n";
    std::cout << "IsEmpty: " << (log.IsEmpty() ? "yes" : "no") << "\n";
    
    auto undoOps = log.GetUndoOperations();
    std::cout << "\nUndo operations (reversed):\n";
    for (size_t i = 0; i < undoOps.size(); ++i)
    {
        std::cout << i + 1 << ". " << undoOps[i].undoSqlStr << "\n";
    }
    
    log.Clear();
    std::cout << "\nAfter clear, IsEmpty: " << (log.IsEmpty() ? "yes" : "no") << "\n";
}
#pragma endregion

#pragma region TestTransactionMgr
void TestTransactionMgr()
{
    TransactionMgr txMgr;
    Transaction tx(1);

    tx.AddOperation(SqlOperation
    {
        DatabaseType::Postgres,
        "UPDATE players SET score = 150 WHERE id = 5",
        {"player:5"},
        LockType::Exclusive
    });

    tx.AddOperation(SqlOperation
    {
        DatabaseType::Postgres,
        "INSERT INTO matches VALUES (123, 5, 150)",
        {"match:123"},
        LockType::Exclusive
    });

    tx.AddOperation(SqlOperation
    {
        DatabaseType::Cassandra,
        "UPDATE stats SET games_played = games_played + 1 WHERE player_id = 5",
        {"stats:5"},
        LockType::Exclusive
    });

    bool success = txMgr.ExecuteTransaction(tx);

    std::cout << "\nTransaction " << (success ? "YAY" : "NOOOOOOOOO") << "\n";
}
#pragma endregion

#pragma region TestConcurrency
void ConcurrentTx(TransactionMgr& mgr, TransactionId txId, int sleepMs)
{
    Transaction tx(txId);
    tx.AddOperation(SqlOperation
    {
        DatabaseType::Postgres,
        "UPDATE players SET score = " + std::to_string(txId * 100),
        {"player:alice"},
        LockType::Exclusive
    });
    
    std::cout << "TX" << txId << " starting\n";
    mgr.ExecuteTransaction(tx);
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
}

void TestConcurrency()
{
    TransactionMgr mgr;
    
    std::thread t1(ConcurrentTx, std::ref(mgr), 1, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::thread t2(ConcurrentTx, std::ref(mgr), 2, 1000);
    
    t1.join();
    t2.join();
}
#pragma endregion

int main()
{
    //TestBasicLocking();
    //TestDeadlockDetection();
    //TestUndoLog();
    //TestTransactionMgr();
    //TestConcurrency();
}