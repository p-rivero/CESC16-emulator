#include "Keyboard.h"

int Globals::keyboard_delay = 0;

Keyboard::Keyboard() {
    term = Terminal::initialize();
    
    output_reg = 0; // At startup, clear the output register
    can_interrupt = false; // At startup, do not send any INT until the OS is ready
}

// WRITE
MemCell& Keyboard::operator=(word rhs) {
    if (busy_flag != 0 and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw "Keyboard/Serial: attempting to output while the controller was busy";
    }
    if (rhs > 0x7F and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 7-bit long
        throw "Keyboard/Serial: Value written is bigger than 7 bit and will be truncated";
    }
    
    byte val = rhs & 0x7F;  // Only lower 7 bits are used
    if (val == ACK) {
        // Input acknowledged: clear output register
        output_reg = 0;
    }
    else if (val == RDY) {
        // OS is ready to be interrupted again
        can_interrupt = true;
        output_reg = 0; // Also clear the output register (same as ACK)
    }
    else throw "Invalid keyboard command";
    
    if (Globals::keyboard_delay > 0) {
        busy_flag = true; // Writing to the input register sets the busy flag
        // Wait for some time and clear the flag
        std::thread([this]() {
            _KILL_GUARD
            std::this_thread::sleep_for(std::chrono::microseconds(Globals::keyboard_delay));
            busy_flag = false;
        }).detach();
    }
    
    return *this;
}

// READ
Keyboard::operator int() const {    
    assert(output_reg <= 0x7F);
    return output_reg | (busy_flag << 7);
}


// Called periodically to check for new inputs. Returns true if an IRQ is triggered
bool Keyboard::update() {    
    term->update_input();
    // If another char is being presented OR the CPU is in the service routine, don't do anything
    if (output_reg or not can_interrupt) return false;
    
    byte pressed_key = term->get_input();
    if (pressed_key != 0) {
        // Write the new char in the output reg (causing an IRQ) and update variables
        output_reg = pressed_key;
        // output_reg_full = true;
        can_interrupt = false;
        return true;
    }
    return false; // No key was pressed: don't interrupt
}
