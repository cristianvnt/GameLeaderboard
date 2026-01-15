#include "DeadlockDetector.h"

#include <algorithm>

bool DeadlockDetector::HasCycleDFS(TransactionId node, std::map<TransactionId, std::set<TransactionId>> const &graph,
    std::set<TransactionId> &visited, std::set<TransactionId> &recStack, std::vector<TransactionId> &cycle)
{
    visited.insert(node);
    recStack.insert(node);
    cycle.push_back(node);

    if (auto it = graph.find(node); it != graph.end())
    {
        for (TransactionId neighbor : it->second)
        {
            if (visited.find(neighbor) == visited.end())
            {
                if (HasCycleDFS(neighbor, graph, visited, recStack, cycle))
                    return true;
            }
            else if (recStack.find(neighbor) != recStack.end())
            {
                cycle.push_back(neighbor);
                return true;
            }
        }
    }

    recStack.erase(node);
    cycle.pop_back();

    return false;
}

std::vector<TransactionId> DeadlockDetector::DetectDeadlock(std::map<TransactionId, std::set<TransactionId>> const &waitForGraph)
{
    std::set<TransactionId> visited;
    std::set<TransactionId> recStack;
    std::vector<TransactionId> cycle;

    for (auto const& [txId, waitinFor] : waitForGraph)
    {
        if (visited.find(txId) == visited.end())
            if (HasCycleDFS(txId, waitForGraph, visited, recStack, cycle))
                return cycle;
    }

    return {};
}

TransactionId DeadlockDetector::ChooseVictim(std::vector<TransactionId> const &cycle)
{
    return cycle.empty() ? 0 : *std::max_element(cycle.begin(), cycle.end());
}
