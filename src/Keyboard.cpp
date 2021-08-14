#include "Keyboard.h"

Keyboard::Keyboard() {
    term = Terminal::initialize();
}

// WRITE
MemCell& Keyboard::operator=(word rhs) {
    byte val = rhs & 0x7F;  // Only lower 7 bits are used
    if (val == ACK) term->ack_input();
    else if (val == RDY) term->ready_input();
    else throw "Invalid keyboard command";
    
    return *this;
}

// READ
Keyboard::operator int() const {
    return term->read_input();
}
