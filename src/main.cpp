#include "CpuController.h"

// Initialize global variables
int64_t Globals::CLK_freq = 2000000;    // Default freq: 2000000 Hz (2 MHz)
word Globals::OS_critical_instr = 6;    // Don't interrupt the CPU on the first 6 instructions

char *Globals::out_file = NULL;         // Don't write output to any file
bool Globals::strict_flg = false;       // By default, strict mode is disabled (add extra protections)
bool Globals::silent_flg = false;       // By default, strict mode is disabled (add extra protections)
// Store the addresses of all the breakpoints and exitpoints
std::vector<word> Globals::breakpoints;
std::vector<word> Globals::exitpoints;


void print_help(const char* prog_name) {
    printf("CESC16 EMULATOR\n");
    printf("       Run binary files for the CESC16 architecture\n");
    printf("\nUSAGE:\n");
    printf("       %s [OPTION] FILE\n", prog_name);
    printf("       FILE is the path to the binary file to be loaded in ROM\n");
    printf("\nOPTIONS:\n");
    printf("       -b address   Add breakpoint at an address (pause emulator when PC=addr)\n");
    printf("       -f freq_hz   Frequency of the emulated CPU clock (in Hertz)\n");
    printf("       -h           Show this help message\n");
    printf("       -k time_us   Set the delay of the keyboard (per key, in microseconds)\n");
    printf("       -o filename  Output file (dump all CPU outputs to file)\n");
    printf("       -s           Silent mode (don't display the ncurses interface)\n");
    printf("       -S           Strict mode (disable extra emulator protections)\n");
    printf("       -t time_us   Set the delay of the terminal (per character, in microseconds)\n");
    printf("       -x address   Add exit point at an address (exit emulator when PC=addr)\n");
    printf("\nEXAMPLES:\n");
    printf("       %s -S -f 1000 my_file.hex     # Run emulator at 1 kHz in strict mode\n", prog_name);
    printf("       %s my_file.hex -o output.txt  # Write all CPU outputs to output.txt\n", prog_name);
    printf("       %s my_file.hex -b 0 -b 50     # Run with 2 breakpoints\n", prog_name);
    printf("       %s my_file.hex -t 1000000     # Very slow terminal: 1 char per sec.\n", prog_name);
    exit(EXIT_SUCCESS);
}

void add_breakpoint(const char *addr, std::vector<word>& breakpoints) {
    char *endptr;
    long address = strtol(addr, &endptr, 16);

    if (*endptr != '\0') {
        // Not all input could be parsed
        fprintf(stderr, "Error: Invalid breakpoint [%s], make sure it's a valid hex integer\n", addr);
        exit(EXIT_FAILURE);
    }
    if (address < 0 or address >= 0xFFFF) {
        fprintf(stderr, "Error: Invalid breakpoint [%s], make sure it's between 0 and 0xFFFF\n", addr);
        exit(EXIT_FAILURE);
    }
    breakpoints.push_back(address);
}

int main (int argc, char **argv) {
    // Variables for getopt
    int c;
    opterr = false; // Disable errors, return '?' instead

    // Parse arguments
    if (argc == 1) print_help(argv[0]);
    
    // -b, -f, -k, -o, -t, -x take an argument (indicated by ':')
    while ((c = getopt(argc, argv, "b:f:hk:o:Sst:x:")) != -1) {
        switch (c) {
        case 'b':
            add_breakpoint(optarg, Globals::breakpoints);
            break;
        
        case 'f':   // Set clock frequency
            Globals::CLK_freq = atoll(optarg);
            if (Globals::CLK_freq <= 0) {
                fprintf(stderr, "Error: Invalid clock frequency, make sure it's a positive integer\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'h':
            print_help(argv[0]);    // Print help
            break;
            
        case 'k':   // Set keyboard delay
            Globals::keyboard_delay = atoll(optarg);
            if (Globals::keyboard_delay < 0) {
                fprintf(stderr, "Error: Invalid keyboard delay, make sure it's a positive integer\n");
                exit(EXIT_FAILURE);
            }
            break;

        case 'o':
            Globals::out_file = optarg; // Output to file
            break;

        case 'S':
            Globals::strict_flg = true; // Strict mode
            break;
            
        case 's':
            Globals::silent_flg = true; // Strict mode
            break;
            
        case 't':   // Set terminal delay
            Globals::terminal_delay = atoll(optarg);
            if (Globals::terminal_delay < 0) {
                fprintf(stderr, "Error: Invalid terminal delay, make sure it's a positive integer\n");
                exit(EXIT_FAILURE);
            }
            break;
            
        case 'x':
            add_breakpoint(optarg, Globals::exitpoints);
            break;
            
        case '?':   // Error
            if (optopt == 'b' or optopt == 'f' or optopt == 'o' or optopt == 't') {
                // Options that take an argument
                fprintf(stderr, "Error: An argument is required for the option -%c\n", optopt);
            }
            else if (isprint(optopt)) {
                // Option is a printable character, but wasn't recognized
                fprintf(stderr, "Error: Unknown option: -%c\n", optopt);
            }
            else fprintf(stderr, "Error: Unknown option character 0x%x\n", optopt&0xFF);
            
            exit(EXIT_FAILURE);
        
        default: abort();
        }
    }

    // Todo Add arguments: -d (use given directory (instead of PWD) for Disk emulation)

    if (optind == argc) {
        // No non-option argument provided
        fprintf(stderr, "Error: No ROM filename was provided\n");
        exit(EXIT_FAILURE);
    }

    if (argc - optind > 1) {
        // More than 1 non-option argument provided
        fprintf(stderr, "Error: Too many filenames provided\n");
        for (int i = optind; i < argc; i++)
            printf ("-> \"%s\"\n", argv[i]);
        exit(EXIT_FAILURE);
    }

    CpuController cpu;
    cpu.read_ROM_file(argv[optind]);
    cpu.execute();
}
