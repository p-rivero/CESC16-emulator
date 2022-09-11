#pragma once

#include "../Terminal.h"

namespace ExitHelper {
    
    // Lock that prevents other threads from executing if a thread has been killed
    extern std::mutex exit_mutex;

    namespace {
        inline static void exit_impl(int code, const char *format, va_list args) {
            // Acquire exit lock
            std::lock_guard<std::mutex> lock(exit_mutex);
            
            Terminal::destroy();
            std::vfprintf(stderr, format, args);
            std::exit(code);
        }
    }
    
    inline static void exitCode(int code, const char *format, ...) {
        va_list args;
        va_start(args, format);
        exit_impl(code, format, args);
        va_end(args);
    }
    
    inline static void error(const char *format, ...) {
        va_list args;
        va_start(args, format);
        exit_impl(EXIT_FAILURE, format, args);
        va_end(args);
    }
};
