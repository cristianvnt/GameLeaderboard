#include "Transaction.h"

Transaction::Transaction(TransactionId id)
    : _id{ id }, _state{ TransactionState::Active }
{
}

void Transaction::AddOperation(SqlOperation const &op)
{
    _operations.push_back(op);
}

void Transaction::SetState(TransactionState state)
{
    _state = state;
}

std::vector<ResourceId> Transaction::GetAllResources() const
{
    std::vector<ResourceId> allRes;

    for (auto const& op : _operations)
        for (auto const& res : op.resources)
        {
            bool found = false;
            for (auto const& existing : allRes)
                if (existing == res)
                {
                    found = true;
                    break;
                }

            if (!found)
                allRes.push_back(res);
        }

    return allRes;
}
