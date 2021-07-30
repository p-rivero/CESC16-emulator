#pragma once

#include <vector>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cassert>
#include <ciso646> // Include this so vscode doesn't complain about alternative logical operators

extern void destroy_terminal();

#if defined __cplusplus
#undef assert
#define assert(expr) \
    if (not static_cast <bool> (expr)) { \
        destroy_terminal(); __assert_fail(#expr, __FILE__, __LINE__, __ASSERT_FUNCTION); \
    }
#endif

// Add at the start of a function in order to prevent it from being run after another thread has been killed
#define _KILL_GUARD     std::lock_guard<std::mutex> _lock(Globals::kill_mutex);

using byte = uint8_t;
using word = uint16_t;


// Global variables that may be changed by user options
namespace Globals {
    extern bool strict_flg;         // True if -S has been used
    extern char *out_file;          // If -o has been used, it contains the name of the output file. Otherwise NULL
    extern bool break_flg;          // True if -b has been used
    extern std::vector<word> breakpoints;   // If break_flg is set, contains the breakpoint address

    extern std::mutex kill_mutex;   // Set to true in order to signal other threads that they should terminate
    extern int64_t CLK_freq;        // Emulated clock frequency (in Hz)
    extern word OS_critical_instr;  // Number of critical instructions that the OS must perform before an interrupt
    extern volatile bool is_paused; // True if the emulator is currently paused
    extern volatile bool single_step;        // True if in single step mode (break on every instruction)
    extern volatile uint64_t elapsed_cycles; // Store how many cycles the CPU has executed
};
