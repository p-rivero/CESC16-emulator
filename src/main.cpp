#include "CPU.h"
#include <fstream>

void read_bin(std::ifstream& bin_file, CPU& cpu) {
    uint32_t address = 0;
    word high, low;
    while (bin_file >> std::hex >> high) {
        assert(bin_file >> std::hex >> low);
        cpu.write_ROM(address++, high, low);

        if (address > 0xFFFF) {
            // File too large for 16 bits of address space
            printf("Error: ROM file is too large\n");
            exit(EXIT_FAILURE);
        }
    }

    // Make sure there is no leftover input
    if (not bin_file.eof()) {
        printf("Error: make sure the ROM file is a valid binary file\n");
        exit(EXIT_FAILURE);
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
    CPU cpu;
    cpu.reset();

    // Enter program in ROM
    std::ifstream bin_file(argv[1], std::fstream::in);
    if (not bin_file) {
        perror("ROM file could not be opened");
        exit(EXIT_FAILURE);
    }
    read_bin(bin_file, cpu);
    bin_file.close();
    
    // Execute program
    for (uint32_t i = 0; i < 50; i++) {
        cpu.execute(2);
    }

    exit(EXIT_SUCCESS);
}
