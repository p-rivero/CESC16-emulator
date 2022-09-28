#pragma once

#include "Terminal.h"

class Keyboard : public MemCell {
private:
    Terminal *term;
    volatile byte output_reg = 0;  // Emulated output register
    volatile bool can_interrupt = false;  // True if the OS has signaled that it's safe to interrupt
    bool busy_flag;  // Emulated busy flag (set and cleared by hardware)

    // Constants for the keyboard interface
    static const byte ACK = 0x06;
    static const byte RDY = 0x07;

public:
    Keyboard();

    // WRITE
    MemCell& operator=(word rhs) override;
    // READ
    operator word() const override;
    
    // Called periodically to check for new inputs. Returns true if there is a new input
    bool update();
};
