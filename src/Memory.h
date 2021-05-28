#pragma once

#include "includes.h"

class MemCell {
protected:
    word val_;

public:
    // WRITE
    virtual MemCell& operator=(word rhs) {
        val_ = rhs;
        return *this;
    }
    // READ
    virtual operator int() const { return val_; }
};

class Reg : public MemCell {
private:
    bool zero_reg;

public:
    // WRITE to register
    virtual MemCell& operator=(word rhs) {
        // Register zero is non-writable
        if (not zero_reg) val_ = rhs;
        return *this;
    }
    Reg() {
        zero_reg = false;
        val_ = 0;
    }
    Reg(bool is_zero) {
        zero_reg = is_zero;
        val_ = 0;
    }
};


class Regfile {
private:
    const static byte REGFILE_SZ = 16;
    Reg Data[REGFILE_SZ];

public:
    // When initializing the register file, set index 0 as the zero register
    Regfile() { Data[0] = Reg(true); }

    Reg& operator[](byte addr) {
        assert(addr < REGFILE_SZ);
        return Data[addr];
    }
};

class Mem {
private:
    const static uint32_t MEM_SZ = 0x10000;
    MemCell Data[MEM_SZ];

public:
    virtual MemCell& operator[](word addr) { 
        return Data[addr];
    }
};

using Rom = Mem;

class Ram : public Mem {
private:
    // Pointers to the 4 memory-mapped GPIO ports
    MemCell *Port0, *Port1, *Port2, *Port3;

public:
    Ram(MemCell& p0, MemCell& p1, MemCell& p2, MemCell& p3) {
        Port0 = &p0;  Port1 = &p1;  Port2 = &p2;  Port3 = &p3;
    }
    
    virtual MemCell& operator[](word addr) { 
        // 0000-FEFF: RAM
        if (addr < 0xFF00) return Mem::operator[](addr);

        // FF00-FFFF: MMIO
        else if (addr == 0xFF00) return *Port0;
        else if (addr == 0xFF40) return *Port1;
        else if (addr == 0xFF80) return *Port2;
        else if (addr == 0xFFC0) return *Port3;
        else return Mem::operator[](addr); // Todo: print error
    }
};
