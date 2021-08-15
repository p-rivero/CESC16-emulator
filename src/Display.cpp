#include "Display.h"

int Globals::terminal_delay = 0; // In real hardware this would be 32 microseconds

Display::Display() {
    term = Terminal::initialize();
}

// WRITE
MemCell& Display::operator=(word rhs) {
    if (busy_flag != 0 and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when overwriting the controller input register
        throw "Terminal: attempting to output while the controller was busy";
    }
    if (rhs > 0xFF and not Globals::strict_flg) {
        // If strict mode is not enabled, warn when written value is more than 8-bit long
        throw "Terminal: Value written is bigger than 8 bit and will be truncated";
    }
    if (Globals::terminal_delay > 0) {
        busy_flag = rhs;
        // VGA terminal can process 1 input every 32 microseconds, wait and clear the flag
        std::thread([this]() {
            _KILL_GUARD
            std::this_thread::sleep_for(std::chrono::microseconds(Globals::terminal_delay));
            busy_flag = 0;
        }).detach();
    }
    
    term->output(rhs);
    return *this;
}

// READ
Display::operator int() const {
    return busy_flag;
}
