#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Common.h"
#include "UndoLog.h"

#include <vector>

class Transaction
{
private:
    TransactionId _id;
    TransactionState _state;
    std::vector<SqlOperation> _operations;
    UndoLog _undoLog;

public:
    explicit Transaction(TransactionId id);
    ~Transaction() = default;

    Transaction(Transaction const&) = delete;
    Transaction& operator=(Transaction const&) = delete;

    void AddOperation(SqlOperation const& op);

    TransactionId GetId() const { return _id; }
    TransactionState GetState() const { return _state; }
    void SetState(TransactionState state);

    std::vector<SqlOperation> const& GetOperations() const { return _operations; }
    UndoLog& GetUndoLog() { return _undoLog; }

    std::vector<ResourceId> GetAllResources() const;
};

#endif