#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>
#include <cstdint>

using TransactionId = uint64_t;
using ResourceId = std::string;

enum class LockType
{
    Shared,
    Exclusive
};

enum class TransactionState
{
    Active,
    Preparing,
    Committed,
    Aborted
};

enum class DatabaseType
{
    Postgres,
    Cassandra
};

struct SqlOperation
{
    DatabaseType db;
    std::string sqlStr;
    std::vector<ResourceId> resources;
    LockType lockType;

    SqlOperation(DatabaseType db, std::string const& sqlStr, std::vector<ResourceId> const& res, LockType lock)
        : db{ db }, sqlStr{ sqlStr }, resources{ res }, lockType{ lock } { }
};

struct UndoOperation
{
    DatabaseType db;
    std::string undoSqlStr;

    UndoOperation(DatabaseType db, std::string const& undoStr) : db{ db }, undoSqlStr{ undoStr } { }
};

#endif