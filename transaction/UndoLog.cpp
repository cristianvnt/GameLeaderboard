#include "UndoLog.h"

#include <algorithm>

void UndoLog::Record(UndoOperation const &op)
{
    _operations.push_back(op);
}

std::vector<UndoOperation> UndoLog::GetUndoOperations() const
{
    std::vector<UndoOperation> reversed{ _operations };
    std::reverse(reversed.begin(), reversed.end());
    return reversed;
}

void UndoLog::Clear()
{
    _operations.clear();
}

bool UndoLog::IsEmpty() const
{
    return _operations.empty();
}
