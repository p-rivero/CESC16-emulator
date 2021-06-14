#include "CpuController.h"

void print_help(const char* prog_name) {
    printf("CESC16 EMULATOR\n");
    printf("       Run binary files for the CESC16 architecture\n\n");
    printf("USAGE:\n");
    printf("       %s [OPTION] FILE\n", prog_name);
    printf("       FILE is the path to the binary file to be loaded in ROM\n\n");
    printf("OPTIONS:\n");
    printf("       -f Frequency of the emulated CPU clock (in Hertz)\n");
    printf("       -h Show this help message\n\n");
    printf("EXAMPLE:\n");
    printf("       %s -f 1000 ./my_file.hex     # Run emulator at 1 kHz\n", prog_name);
    exit(EXIT_SUCCESS);
}

int main (int argc, char **argv) {
    int64_t CLK_freq = 2000000;  // Default freq: 2000000 Hz (2 MHz)

    // Variables for getopt
    int c;
    opterr = 0; // Disable message errors, return '?' instead

    // Parse arguments
    if (argc == 1) print_help(argv[0]);
    
    // Capture -f with an argument (':') and -h without argument
    while ((c = getopt(argc, argv, "f:h")) != -1) {
        switch (c) {
        case 'f':   // Set clock frequency
            CLK_freq = atoll(optarg);
            if (CLK_freq <= 0) {
                fprintf(stderr, "Error: Invalid clock frequency, make sure it's a positive integer\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'h': print_help(argv[0]);  // Print help
            
        case '?':   // Error
            if (optopt == 'f') {
                // -f argument takes an argument
                fprintf(stderr, "Error: An argument is required for the option -%c\n", optopt);
            }
            else if (isprint (optopt)) {
                // Option is a printable character, but wasn't recognized
                fprintf(stderr, "Error: Unknown option: -%c\n", optopt);
            }
            else fprintf(stderr, "Error: Unknown option character 0x%x\n", optopt&0xFF);
            
            exit(EXIT_FAILURE);
        
        default: abort();
        }
    }

    if (optind == argc) {
        // No non-option argument provided
        fprintf(stderr, "Error: No ROM filename was provided\n");
        exit(EXIT_FAILURE);
    }

    if (argc - optind > 1) {
        // More than 1 non-option argument provided
        fprintf(stderr, "Error: Too many filenames provided\n");
        for (int i = optind; i < argc; i++)
            printf ("-> %s\n", argv[i]);
        exit(EXIT_FAILURE);
    }

    CpuController cpu;
    cpu.read_ROM_file(argv[optind]);
    cpu.execute(CLK_freq);
}
