#include "CPU.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <functional>

const double FREQ = 1600;    // 2 MHz 2000000

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


    const int sleep_ms = 10;
    // Amount of cycles to execute every (sleep_ms) millisecods
    int CYCLES = FREQ * double(sleep_ms) / 1000.0;

    if (CYCLES<16) fatal_error("Target clock frequency too low"); // Todo: if CYCLES<16, increase sleep_ms and try again

    // Execute program
    int32_t extra_cycles = 0;
    while (true) {
        auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(10);
        // Store the used extra cycles and subtract them from the next execution
        extra_cycles = cpu->execute(CYCLES - extra_cycles);

        if (std::chrono::steady_clock::now() > x)
            fatal_error("Target clock frequency too high for real-time emulation, try a slower clock");
        
        std::this_thread::sleep_until(x);
    }

    fatal_error("UNREACHABLE");
}
