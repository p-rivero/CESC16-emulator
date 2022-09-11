#pragma once

#include "ExitHelper.h"

#ifdef assert
    #error "assert was already defined! Check your includes"
#endif

#define assert(cond) Assert::assertCondition(static_cast<bool>(cond), #cond, __FILE__, __LINE__)

namespace Assert
{
    // Assert that a condition is true
    // If it is not, print an error message and exit
    inline void assertCondition(bool condition, const char *message, const char *file, int line) {
        if (!condition) {
            ExitHelper::error("Assertion failed: %s\n\tFile: %s\n\tLine: %d\n", message, file, line);
        }
    }
}
