#ifndef POSTGRES_CONNECTOR_H
#define POSTGRES_CONNECTOR_H

#include "Common.h"

#include <string>
#include <vector>
#include <map>
#include <pqxx/pqxx>

class PostgresConnector
{
private:
    std::string _connString;

public:
    PostgresConnector(std::string const& dbName);
    ~PostgresConnector() = default;

    void Execute(std::string const& sql);
    std::vector<std::map<std::string, std::string>> Query(std::string const& sql);
};

#endif
