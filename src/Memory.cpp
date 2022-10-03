#include "Memory.h"
#include "Exceptions/EmulatorException.h"
#include "Utilities/Assert.h"

// BASIC MEMORY CELL

// WRITE
MemCell& MemCell::operator=(word rhs) {
    storedValue = rhs;
    return *this;
}
// READ
MemCell::operator word() const {
    return storedValue;
}



// REGISTER

Reg::Reg(bool is_zero) : zero_reg(is_zero) { }

// WRITE to register
MemCell& Reg::operator=(word rhs) {
    // Register zero is non-writable
    if (zero_reg) return *this;
    // Else, call base class
    return MemCell::operator=(rhs);
}



// REGISTER FILE

// When initializing the register file, set index 0 as the zero register
Regfile::Regfile() {
    registers[0] = Reg(true);
}
Reg& Regfile::ABI_A0() {
    const int ABI_A0_idx = 12;
    assert(ABI_names[ABI_A0_idx] == "a0");
    return registers[ABI_A0_idx];
}
Reg& Regfile::operator[](byte addr) {
    if (addr >= REGFILE_SZ) throw EmulatorException("Invalid regfile access");
    return registers[addr];
}


// GENERIC MEMORY

MemCell& Mem::operator[](word addr) { 
    return data[addr];
}



// ROM

using Rom = Mem;



// RAM

Ram::Ram(MemCell& p0, MemCell& p1, MemCell& p2, MemCell& p3) : 
    port0(&p0), port1(&p1), port2(&p2), port3(&p3) { }
    
// Memory mapping of RAM and IO
MemCell& Ram::operator[](word addr) { 
    // 0000-FEFF: RAM
    if (addr < 0xFF00) return Mem::operator[](addr);

    // FF00-FFFF: MMIO
    switch (addr) {
        case 0xFF00: return *port0;
        case 0xFF40: return *port1;
        case 0xFF80: return *port2;
        case 0xFFC0: return *port3;
        default: throw EmulatorException("Invalid memory access");
    }
}
