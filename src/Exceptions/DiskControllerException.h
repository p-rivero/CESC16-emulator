#pragma once

#include "EmulatorException.h"

class DiskControllerException : public EmulatorException {
public:
    explicit DiskControllerException(const std::string& msg) : EmulatorException("[Disk Controller] " + msg) {}
};
