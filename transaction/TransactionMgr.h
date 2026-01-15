#ifndef TRANSACTION_MGR_H
#define TRANSACTION_MGR_H

#include "Common.h"
#include "Transaction.h"
#include "LockMgr.h"
#include "DeadlockDetector.h"

#include <memory>
#include <map>

class DatabaseConnector;

class TransactionMgr
{
private:
    LockMgr _lockMgr;
    DeadlockDetector _deadlockDetector;

    std::map<std::string, std::string> _mockDatabase;

    bool CheckForDeadlock(TransactionId txId);
    void ExecuteOperation(SqlOperation const& op);
    void Rollback(Transaction& tx);

public:
    TransactionMgr() = default;
    ~TransactionMgr() = default;

    bool ExecuteTransaction(Transaction& tx);
};

#endif