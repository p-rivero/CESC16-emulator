#pragma once

#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <ciso646> // Include this so vscode doesn't complain about alternative logical operators

// Redefine assert() so that the ncurses window is destroyed before printing and exiting
#if defined __cplusplus
#undef assert
#define assert(expr) \
    if (not static_cast <bool> (expr)) { \
        Terminal::destroy(); __assert_fail(#expr, __FILE__, __LINE__, __ASSERT_FUNCTION); \
    }
#endif

using byte = uint8_t;
using word = uint16_t;


// Global variables that may be changed by user options
namespace Globals {
    extern bool strict_flg;         // True if -S has been used
    extern char *out_file;          // If -o has been used, it contains the name of the output file. Otherwise NULL
    extern bool break_flg;          // True if -b has been used
    extern std::vector<word> breakpoints;   // If break_flg is set, contains the breakpoint address

    extern int64_t CLK_freq;        // Emulated clock frequency (in Hz)
    extern word OS_critical_instr;  // Number of critical instructions that the OS must perform before an interrupt
    extern volatile bool is_paused; // True if the emulator is currently paused
    extern volatile bool single_step;        // True if in single step mode (break on every instruction)
};
