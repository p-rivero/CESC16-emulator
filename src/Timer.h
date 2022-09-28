#pragma once

#include "Memory.h"

class Timer : public MemCell {

/*  A note on simulating timers:
    This emulator doesn't simulate the timesteps of the original CPU. Instead, entire instructions are
    emulated atomically and then the timer is incremented by the amount of cycles that the instruction
    would take. This means that an exact simulation of the timer will never be possible.
    Therefore, the goal of this timer is not to report the exact same readings as the actual CPU, but
    to stay consistent in the long term (avoid drifting away from the correct timer value).
*/

private:
    // Value of the timer when the count ends
    static const int END_COUNT = 0xF000;   // This value should be 0x10000. The restricted range is a workaround for a hardware bug 
    
    /* Bits 0..3  = Ignored, 16x prescaler for the timer
       Bits 4..20 = The 16-bit timer itself
       Bit 21     = Used for overflow detection */
    uint32_t timer_count = 0;

    // If true, the timer gets incremented
    bool timer_active = false;

    // True if the timer has been written in this instruction
    bool just_updated = false;

public:
    // WRITE: Set the current timer value
    MemCell& operator=(word rhs) override;
    // READ: Read the current value of the timer
    operator word() const override;

    // Tick the timer for a number of clock cycles
    // Returns true only if an overflow has occurred
    bool tick(int amount);

    // Reset the timer
    void reset();
};
