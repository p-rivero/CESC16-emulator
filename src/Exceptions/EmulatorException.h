#pragma once

#include <exception>
#include <string>

class EmulatorException : public std::exception {
private:
    std::string msg;
public:
    EmulatorException(const std::string& msg) : msg(msg) {}
    virtual const char* what() const throw() {
        return msg.c_str();
    }
};
