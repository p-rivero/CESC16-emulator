#include "CPU.h"

int main() {
    CPU cpu;
    cpu.reset();

    for (uint32_t i = 0; i < 50; i++) {
        cpu.execute(2);
    }
    exit(EXIT_SUCCESS);
}
