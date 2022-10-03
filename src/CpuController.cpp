#include "CpuController.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"

volatile bool Globals::is_paused;
volatile bool Globals::single_step;
volatile uint64_t Globals::elapsed_cycles;
std::mutex ExitHelper::exit_mutex;

CPU CpuController::cpu;
std::mutex CpuController::update_mutex;

CpuController::CpuController() {
    // Create and reset CPU
    cpu.reset();
    
    Globals::is_paused = false;
    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        ExitHelper::error("Error: Couldn't catch SIGINT\n");
    }
}

void CpuController::sig_handler(int sig) {
    // ^C ends execution
    if (sig == SIGINT) ExitHelper::exitCode(EXIT_SUCCESS, "");
}


// Enter program in ROM
void CpuController::read_ROM_file(const char* filename) const {
    std::ifstream hex_file(filename, std::fstream::in);
    if (!hex_file) {
        ExitHelper::error("Error: ROM file [%s] could not be found/opened\n", filename);
    }

    uint32_t address = 0;
    word high;
    word low;
    while (hex_file >> std::hex >> high) {
        assert(hex_file >> std::hex >> low);
        cpu.write_ROM(word(address), high, low);
        
        address++;
        // File too large for 16 bits of address space
        if (address > 0xFFFF) {
            ExitHelper::error("Error: ROM file is too large\n");
        }
    }

    // Make sure there is no leftover input
    if (!hex_file.eof()) {
        ExitHelper::error("Error: make sure the ROM file is a valid binary file\n");
    }

    hex_file.close();
}

void CpuController::call_update() {
    std::scoped_lock<std::mutex> lock(update_mutex);
    cpu.update();
}



// Execute the program
void CpuController::execute() const {
    // Schedule timer for calling update() every 30 milliseconds
    // Since execute() does not return, this thread will run forever
    auto update_thread = std::thread([]() {
        while (true) {
            auto x = std::chrono::steady_clock::now() + std::chrono::milliseconds(30);
            call_update();
            std::this_thread::sleep_until(x);
        }
    });

    // Give some time for the ncurses window to initialize.
    // Otherwise, if the program sends outputs too soon, the first chars wouldn't be displayed
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Amount of cycles to execute every (DEFAULT_SLEEP_US) microseconds
    auto CYCLES = int32_t((Globals::CLK_freq * DEFAULT_SLEEP_US) / TEN_RAISED_6);

    // Workaround for breakpoints not being checked on the first instruction
    for (word addr : Globals::breakpoints)
        if (addr == 0) Globals::is_paused = true;
    
    if (CYCLES < CPU::MAX_TIMESTEPS) run_slow();
    else run_fast(CYCLES, DEFAULT_SLEEP_US);

    // Unreachable
    assert(false);
}

// Execute the program at high clock speeds
void CpuController::run_fast(int32_t CYCLES, int32_t sleep_us) const {
    int32_t extra_cycles = 0;
    while (true) {
        // Wait until the program is unpaused
        while (Globals::is_paused);

        
        auto end_wait = std::chrono::steady_clock::now() + std::chrono::microseconds(sleep_us);
        {
            std::scoped_lock<std::mutex> lock(update_mutex);
            // Store the used extra cycles and subtract them from the next execution
            extra_cycles = cpu.execute(CYCLES - extra_cycles);
        }

        if (std::chrono::steady_clock::now() > end_wait) {
            ExitHelper::error("Target clock frequency too high for real-time emulation, try a slower clock\n");
        }
        
        std::this_thread::sleep_until(end_wait);
    }
}

// Execute the program at low clock speeds
void CpuController::run_slow() const {
    while (true) {
        // gcc with -O2 will "optimize" this to an endless loop unless is_paused is marked as volatile
        while (Globals::is_paused);

        // Request only 1 clock cycle so that exactly 1 instruction is executed.
        // Returned value is (required_timesteps - 1): sleep to simulate the instruction timesteps
        // The execution time of cpu.execute() is negligible at low clock speeds
        int32_t required_timesteps;
        {
            std::scoped_lock<std::mutex> lock(update_mutex);
            required_timesteps = cpu.execute(1) + 1;
        }
        int64_t required_us = TEN_RAISED_6 * required_timesteps / Globals::CLK_freq;
        
        std::this_thread::sleep_for(std::chrono::microseconds(required_us));
    }
}
