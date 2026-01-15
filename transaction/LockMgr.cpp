#include "LockMgr.h"

#include <iostream>

bool LockMgr::AcquireLock(TransactionId txId, ResourceId resId, LockType type)
{
    std::unique_lock<std::mutex> lock(_mutex);
    LockInfo& lockInfo = _lockTable[resId];

    if (type == LockType::Exclusive)
    {
        while ((lockInfo.isExclusive && lockInfo.exclusiveHolder != txId)
            || (!lockInfo.sharedHolders.empty()
            && !(lockInfo.sharedHolders.size() == 1 && lockInfo.sharedHolders.count(txId))))
        {
            std::cout << "TX" << txId << " waitin for exclusive lock on " << resId << "\n";
            lockInfo.waitinQ.insert(txId);
            _cv.wait(lock);
            lockInfo.waitinQ.erase(txId);
        }

        lockInfo.isExclusive = true;
        lockInfo.exclusiveHolder = txId;
        lockInfo.sharedHolders.clear();
        _txLocks[txId].insert(resId);

        std::cout << "TX" << txId << " acquired exclusive lock on " << resId << "\n";
        return true;
    }
    
    while (lockInfo.isExclusive && lockInfo.exclusiveHolder != txId)
    {
        std::cout << "TX" << txId << " waitin for shared lock on " << resId << "\n";
        lockInfo.waitinQ.insert(txId);
        _cv.wait(lock);
        lockInfo.waitinQ.erase(txId);
    }

    lockInfo.sharedHolders.insert(txId);
    _txLocks[txId].insert(resId);

    std::cout << "TX" << txId << " acquired shared lock on " << resId << "\n";
    return true;
}

void LockMgr::ReleaseLocks(TransactionId txId)
{
    std::unique_lock<std::mutex> lock(_mutex);

    auto it = _txLocks.find(txId);
    if (it == _txLocks.end())
        return;

    for (ResourceId const& resId : it->second)
    {
        auto lockIt = _lockTable.find(resId);
        if (lockIt == _lockTable.end())
            continue;

        LockInfo& lockInfo = lockIt->second;
        lockInfo.sharedHolders.erase(txId);

        if (lockInfo.exclusiveHolder != txId)
            continue;
            
        lockInfo.isExclusive = false;
        lockInfo.exclusiveHolder = 0;
    }

    _txLocks.erase(txId);
    _cv.notify_all();
    
    std::cout << "TX" << txId << " released all locks\n"; 
}

std::map<TransactionId, std::set<TransactionId>> LockMgr::GetWaitForGraph()
{
    std::unique_lock<std::mutex> lock(_mutex);
    std::map<TransactionId, std::set<TransactionId>> waitForGraph;

    for (auto const& [resId, lockInfo] : _lockTable)
    {
        for (TransactionId waiter : lockInfo.waitinQ)
        {
            if (lockInfo.isExclusive)
                waitForGraph[waiter].insert(lockInfo.exclusiveHolder);
            else
            {
                for(TransactionId holder : lockInfo.sharedHolders)
                if (holder != waiter)
                    waitForGraph[waiter].insert(holder);
            }
        }
    }

    return waitForGraph;
}
