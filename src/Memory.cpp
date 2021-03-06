#include "Memory.h"

// BASIC MEMORY CELL

// WRITE
MemCell& MemCell::operator=(word rhs) {
    val_ = rhs;
    return *this;
}
// READ
MemCell::operator int() const {
    return val_;
}



// REGISTER

Reg::Reg() {
    zero_reg = false;
    val_ = 0;
}
Reg::Reg(bool is_zero) {
    zero_reg = is_zero;
    val_ = 0;
}
// WRITE to register
MemCell& Reg::operator=(word rhs) {
    // Register zero is non-writable
    if (not zero_reg) val_ = rhs;
    return *this;
}



// REGISTER FILE

// When initializing the register file, set index 0 as the zero register
Regfile::Regfile() {
    Data[0] = Reg(true);
}
Reg& Regfile::ABI_A0() {
    const int ABI_A0_idx = 12;
    assert(ABI_names[ABI_A0_idx] == "a0");
    return Data[ABI_A0_idx];
}
Reg& Regfile::operator[](byte addr) {
    if (addr >= REGFILE_SZ) throw "Invalid regfile access";
    return Data[addr];
}


// GENERIC MEMORY

MemCell& Mem::operator[](word addr) { 
    return Data[addr];
}



// ROM

using Rom = Mem;



// RAM

Ram::Ram(MemCell& p0, MemCell& p1, MemCell& p2, MemCell& p3) {
    Port0 = &p0;  Port1 = &p1;  Port2 = &p2;  Port3 = &p3;
}
// Memory mapping of RAM and IO
MemCell& Ram::operator[](word addr) { 
    // 0000-FEFF: RAM
    if (addr < 0xFF00) return Mem::operator[](addr);

    // FF00-FFFF: MMIO
    switch (addr) {
        case 0xFF00: return *Port0;
        case 0xFF40: return *Port1;
        case 0xFF80: return *Port2;
        case 0xFFC0: return *Port3;
        default: throw "Invalid memory access";
    }
}
