#include "RequestParser.h"

#include <sstream>

Request RequestParser::Parse(std::string const& message)
{
    Request req;
    
    std::string trimmed = message;
    while (!trimmed.empty() && (trimmed.back() == '\n' || trimmed.back() == '\r'))
        trimmed.pop_back();
    
    std::istringstream stream(trimmed);
    std::string token;
    
    bool first = true;
    while (std::getline(stream, token, '|'))
    {
        if (first)
        {
            req.command = token;
            first = false;
        }
        else
        {
            size_t colonPos = token.find(':');
            if (colonPos != std::string::npos)
            {
                std::string key = token.substr(0, colonPos);
                std::string value = token.substr(colonPos + 1);
                req.params[key] = value;
            }
        }
    }
    
    return req;
}

std::string RequestParser::BuildResponse(bool success, std::string const& message)
{
    return (success ? "SUCCESS|" : "ERROR|") + message + "\n";
}