#ifndef UNDO_LOG_H
#define UNDO_LOG_H

#include "Common.h"

#include <vector>

class UndoLog
{
private:
    std::vector<UndoOperation> _operations;

public:
    UndoLog() = default;
    ~UndoLog() = default;

    UndoLog(UndoLog const&) = delete;
    UndoLog& operator=(UndoLog const&) = delete;

    void Record(UndoOperation const& op);
    std::vector<UndoOperation> GetUndoOperations() const;
    void Clear();
    bool IsEmpty() const;
};

#endif