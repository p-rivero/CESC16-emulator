#pragma once

#include "Terminal.h"

class Keyboard : public MemCell {
private:
    Terminal *term;
    volatile byte output_reg;    // Emulated output register
    volatile bool can_interrupt; // True if the OS has signaled that it's safe to interrupt
    bool busy_flag;              // Emulated busy flag (set and cleared by hardware)

    // Constants for the keyboard interface
    static const byte ACK = 0x06;
    static const byte RDY = 0x07;

public:
    Keyboard();

    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
    
    // Called periodically to check for new inputs. Returns true if there is a new input
    bool update();
};
