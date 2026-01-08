#include "LockMgr.h"

#include <iostream>

bool LockMgr::acquireLock(TransactionId txId, ResourceId resId, LockType type)
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
            lockInfo.waitQ.insert(txId);
            _cv.wait(lock);
            lockInfo.waitQ.erase(txId);
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
        lockInfo.waitQ.insert(txId);
        _cv.wait(lock);
        lockInfo.waitQ.erase(txId);
    }

    lockInfo.sharedHolders.insert(txId);
    _txLocks[txId].insert(resId);

    std::cout << "TX" << txId << " acquired shared lock on " << resId << "\n";
    return true;
}

void LockMgr::releaseLocks(TransactionId txId)
{

}
