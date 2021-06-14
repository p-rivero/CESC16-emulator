#include "CpuController.h"


CpuController::CpuController() {
    // Create and reset CPU
    cpu = new CPU;
    cpu->reset();
}

CpuController::~CpuController() {
    if (cpu != nullptr) delete cpu;
}

void CpuController::fatal_error(const char* format, ...) {
    if (cpu != nullptr) delete cpu;
    // Print error message
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end (args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}



// Enter program in ROM
void CpuController::read_ROM_file(const char* filename) {
    std::ifstream hex_file(filename, std::fstream::in);
    if (not hex_file) fatal_error("Error: ROM file [%s] could not be found/opened", filename);   

    uint32_t address = 0;
    word high, low;
    while (hex_file >> std::hex >> high) {
        assert(hex_file >> std::hex >> low);
        cpu->write_ROM(address++, high, low);

        // File too large for 16 bits of address space
        if (address > 0xFFFF) fatal_error("Error: ROM file is too large");
    }

    // Make sure there is no leftover input
    if (not hex_file.eof()) fatal_error("Error: make sure the ROM file is a valid binary file");

    hex_file.close();
}



void CpuController::timer_start(std::function<void(CPU*)> func, unsigned int interval) {
    std::thread([this, func, interval]() {
        while (true) {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
            func(this->cpu);
            std::this_thread::sleep_until(x);
        }
    }).detach();
}

void CpuController::call_update(CPU *cpu) { cpu->update(); }



// Execute the program
void CpuController::execute(int64_t CLK_freq) {
    // Schedule timer for calling update() every 30 milliseconds
    timer_start(call_update, 30);

    // Give some time for the ncurses window to initialize.
    // Otherwise, if the program sends outputs too soon, the first chars wouldn't be displayed
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Amount of cycles to execute every (DEFAULT_SLEEP_US) microseconds
    int32_t CYCLES = (CLK_freq * DEFAULT_SLEEP_US) / TEN_RAISED_6;

    if (CYCLES < CPU::MAX_TIMESTEPS) run_slow(CLK_freq);
    else run_fast(CYCLES, DEFAULT_SLEEP_US);

    fatal_error("UNREACHABLE");
}

// Execute the program at high clock speeds
void CpuController::run_fast(int32_t CYCLES, int32_t sleep_us) {
    int32_t extra_cycles = 0;
    while (true) {
        auto x = std::chrono::steady_clock::now() + std::chrono::microseconds(sleep_us);
        // Store the used extra cycles and subtract them from the next execution
        extra_cycles = cpu->execute(CYCLES - extra_cycles);

        if (std::chrono::steady_clock::now() > x)
            fatal_error("Target clock frequency too high for real-time emulation, try a slower clock");
        
        std::this_thread::sleep_until(x);
    }
}

// Execute the program at low clock speeds
void CpuController::run_slow(int64_t CLK_freq) {
    while (true) {
        // Request only 1 clock cycle so that exactly 1 instruction is executed.
        // Returned value is (required_timesteps - 1): sleep to simulate the instruction timesteps
        // The execution time of cpu->execute() is negligible at low clock speeds
        int32_t required_timesteps = cpu->execute(1) + 1;
        int64_t required_us = TEN_RAISED_6 * required_timesteps / CLK_freq;
        
        std::this_thread::sleep_for(std::chrono::microseconds(required_us));
    }
}
