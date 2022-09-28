#pragma once

#include <cstdint>
#include <string>
#include <vector>

using byte = uint8_t;
using word = uint16_t;


// Global variables that may be changed by user options
namespace Globals {
    extern bool strict_flg;         // True if -S has been used
    extern bool silent_flg;         // True if -s has been used
    extern char *out_file;          // If -o has been used, it contains the name of the output file. Otherwise nullptr
    extern std::vector<word> breakpoints;   // Contains the breakpoint addresses (if -b has been used)
    extern std::vector<word> exitpoints;    // Contains the exitpoint addresses (if -x has been used)
    extern int terminal_delay;      // How many microseconds to wait before the output terminal clears the busy flag
    extern int keyboard_delay;      // Microseconds to wait before the keyboard controller clears the busy flag

    extern int64_t CLK_freq;        // Emulated clock frequency (in Hz)
    extern word OS_critical_instr;  // Number of critical instructions that the OS must perform before an interrupt
    extern volatile bool is_paused; // True if the emulator is currently paused
    extern volatile bool single_step;        // True if in single step mode (break on every instruction)
    extern volatile uint64_t elapsed_cycles; // Store how many cycles the CPU has executed
    extern std::string disk_root_dir;   // Root directory used for disk emulation
};
