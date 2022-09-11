#pragma once

#include "Globals.h"

class MemCell {
protected:
    word val_;

public:
    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator int() const;
};


class Reg : public MemCell {
private:
    bool zero_reg;

public:
    Reg();
    Reg(bool is_zero);
    // WRITE to register
    virtual MemCell& operator=(word rhs);
};

// Flags / Status register
struct StatusFlags {
    bool Z : 1; // Zero flag
    bool C : 1; // Carry flag
    bool V : 1; // Overflow flag
    bool S : 1; // Negative/Sign flag
};



class Regfile {
private:
    const static byte REGFILE_SZ = 16;
    Reg Data[REGFILE_SZ];

public:
    const char *ABI_names[16] = {"zero", "sp", "bp", "s0", "s1", "s2", "s3", "s4",
        "t0", "t1", "t2", "t3", "a0", "a1", "a2", "a3"};
    Reg& ABI_A0();
    
    Regfile();
    Reg& operator[](byte addr);
};


class Mem {
private:
    const static uint32_t MEM_SZ = 0x10000;
    MemCell Data[MEM_SZ];

public:
    virtual MemCell& operator[](word addr);
};


using Rom = Mem;


class Ram : public Mem {
private:
    // Pointers to the 4 memory-mapped GPIO ports
    MemCell *Port0, *Port1, *Port2, *Port3;

public:
    Ram(MemCell& p0, MemCell& p1, MemCell& p2, MemCell& p3);
    virtual MemCell& operator[](word addr);
};
