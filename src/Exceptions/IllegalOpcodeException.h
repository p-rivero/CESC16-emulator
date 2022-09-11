#pragma once

#include "EmulatorException.h"

class IllegalOpcodeException : public EmulatorException {
public:
    IllegalOpcodeException() : EmulatorException("Illegal opcode") {}
};
