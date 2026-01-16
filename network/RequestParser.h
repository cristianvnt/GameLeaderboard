#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include <string>
#include <map>

struct Request
{
    std::string command;
    std::map<std::string, std::string> params;
};

class RequestParser
{
public:
    static Request Parse(std::string const& message);
    static std::string BuildResponse(bool success, std::string const& message);
};

#endif