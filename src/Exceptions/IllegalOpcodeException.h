#pragma once

#include "EmulatorException.h"

using word = uint16_t;

class IllegalOpcodeException : public EmulatorException {
public:
    IllegalOpcodeException() : EmulatorException("Illegal opcode") {}
};
