#pragma once

#include "includes.h"
#include "Memory.h"
#include "Terminal.h"
#include "Timer.h"

// Based on Dave Poo's 6502 emulator
class CPU {

private:

    // Flags / Status register
    struct StatusFlags {
        bool Z : 1; // Zero flag
        bool C : 1; // Carry flag
        bool V : 1; // Overflow flag
        bool S : 1; // Negative/Sign flag
    };



    word PC;                    // Program Counter
    Regfile regs;               // Register file
    Reg* const SP = &regs[1];   // Direct access to SP

    // Flags register
    union {
        byte FLG;
        CPU::StatusFlags Flags;
    };
    

    bool user_mode; // true if fetching from RAM, false if fetching from ROM
    bool increment_PC; // If set to false by an instruction, the PC won't be postincremented
    bool IRQ;

    // Direct access to the terminal for calling update()
    Terminal *terminal;

    // Input terminal
    Keyboard keyboard;
    // Output terminal
    Display display;
    // 16-bit timer
    Timer timer;

    // Memory banks: ROM (32 bit), RAM (16 bit)
    Rom rom_l, rom_h;
    Ram ram = Ram(keyboard, display, timer, /* Todo: add disk */ timer); 



    // HELPER FUNCTIONS

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

    // Equivalent to PC++, but if PC overflows an exception is thrown
    inline word PC_plus_1();

    // Returns the result of an ALU operation, given the funct bits and the 2 operands
    word ALU_result(byte funct, word A, word B);

    // Returns true if the jump condition is met and the jump has to be performed
    bool is_condition_met(byte cond);



    // MAIN INSTRUCTION FUNCTIONS

    // Execute a generic instruction. Returns the used cycles
    int exec_INSTR(word opcode);

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

    // Run CPU for a number of clock cycles. Instructions are atomic, the function  
    // returns how many extra cycles were needed to finish the last instruction.
    int32_t execute(int32_t cycles);

    // Called at regular intervals for updating the UI and getting input
    void update();

    // Write a 32-bit word in ROM, at a given address
    void write_ROM(word address, word data_high, word data_low);
};
