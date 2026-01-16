#include "PostgresConnector.h"

#include <iostream>

PostgresConnector::PostgresConnector(std::string const &dbName)
{
    _connString = "dbname=" + dbName + " user=postgres" + " password=root1" + " host=localhost" + " port=5432";
}

void PostgresConnector::Execute(std::string const &sql)
{
    try
    {
        pqxx::connection conn(_connString);
        pqxx::work txn(conn);

        txn.exec(sql);
        txn.commit();

        std::cout << "Executed: " << sql << "\n";
    }
    catch (std::exception const& e)
    {
        std::cout << "Sql err: " << e.what() << "\n";
        throw;
    }
}

std::vector<std::map<std::string, std::string>> PostgresConnector::Query(std::string const &sql)
{
    std::vector<std::map<std::string, std::string>> results;

    try
    {
        pqxx::connection conn(_connString);
        pqxx::work txn(conn);

        pqxx::result res = txn.exec(sql);

        for (auto const& row : res)
        {
            std::map<std::string, std::string> rowMap;

            for (size_t i = 0; i < row.size(); ++i)
            {
                std::string colName = res.column_name(i);
                std::string val = row[i].is_null() ? "" : row[i].c_str();
                rowMap[colName] = val;
            }
            results.push_back(rowMap);
        }

        std::cout << "Query returned " << results.size() << " rows\n";
    }
    catch(const std::exception& e)
    {
        std::cout << "Sql err: " << e.what() << '\n';
        throw;
    }
    
    return results;
}
