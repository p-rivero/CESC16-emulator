#include "Keyboard.h"
#include "Exceptions/EmulatorException.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"

#include <thread>

int Globals::keyboard_delay = 0;

Keyboard::Keyboard() {
    term = Terminal::get_instance();
}

// WRITE
MemCell& Keyboard::operator=(word rhs) {
    if (busy_flag != 0 && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw EmulatorException("Keyboard/Serial: attempting to output while the controller was busy");
    }
    
    if (rhs > 0x7F && !Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 7-bit long
        throw EmulatorException("Keyboard/Serial: Value written is bigger than 7 bit and will be truncated");
    }
    
    // Only lower 7 bits are used
    if (byte val = rhs & 0x7F; val == ACK) {
        // Input acknowledged: clear output register
        output_reg = 0;
    }
    
    else if (val == RDY) {
        // OS is ready to be interrupted again
        can_interrupt = true;
        output_reg = 0; // Also clear the output register (same as ACK)
    }
    else throw EmulatorException("Invalid keyboard command");
    
    if (Globals::keyboard_delay > 0) {
        busy_flag = true; // Writing to the input register sets the busy flag
        // Wait for some time and clear the flag
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::microseconds(Globals::keyboard_delay));
            
            // Acquire exit lock to prevent segfault when the main thread is exiting
            std::scoped_lock<std::mutex> lock(ExitHelper::get_exit_mutex());
            
            busy_flag = false;
        }).detach();
    }
    
    return *this;
}

// READ
Keyboard::operator word() const {    
    assert(output_reg <= 0x7F);
    return output_reg | word(int(busy_flag) << 7);
}


// Called periodically to check for new inputs. Returns true if an IRQ is triggered
bool Keyboard::update() {    
    term->update_input();
    // If another char is being presented OR the CPU is in the service routine, don't do anything
    if (output_reg || !can_interrupt) return false;
    
    if (byte pressed_key = term->get_input(); pressed_key != 0) {
        // Write the new char in the output reg (causing an IRQ) and update variables
        output_reg = pressed_key;
        can_interrupt = false;
        return true;
    }
    return false; // No key was pressed: don't interrupt
}
