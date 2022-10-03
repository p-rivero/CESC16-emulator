#pragma once

#include <mutex>

#include "../Terminal.h"

class ExitHelper {
    
private:
    // Lock that prevents other threads from executing if a thread has been killed
    static std::mutex exit_mutex;
    
    [[noreturn]] inline static void exit_impl(int code, const char *format, va_list args) {
        // Acquire exit lock
        std::scoped_lock<std::mutex> lock(exit_mutex);
        
        Terminal::get_instance()->destroy();
        std::vfprintf(stderr, format, args);
        std::exit(code);
    }

public:
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
    
    inline static std::mutex& get_exit_mutex() {
        return const_cast<std::mutex &>(exit_mutex);
    }
};
