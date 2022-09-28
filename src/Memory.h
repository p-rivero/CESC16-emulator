#pragma once

#include "Globals.h"

#include <string>
#include <array>

class MemCell {
protected:
    word val_ = 0;

public:
    virtual ~MemCell() = default;
    // WRITE
    virtual MemCell& operator=(word rhs);
    // READ
    virtual operator word() const;
};


class Reg : public MemCell {
private:
    bool zero_reg = false;

public:
    Reg() = default;
    explicit Reg(bool is_zero);
    // WRITE to register
    MemCell& operator=(word rhs) override;
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
    std::array<Reg,REGFILE_SZ> Data;

public:
    const std::array<std::string,REGFILE_SZ> ABI_names = {"zero", "sp", "bp", "s0", "s1", "s2", "s3", "s4",
        "t0", "t1", "t2", "t3", "a0", "a1", "a2", "a3"};
    Reg& ABI_A0();
    
    Regfile();
    Reg& operator[](byte addr);
};


class Mem {
private:
    const static uint32_t MEM_SZ = 0x10000;
    std::array<MemCell,MEM_SZ> Data;

public:
    virtual ~Mem() = default;
    virtual MemCell& operator[](word addr);
};


using Rom = Mem;


class Ram : public Mem {
private:
    // Pointers to the 4 memory-mapped GPIO ports
    MemCell *Port0;
    MemCell *Port1;
    MemCell *Port2;
    MemCell *Port3;

public:
    Ram(MemCell& p0, MemCell& p1, MemCell& p2, MemCell& p3);
    MemCell& operator[](word addr) override;
};
