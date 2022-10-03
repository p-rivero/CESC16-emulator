#pragma once

#include <mutex>

#include "../Terminal.h"

namespace ExitHelper {
    
    // Lock that prevents other threads from executing if a thread has been killed
    extern std::mutex exit_mutex;

    namespace {
        [[noreturn]] inline void exit_impl(int code, const char *format, va_list args) {
            // Acquire exit lock
            std::scoped_lock<std::mutex> lock(exit_mutex);
            
            Terminal::get_instance()->destroy();
            std::vfprintf(stderr, format, args);
            std::exit(code);
        }
    }
    
    [[noreturn]] inline static void exitCode(int code, const char *format, ...) {
        va_list args;
        va_start(args, format);
        exit_impl(code, format, args);
        va_end(args);
    }
    
    [[noreturn]] inline static void error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        exit_impl(EXIT_FAILURE, format, args);
        va_end(args);
    }
};
