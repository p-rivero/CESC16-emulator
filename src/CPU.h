#pragma once

#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <cstdint>
#include <ciso646> // Include this so vscode doesn't complain about alternative logical operators

#include "Terminal.h"

// Based on Dave Poo's 6502 emulator

using byte = uint8_t;
using word = uint16_t;

class CPU {

private:

    // HELPER STRUCTS

    struct Mem {
        const static uint32_t MEM_SZ = 0x10000;
        word Data[MEM_SZ];

        word& operator[](word addr) { return Data[addr]; }
    };

    struct Regfile {
        class Reg {
        public:
            Reg() { zero_reg = false; }
            Reg(bool is_zero) { zero_reg = is_zero; }
            operator int() const { return val_; }

            Reg& operator=(word rhs) {
                // Register zero is non-writable
                if (not zero_reg) val_ = rhs;
                return *this;
            }

        private:
            bool zero_reg;
            word val_ = 0;
        };

        const static byte REGFILE_SZ = 16;
        Reg Data[REGFILE_SZ];

        // When initializing the register file, set index 0 as the zero register
        Regfile() { Data[0] = Reg(true); }

        Reg& operator[](byte addr) {
            assert(addr < REGFILE_SZ);
            return Data[addr];
        }
    };

    // Flags / Status register
    struct StatusFlags {
        bool Z : 1; // Zero flag
        bool C : 1; // Carry flag
        bool V : 1; // Overflow flag
        bool S : 1; // Negative/Sign flag
    };



    word PC;                            // Program Counter
    Regfile regs;                       // Register file
    Regfile::Reg* const SP = &regs[1];  // Direct access to SP

    // Flags register
    union {
        byte FLG;
        CPU::StatusFlags Flags;
    };
    

    bool user_mode; // true if fetching from RAM, false if fetching from ROM
    bool increment_PC; // If set to false by an instruction, the PC won't be postincremented

    // Memory banks: ROM (32 bit), RAM (16 bit)
    Mem rom_l, rom_h, ram;

    // Terminal for input and output
    Terminal *terminal;



    // HELPER FUNCTIONS

    // Prints a formatted error message and terminates the program
    void ERROR(const char *msg, ...);
    
    // Returns the value encoded between bit_left and bit_right (both included)
    static inline word extract_bitfield(word original, byte bit_left, byte bit_right);

    // Returns the bit encoded in a given position (0 or 1)
    static inline byte extract_bit(word original, byte bit_pos);

    // Returns the argument pointed by the PC
    inline word fetch_argument();

    // Push some data into the stack
    inline void push(word data);

    // Pop some data from the stack
    inline word pop();

    // Returns the result of an ALU operation, given the funct bits and the 2 operands
    word ALU_result(byte funct, word A, word B);

    // Returns true if the jump condition is met and the jump has to be performed
    bool is_condition_met(byte cond);



    // MAIN INSTRUCTION FUNCTIONS

    // Execute an ALU operation (operands in registers). Returns the used cycles
    int exec_ALU_reg(word opcode);

    // Execute an ALU operation (operand in memory). Returns the used cycles
    int exec_ALU_m_op(word opcode);

    // Execute an ALU operation (destination in memory). Returns the used cycles
    int exec_ALU_m_dest(word opcode);

    // Execute a bit shift. Returns the used cycles
    int exec_SHFT(word opcode);

    // Execute a memory operation. Returns the used cycles
    int exec_MEM(word opcode);
    
    // Execute a jump. Returns the used cycles
    int exec_JMP(word opcode);

    // Execute a call/ret operation. Returns the used cycles
    int exec_CALL(word opcode);

    // Called whenever the CPU attempts to execute an illegal opcode
    void ILLEGAL_OP(word opcode);


public:
    const static word MSB = 0x8000;

    CPU() {
        terminal = Terminal::initialize();
    }
    ~CPU() {
        Terminal::destroy();
    }


    // Reset CPU
    void reset();

    // Run CPU for a number of clock cycles
    void execute(int32_t cycles);

    // Write a 32-bit word in ROM, at a given address
    void write_ROM(word address, word data_high, word data_low);
};
