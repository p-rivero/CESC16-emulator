#pragma once

#include "CPU.h"

#include <functional>
#include <thread>

class CpuController {
private:
    static const int64_t DEFAULT_SLEEP_US = 10000; // 10000 microseconds (10 ms)
    static const int64_t TEN_RAISED_6 = 1000000;   // 10^6
    static CPU cpu;
    static std::mutex update_mutex;

    static void sig_handler(int sig);
    static void call_update();
    [[noreturn]] void run_fast(int32_t CYCLES, int32_t sleep_us) const;
    [[noreturn]] void run_slow() const;


public:
    CpuController();
    
    void read_ROM_file(const char* filename) const;
    [[noreturn]] void execute() const;
};
