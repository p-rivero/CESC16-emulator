#pragma once

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
