#pragma once

#include "CPU.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <functional>

class CpuController {
private:
    static const int64_t DEFAULT_SLEEP_US = 10000; // 10000 microseconds (10 ms)
    static const int64_t TEN_RAISED_6 = 1000000;   // 10^6
    static CPU *cpu;
    static std::mutex update_mutex;

    static void sig_handler(int sig);

    void fatal_error(const char* msg, ...);
    void timer_start(std::function<void(CPU*)> func, unsigned int interval);
    static void call_update(CPU *cpu);
    void run_fast(int32_t CYCLES, int32_t sleep_us);
    void run_slow();


public:
    CpuController();
    ~CpuController();

    void read_ROM_file(const char* filename);
    void execute();
};
