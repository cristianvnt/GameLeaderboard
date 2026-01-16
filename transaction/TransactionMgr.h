#ifndef TRANSACTION_MGR_H
#define TRANSACTION_MGR_H

#include "Common.h"
#include "Transaction.h"
#include "LockMgr.h"
#include "DeadlockDetector.h"
#include "PostgresConnector.h"

#include <memory>
#include <map>

class DatabaseConnector;

class TransactionMgr
{
private:
    LockMgr _lockMgr;
    DeadlockDetector _deadlockDetector;

    PostgresConnector _mainDb;
    PostgresConnector _statsDb;

    bool CheckForDeadlock(TransactionId txId);
    void ExecuteOperation(SqlOperation const& op);
    void Rollback(Transaction& tx);
    std::string BuildUndoOperation(SqlOperation const& op); 

public:
    TransactionMgr();
    ~TransactionMgr() = default;

    bool ExecuteTransaction(Transaction& tx);
};

#endif