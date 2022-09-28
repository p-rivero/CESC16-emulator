#include "Timer.h"

// Tick the timer for a number of clock cycles
bool Timer::tick(int amount) {
    // If the timer is disabled, don't do anything
    if (!timer_active) return false;

    // Memory writes are performed on the rising edge of the last timestep,
    // the instruction that called Timer::write() does not increment the counter.
    if (just_updated) {
        just_updated = false;
        return false;
    }

    // Increment hardware timer
    timer_count += amount;

    // Timer would overflow, trigger interrupt
    if (timer_count >= (END_COUNT << 4)) {
        timer_count = (END_COUNT << 4);
        timer_active = false;
        return true;
    }
    return false;
}

// Reset the timer
void Timer::reset() {
    timer_count = 0;
    timer_active = false;
    just_updated = false;
}


// Set the current timer value
MemCell& Timer::operator=(word rhs) {
    timer_count &= 0x0000F; // Preserve prescaler bits
    timer_count |= (rhs << 4); // Add value of the timer

    // Also activate the timer
    timer_active = true;
    just_updated = true;
    return *this;
}

// Read the current value of the timer
Timer::operator word() const {
    return word(timer_count >> 4);
}
