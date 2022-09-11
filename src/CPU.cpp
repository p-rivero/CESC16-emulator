#include "CPU.h"
#include "Exceptions/EmulatorException.h"
#include "Exceptions/IllegalOpcodeException.h"
#include "Utilities/Assert.h"
#include "Utilities/ExitHelper.h"


// Returns the value encoded between bit_left and bit_right (both included)
word CPU::extract_bitfield(word original, byte bit_left, byte bit_right) {
    assert(bit_left >= bit_right);
    word mask = (uint(1) << (bit_left+1 - bit_right)) - 1;
    return (original >> bit_right) & mask;
}

// Returns the bit encoded in a given position (0 or 1)
byte CPU::extract_bit(word original, byte bit_pos) {
    return extract_bitfield(original, bit_pos, bit_pos);
}

// Returns the argument pointed by the PC
word CPU::fetch_argument() {
    return user_mode ? ram[PC] : rom_l[PC];
}

// Push some data into the stack
void CPU::push(word data) {
    *SP = *SP - 1; // No -= operator
    if (*SP == 0xFFFF) throw EmulatorException("SP overflowed");
    ram[*SP] = data;
}

// Pop some data from the stack
word CPU::pop() {
    word data = ram[*SP];
    *SP = *SP + 1; // No += operator
    if (*SP == 0x0000) throw EmulatorException("SP overflowed");
    return data;
}

// Equivalent to ++PC, but if PC overflows an exception is thrown
word CPU::PC_plus_1() {
    PC++;
    if (PC == 0x0000) throw EmulatorException("PC overflowed");
    return PC;
}


// Returns the result of an ALU operation, given the funct bits and the 2 operands
word CPU::ALU_result(byte funct, word A, word B) {
    if (funct == 0b000) return B;  // mov

    word result;
    // true_result is used to detect overflow. Undefined in logical operations
    uint32_t true_result = -1;

    switch (funct) {
    case 0b001: result = A&B; break; // and
    case 0b010: result = A|B; break; // or
    case 0b011: result = A^B; break; // xor
    case 0b100: // add
        result = A+B;
        true_result = uint32_t(A) + uint32_t(B);
        break;
    case 0b101: // sub
        result = A-B;
        true_result = uint32_t(A) - uint32_t(B);
        B = ~B; // Invert second operand for overflow computation
        break;
    case 0b110: // addc
        result = A+B + Flags.C;
        true_result = uint32_t(A) + uint32_t(B) + uint32_t(Flags.C);
        break;
    case 0b111: // subb
        result = A-B - Flags.C;
        true_result = uint32_t(A) - uint32_t(B) - uint32_t(Flags.C);
        B = ~B; // Invert second operand for overflow computation
        break;
    default:
        throw EmulatorException("Unreachable ALU funct!");
    }

    // Set flags
    Flags.Z = (result == 0x0000);
    Flags.C = bool(true_result & 0x10000);
    Flags.V = ((A&MSB) == (B&MSB)) && ((A&MSB) != (result&MSB));
    Flags.S = bool(result&MSB);
    // No need to invert carry on sub, in case of borrow the upper bits of true_result are 0xFFFF

    return result;
}

// Returns true if the jump condition is met and the jump has to be performed
bool CPU::is_condition_met(byte cond) {
    switch (cond) {
    case 0b0000: // jmp
        return true;

    case 0b0001: // jz / je
        return Flags.Z;
    case 0b0010: // jnz / jne
        return !Flags.Z;

    case 0b0011: // jc / jb / jnae
        return Flags.C;
    case 0b0100: // jnc / jnb / jae
        return !Flags.C;

    case 0b0101: // jo
        return Flags.V;
    case 0b0110: // jno
        return !Flags.V;

    case 0b0111: // js
        return Flags.S;
    case 0b1000: // jns
        return !Flags.S;

    case 0b1001: // jbe / jna
        return Flags.C || Flags.Z;
    case 0b1010: // ja / jnbe
        return !(Flags.C || Flags.Z);

    case 0b1011: // jl / jnge
        return Flags.V != Flags.S;
    case 0b1100: // jle / jng
        return (Flags.V != Flags.S) || Flags.Z;

    case 0b1101: // jg / jnle
        return (Flags.V == Flags.S) && !Flags.Z;
    case 0b1110: // jge / jnl
        return Flags.V == Flags.S;
    
    default:
        throw EmulatorException("Invalid jump condition: " + std::to_string(cond));
    }
    
    // Unreachable
    assert(false);
}

// Returns true if the OS is ready to be interrupted (handlers have been initialized)
bool CPU::is_OS_ready() {
    // 1. We suppose that all the critical work is done on the first instructions
    // 2. This extra protection layer isn't present in the real CPU. If strict_flg is set, return true
    return Globals::strict_flg || (PC >= Globals::OS_critical_instr);
}

bool CPU::is_breakpoint(const std::vector<word>& breakpoints) {
    for (int i = 0; i < breakpoints.size(); i++)
        if (PC == breakpoints[i]) return true;

    return false;
}



int CPU::exec_INSTR(word opcode) {

    int used_cycles;
    increment_PC = true;

    // Decode instruction
    switch (extract_bitfield(opcode, 15, 13)) {
    case 0b000:
        // ALU or shift operation
        if (extract_bit(opcode, 12) == 0) {
            // 0000... -> ALU op
            used_cycles = exec_ALU_reg(opcode);
        }
        else {
            // 0001... -> sll
            used_cycles = exec_SHFT(opcode);
        }
        break;

    case 0b001:
        // Shift operation
        used_cycles = exec_SHFT(opcode);
        break;

    case 0b010:
        // ALU operation (operand in memory)
        used_cycles = exec_ALU_m_op(opcode);
        break;

    case 0b011:
        // ALU operation (destination in memory)
        used_cycles = exec_ALU_m_dest(opcode);
        break;

    case 0b100:
        // ALU operation (destination in memory, immediate operand)
        used_cycles = exec_ALU_mem_imm(opcode);
        break;
    
    case 0b101:
        // Memory operation
        used_cycles = exec_MEM(opcode);
        break;

    case 0b110:
        // Jump
        used_cycles = exec_JMP(opcode);
        break;

    case 0b111:
        // Call/ret opcode
        used_cycles = exec_CALL(opcode);
        break;
    
    default:
        throw EmulatorException("Unreachable opcode: " + std::to_string(opcode));
    }

    // Increment PC at the end of instruction
    if (increment_PC) PC_plus_1();

    return used_cycles;
}

// Execute an ALU operation (operands in registers). Returns the used cycles
int CPU::exec_ALU_reg(word opcode) {
    bool immediate_mode = extract_bit(opcode, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);

    bool is_mov = (funct == 0b000);

    word argument = fetch_argument();

    if (immediate_mode)
        regs[rD] = ALU_result(funct, regs[rA], argument);
    else {
        byte rB = extract_bitfield(argument, 3, 0);
        // mov register to register uses rA instead of rB
        if (is_mov) rB = rA;

        regs[rD] = ALU_result(funct, regs[rA], regs[rB]);
    }

    return is_mov ? 2 : 3;
}

// Execute an ALU operation (operand in memory). Returns the used cycles
int CPU::exec_ALU_m_op(word opcode) {
    byte addr_mode = extract_bitfield(opcode, 12, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);

    word argument = fetch_argument();
    int cycles = 4;

    if (addr_mode == 0b00) {
        // Direct addressing: OP rD, rA, [imm]
        regs[rD] = ALU_result(funct, regs[rA], ram[argument]);
    }
    else if (addr_mode == 0b01) {
        // Indirect addressing: OP rD, rA, [rB]
        byte rB = extract_bitfield(argument, 3, 0);
        regs[rD] = ALU_result(funct, regs[rA], ram[regs[rB]]);
    }
    else if (addr_mode == 0b10) {
        // Indexed addressing: OP rD, [rA+imm]
        word address = regs[rA] + argument;
        regs[rD] = ALU_result(funct, regs[rD], ram[address]);
        cycles = 5;
    }
    else /* addr_mode == 0b11 */ {
        // Indexed addressing: OP rD, [rA+rB]
        byte rB = extract_bitfield(argument, 3, 0);
        word address = regs[rA] + regs[rB];
        regs[rD] = ALU_result(funct, regs[rD], ram[address]);
        cycles = 5;
    }
    if (funct == 0b000) cycles = 3; // mov always takes 3 cycles

    return cycles;
}

// Execute an ALU operation (destination in memory). Returns the used cycles
int CPU::exec_ALU_m_dest(word opcode) {
    byte addr_mode = extract_bitfield(opcode, 12, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    byte rA = extract_bitfield(opcode, 3, 0);

    word argument = fetch_argument();
    int cycles = 4;

    if (addr_mode == 0b00) {
        // Direct addressing: OP [imm], rA
        ram[argument] = ALU_result(funct, ram[argument], regs[rA]);
    }
    else if (addr_mode == 0b01) {
        // Indirect addressing: OP [rA], rB
        byte rB = extract_bitfield(argument, 3, 0);
        ram[regs[rA]] = ALU_result(funct, ram[regs[rA]], regs[rB]);
    }
    else if (addr_mode == 0b10) {
        // Indexed addressing: OP [rA+imm], rB
        byte rB = extract_bitfield(opcode, 7, 4);
        word address = regs[rA] + argument;
        ram[address] = ALU_result(funct, ram[address], regs[rB]);
        cycles = 5;
    }
    else /* addr_mode == 0b11 */ {
        // Indexed addressing: OP [rA+rC], rB
        byte rB = extract_bitfield(opcode, 7, 4);
        byte rC = extract_bitfield(argument, 3, 0);
        word address = regs[rA] + regs[rC];
        ram[address] = ALU_result(funct, ram[address], regs[rB]);
        cycles = 5;
    }
    if (funct == 0b000) cycles--; // mov takes 3 cycles (4 cycles in indexed mode)

    return cycles;
}

// Execute an ALU operation (destination in memory). Returns the used cycles
int CPU::exec_ALU_mem_imm(word opcode) {
    byte addr_mode = extract_bitfield(opcode, 12, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    word imm4 = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);

    word argument = fetch_argument();
    int cycles = 4;

    if (addr_mode == 0b00) {
        // Direct addressing: OP [Addr16], imm4
        ram[argument] = ALU_result(funct, ram[argument], imm4);
        cycles = 5; // Direct addressing is implemented as indexed
    }
    else if (addr_mode == 0b01) {
        // Indirect addressing: OP [rA], Imm16
        ram[regs[rA]] = ALU_result(funct, ram[regs[rA]], argument);
    }
    else if (addr_mode == 0b10) {
        // Indexed addressing: OP [rA+imm], imm4
        word address = regs[rA] + argument;
        ram[address] = ALU_result(funct, ram[address], imm4);
        cycles = 5;
    }
    else /* addr_mode == 0b11 */ {
        // Indexed addressing: OP [rA+rC], imm4
        byte rC = extract_bitfield(argument, 3, 0);
        word address = regs[rA] + regs[rC];
        ram[address] = ALU_result(funct, ram[address], imm4);
        cycles = 5;
    }
    if (funct == 0b000) cycles--; // mov takes 3 cycles (4 cycles in indexed mode)

    return cycles;
}

// Execute a bit shift. Returns the used cycles
int CPU::exec_SHFT(word opcode) {
    byte op = extract_bitfield(opcode, 13, 12);
    byte shamt = extract_bitfield(opcode, 11, 8);
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);
    word result;
    uint32_t true_result;

    switch (op) {
    case 0b01: // sll
        result = regs[rA];
        for (byte i = 0; i < shamt; i++)
            result = ALU_result(0b100, result, result);
        break;

    case 0b10: // srl
        result = uint16_t(regs[rA]) >> shamt;
        Flags.Z = (result == 0);
        Flags.S = bool(result&MSB);
        // V and C are undefined
        break;

    case 0b11: // sra
        result = int16_t(regs[rA]) >> shamt;
        Flags.Z = (result == 0);
        Flags.S = bool(result&MSB);
        // V and C are undefined
        break;
    
    default:
        throw EmulatorException("Unreachable shift op: " + std::to_string(op));
    }
    
    regs[rD] = result;

    return shamt+1;
}

// Execute a memory operation. Returns the used cycles
int CPU::exec_MEM(word opcode) {
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);
    word argument = fetch_argument();

    switch (extract_bitfield(opcode, 12, 8)) {
    case 0b00000: { // movb
        throw EmulatorException("MOVB is deprecated and cannot be used");
    }
    case 0b00001: { // swap
        word address = regs[rA] + argument;
        word temp = regs[rD];
        regs[rD] = ram[address];
        ram[address] = temp;
        return 5;
    }
    case 0b00010: // peek (LSB/argument)
        regs[rD] = rom_l[regs[rA] + argument];
        return 3;
    case 0b00011: // peek (MSB/opcode)
        regs[rD] = rom_h[regs[rA] + argument];
        return 3;

    case 0b00100: { // push (reg)
        assert(rA == 0b0001);
        byte rB = extract_bitfield(argument, 3, 0);
        push(regs[rB]);
        return 3;
    }
    case 0b00101: // push (imm)
        assert(rA == 0b0001);
        push(argument);
        return 3;
    case 0b00110: // pushf
        assert(rA == 0b0001);
        push(FLG);
        return 3;
        
    case 0b00111: // pop
        assert(rA == 0b0001);
        regs[rD] = pop();
        return 3;
    case 0b01000: // popf
        assert(rA == 0b0001);
        FLG = pop();
        assert((FLG & 0xF0) == 0); // Top bits should be 0
        return 3;
    
    default:
        throw IllegalOpcodeException();
    }

    // Unreachable
    assert(false);
    return 0;
}

// Execute a jump. Returns the used cycles
int CPU::exec_JMP(word opcode) {
    byte cond = extract_bitfield(opcode, 11, 8);

    if (!is_condition_met(cond)) return 2; // Jump is not taken

    // Jump is taken
    if (extract_bit(opcode, 12) == 0) {
        // Jump to address in register
        byte rA = extract_bitfield(opcode, 3, 0);
        PC = regs[rA];
    }
    else {
        // Jump to immediate address
        PC = fetch_argument();
    }
    // do not increment the PC after executing this instruction
    increment_PC = false;
    
    return 2;
}

// Execute a call/ret operation. Returns the used cycles
int CPU::exec_CALL(word opcode) {
    word argument = fetch_argument();
    byte rB = extract_bitfield(argument, 3, 0);

    word destination;
    if (extract_bit(opcode, 8) == 0) destination = regs[rB]; // REG variant
    else destination = argument; // IMM variant

    // do not increment the PC after executing this instruction
    increment_PC = false;

    int cycles = 4;

    switch (extract_bitfield(opcode, 12, 9)) {
    case 0b0000: // call
        assert(extract_bitfield(opcode, 3, 0) == 0b0001);
        push(PC_plus_1());
        PC = destination;
        break;
        
    case 0b0001: // syscall
        assert(extract_bitfield(opcode, 3, 0) == 0b0001);
        push(PC_plus_1());
        PC = destination;
        user_mode = false;
        break;

    case 0b0010: // enter
        assert(extract_bitfield(opcode, 3, 0) == 0b0001);
        push(PC_plus_1());
        PC = destination;
        user_mode = true;
        break;

    case 0b0011:
        if (extract_bit(opcode, 8) == 0) {
            assert(extract_bitfield(opcode, 3, 0) == 0b0001);
            // 0b00110 -> ret
            PC = pop();
            cycles = 3;
        }
        else {
            assert(extract_bitfield(opcode, 3, 0) == 0b0001);
            // 0b00111 -> sysret
            PC = pop();
            user_mode = true;
            cycles = 3;
        }
        break;
    
    case 0b0100: // exit
        if (extract_bit(opcode, 8) == 0) {
            assert(extract_bitfield(opcode, 3, 0) == 0b0001);
            // 0b01000 -> exit
            PC = pop();
            user_mode = false;
            cycles = 3;
        }
        else {
            // 0b01001 -> illegal
            throw IllegalOpcodeException();
        }
        break;
    
    default:
        throw IllegalOpcodeException();
    }

    return cycles;
}




// Reset CPU
void CPU::reset() {
    PC = 0x0000;
    user_mode = false;
    IRQ = false;
    timer.reset();
    Globals::elapsed_cycles = 0;
}

// Run CPU for a number of clock cycles. Instructions are atomic, the function  
// returns how many extra cycles were needed to finish the last instruction.
int32_t CPU::execute(int32_t cycles) {

    while (cycles > 0) {
        int used_cycles;
        word old_PC = PC;
        
        // CPU INTERRUPT! Jump to interrupt vector (0x0011 if in RAM, 0x0013 if in ROM)
        if (IRQ && is_OS_ready()) try {
            push(PC);
            PC = user_mode ? 0x0011 : 0x0013;
            user_mode = false;  // Jump to ROM
            used_cycles = 3;    // Takes 3 clock cycles in both cases
            IRQ = false;
            if (timer.tick(used_cycles)) IRQ = true; // If an overflow occurs, trigger interrupt
        }
        catch (EmulatorException& e) {
            ExitHelper::error("Error while processing interrupt:\n%s\n", e.what());
        }

        // EXECUTE INSTRUCTION NORMALLY
        else try {
            word opcode = user_mode ? ram[PC] : rom_h[PC];
            if (user_mode) PC_plus_1();
            used_cycles = exec_INSTR(opcode);
            if (timer.tick(used_cycles)) IRQ = true; // If an overflow occurs, trigger interrupt
        }
        catch (EmulatorException& e) {
            if (user_mode) {
                ExitHelper::error(
                    "Error at PC = 0x%04X [RAM] (OP = 0x%04X, ARG = 0x%04X):\n%s\n",
                    old_PC, uint(ram[old_PC]), uint(ram[old_PC+1]), e.what()
                );
            }
            else {
                ExitHelper::error(
                    "Error at PC = 0x%04X [ROM] (OP = 0x%04X, ARG = 0x%04X):\n%s\n",
                    old_PC, uint(rom_h[old_PC]), uint(rom_l[old_PC]), e.what()
                );
            }
        }

        // Add CPI info
        cpi_mean.addDataPoint(used_cycles);

        // Decrement the remaining cycles
        cycles -= used_cycles;
        
        // Increment global count to be displayed
        Globals::elapsed_cycles = Globals::elapsed_cycles + used_cycles;
        // (+= is deprecated for volatile variables)
        
        // Check if we landed on an exit point
        if (is_breakpoint(Globals::exitpoints)) {
            // The exit code is the value stored in a0
            int exit_code = regs.ABI_A0();
            
            if (exit_code > 0xFF) {
                ExitHelper::exitCode(
                    exit_code & 0xFF,
                    "Warning: the exit code 0x%X is bigger than 255 and will be truncated\n", exit_code
                );
            }
            else {
                ExitHelper::exitCode(exit_code, "");
            }
        }
        
        // Check if we landed on a breakpoint
        if (Globals::single_step || is_breakpoint(Globals::breakpoints)) {
            Globals::is_paused = true;
            return 0;
        }
    }

    // If finishing an instruction took some extra cycles, return how many
    return -cycles;
}

// Called at regular intervals for updating the UI and getting input
void CPU::update() {
    // Flush the output stream
    terminal->display_status(PC, user_mode, Flags, regs, cpi_mean.getCurrentMean());
    terminal->flush();

    // If a new key has been pressed, trigger interrupt
    if (keyboard.update()) IRQ = true;
}


// Write a 32-bit word to the upper and lower bits of ROM, at a given address
void CPU::write_ROM(word address, word data_high, word data_low) {
    rom_h[address] = data_high;
    rom_l[address] = data_low;
}
