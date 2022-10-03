#pragma once

#include <cstdint>
#include <string>
#include <vector>

using byte = uint8_t;
using word = uint16_t;


// Global variables that may be changed by user options
class Globals {
private:
    Globals() = delete; // Prevent instantiation
    
public:
    static bool strict_flg;         // True if -S has been used
    static bool silent_flg;         // True if -s has been used
    static char *out_file;          // If -o has been used, it contains the name of the output file. Otherwise nullptr
    static std::vector<word> breakpoints;   // Contains the breakpoint addresses (if -b has been used)
    static std::vector<word> exitpoints;    // Contains the exitpoint addresses (if -x has been used)
    static int terminal_delay;      // How many microseconds to wait before the output terminal clears the busy flag
    static int keyboard_delay;      // Microseconds to wait before the keyboard controller clears the busy flag

    static int64_t CLK_freq;        // Emulated clock frequency (in Hz)
    static word OS_critical_instr;  // Number of critical instructions that the OS must perform before an interrupt
    static volatile bool is_paused; // True if the emulator is currently paused
    static volatile bool single_step;        // True if in single step mode (break on every instruction)
    static volatile uint64_t elapsed_cycles; // Store how many cycles the CPU has executed
    static std::string disk_root_dir;   // Root directory used for disk emulation
};
