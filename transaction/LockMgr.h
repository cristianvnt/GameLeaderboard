#ifndef LOCK_MGR_H
#define LOCK_MGR_H

#include "Common.h"

#include <map>
#include <set>
#include <mutex>
#include <condition_variable>

struct LockInfo
{
    std::set<TransactionId> sharedHolders;
    TransactionId exclusiveHolder;
    bool isExclusive;
    std::set<TransactionId> waitQ;

    LockInfo() : exclusiveHolder{ 0 }, isExclusive{ false } { }
};

class LockMgr
{
private:
    std::map<ResourceId, LockInfo> _lockTable;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::map<TransactionId, std::set<ResourceId>> _txLocks;

public:
    LockMgr() = default;
    ~LockMgr() = default;

    bool acquireLock(TransactionId txId, ResourceId resId, LockType type);
    void releaseLocks(TransactionId txId);
    std::map<TransactionId, std::set<TransactionId>> getWaitForGraph();
};

#endif