#ifndef DEADLOCK_DETECTOR_H
#define DEADLOCK_DETECTOR_H

#include "Common.h"

#include <map>
#include <set>
#include <vector>

class DeadlockDetector
{
private:
    bool HasCycleDFS(TransactionId node, std::map<TransactionId, std::set<TransactionId>> const& graph,
        std::set<TransactionId>& visited, std::set<TransactionId>& recStack, std::vector<TransactionId>& cycle);

public:
    DeadlockDetector() = default;
    ~DeadlockDetector() = default;

    std::vector<TransactionId> DetectDeadlock(std::map<TransactionId, std::set<TransactionId>> const& waitForGraph);
    TransactionId ChooseVictim(std::vector<TransactionId> const& cycle);
};

#endif