#pragma once

#include <exception>
#include <string>

class EmulatorException : public std::exception {
private:
    std::string msg;
public:
    explicit EmulatorException(const std::string& msg) : msg(msg) {}
    const char* what() const throw() override {
        return msg.c_str();
    }
};
