#include "CPU.h"
#include <fstream>

void read_bin(std::ifstream& hex_file, CPU *cpu) {
    uint32_t address = 0;
    word high, low;
    while (hex_file >> std::hex >> high) {
        assert(hex_file >> std::hex >> low);
        cpu->write_ROM(address++, high, low);

        if (address > 0xFFFF) {
            // File too large for 16 bits of address space
            delete cpu;
            printf("Error: ROM file is too large\n");
            exit(EXIT_FAILURE);
        }
    }

    // Make sure there is no leftover input
    if (not hex_file.eof()) {
        delete cpu;
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
    CPU *cpu = new CPU;
    cpu->reset();

    // Enter program in ROM
    std::ifstream hex_file(argv[1], std::fstream::in);
    if (not hex_file) {
        delete cpu;
        perror("ROM file could not be opened");
        exit(EXIT_FAILURE);
    }
    read_bin(hex_file, cpu);
    hex_file.close();
    
    // Execute program
    while (true) {
        cpu->execute(50);
    }

    delete cpu;
    exit(EXIT_SUCCESS);
}
