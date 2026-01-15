#include "LockMgr.h"
#include "DeadlockDetector.h"

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

int main()
{
    //TestBasicLocking();
    TestDeadlockDetection();
}