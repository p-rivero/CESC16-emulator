#include "CpuController.h"

void print_help() {
    // TODO
    //printf("CESC16 Emulator\nUsage: %s file\n", argv[0]);
    //printf("file = path to the binary file to be loaded in ROM\n");
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {

    int64_t CLK_freq = 2000000;  // Default freq: 2000000 Hz (2 MHz)
    char *filename = NULL;

    // Parse arguments
    if (argc == 1) print_help();

    filename = argv[1];

    if (filename == NULL) {
        // TODO
        // printf("Error: ROM file was not provided\n");
        // printf("Use the -R option to specify a file or the -h option for help\n");
        exit(EXIT_FAILURE);
    }

    CpuController cpu;
    cpu.read_ROM_file(filename);
    cpu.execute(CLK_freq);
}
