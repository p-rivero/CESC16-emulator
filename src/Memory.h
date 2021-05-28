#pragma once

#include "includes.h"

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



class Regfile {
private:
    const static byte REGFILE_SZ = 16;
    Reg Data[REGFILE_SZ];

public:
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
