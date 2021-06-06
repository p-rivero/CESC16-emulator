#include "CPU.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <functional>

const double FREQ_HZ = 2000000;         // 2000000 Hz (2 MHz) 
const int32_t DEFAULT_SLEEP_US = 10000; // 10000 microseconds (10 ms)
const int32_t TEN_RAISED_6 = 1000000;   // 10^6

CPU *cpu;

void fatal_error(const char* msg) {
    delete cpu;
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

void read_bin(std::ifstream& hex_file) {
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
}

void timer_start(std::function<void(void)> func, unsigned int interval) {
    std::thread([func, interval]() {
        while (true) {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(interval);
            func();
            std::this_thread::sleep_until(x);
        }
    }).detach();
}

void call_update() { cpu->update(); }


// Execute the program at high clock speeds
void run_fast(int32_t CYCLES, int32_t sleep_us) {
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
void run_slow() {
    while (true) {
        // Request only 1 clock cycle so that exactly 1 instruction is executed.
        // Returned value is (required_timesteps - 1): sleep to simulate the instruction timesteps
        // The execution time of cpu->execute() is negligible at low clock speeds
        int32_t required_timesteps = cpu->execute(1) + 1;
        int64_t required_us = TEN_RAISED_6 * required_timesteps / FREQ_HZ;
        
        std::this_thread::sleep_for(std::chrono::microseconds(required_us));
    }
}

int main(int argc, char **argv) {
    // Check arguments
    if (argc != 2) {
        printf("CESC16 Emulator\nUsage: %s file\n", argv[0]);
        printf("file = path to the binary file to be loaded in ROM\n");
        exit(EXIT_SUCCESS);
    }

    // Create and reset CPU
    cpu = new CPU;
    cpu->reset();

    // Enter program in ROM
    std::ifstream hex_file(argv[1], std::fstream::in);
    if (not hex_file) fatal_error("ROM file could not be opened");
    read_bin(hex_file);
    hex_file.close();
    
    // Schedule timer for calling update() every 30 milliseconds
    timer_start(call_update, 30);

    // Give some time for the ncurses window to initialize.
    // Otherwise, if the program sends outputs too soon, the first chars wouldn't be displayed
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Amount of cycles to execute every (DEFAULT_SLEEP_US) microseconds
    const int32_t CYCLES = FREQ_HZ * double(DEFAULT_SLEEP_US) / TEN_RAISED_6;

    if (CYCLES < CPU::MAX_TIMESTEPS) run_slow();
    else run_fast(CYCLES, DEFAULT_SLEEP_US);

    fatal_error("UNREACHABLE");
}
