#include "CPU.h"

// Returns the value encoded between bit_left and bit_right (both included)
word CPU::extract_bitfield(word original, byte bit_left, byte bit_right) {
    assert(bit_left >= bit_right);
    word mask = (uint(1) << (bit_left+1 - bit_right)) - 1;
    return (original >> bit_right) & mask;
}

// Returns the argument pointed by the PC
word CPU::fetch_argument() {
    return user_mode ? ram[PC] : rom_l[PC];
}

// Push some data into the stack
void CPU::push(word data) {
    *SP = *SP - 1; // No -= operator
    ram[*SP] = data;
}

// Pop some data from the stack
word CPU::pop() {
    word data = ram[*SP];
    *SP = *SP + 1; // No += operator
    return data;
}


// Returns the result of an ALU operation, given the funct bits and the 2 operands
word CPU::ALU_result(byte funct, word A, word B) {
    if (funct == 0b000) return A;  // mov

    word result;
    uint32_t true_result;

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
        true_result = ~true_result; // borrow is the opposite of carry
        break;
    case 0b110: // addc
        result = A+B + Flags.C;
        true_result = uint32_t(A) + uint32_t(B) + uint32_t(Flags.C);
        break;
    case 0b111: // subb
        result = A-B - Flags.C;
        true_result = uint32_t(A) - uint32_t(B) - uint32_t(Flags.C);
        true_result = ~true_result; // borrow is the opposite of carry
        break;
    default:
        printf("Unreachable ALU funct: 0x%X\n", funct);
        exit(EXIT_FAILURE);
    }

    // Set flags
    Flags.Z = not result;
    Flags.C = bool(true_result & 0x10000);
    Flags.V = (A&MSB == B&MSB) and (A&MSB != result&MSB);
    Flags.S = bool(result&MSB);

    // todo: remove assert
    assert(Flags.V == (A&MSB and B&MSB and not result&MSB) or (not A&MSB and not B&MSB and result&MSB));

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
        return not Flags.Z;

    case 0b0011: // jc / jb / jnae
        return Flags.C;
    case 0b0100: // jnc / jnb / jae
        return not Flags.C;

    case 0b0101: // jo
        return Flags.V;
    case 0b0110: // jno
        return not Flags.V;

    case 0b0111: // js
        return Flags.S;
    case 0b1000: // jns
        return not Flags.S;

    case 0b1001: // jbe / jna
        return Flags.C or Flags.Z;
    case 0b1010: // ja / jnbe
        return not (Flags.C or Flags.Z);

    case 0b1011: // jl / jnge
        return Flags.V != Flags.S;
    case 0b1100: // jle / jng
        return (Flags.V != Flags.S) or Flags.Z;

    case 0b1101: // jg / jnle
        return (Flags.V == Flags.S) and not Flags.Z;
    case 0b1110: // jge / jnl
        return Flags.V == Flags.S;
    
    default:
        printf("Invalid jump condition: 0x%X\n", cond);
        exit(EXIT_FAILURE);
        break;
    }
}


// Execute an ALU operation (operands in registers). Returns the used cycles
int CPU::exec_ALU_reg(word opcode) {
    bool immediate_mode = extract_bitfield(opcode, 11, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);

    word argument = fetch_argument();

    if (immediate_mode)
        regs[rD] = ALU_result(funct, regs[rA], argument);
    else {
        byte rB = extract_bitfield(argument, 3, 0);
        regs[rD] = ALU_result(funct, regs[rA], regs[rB]);
    }

    return 3;
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
    else {
        // Indexed addressing: OP rD, [rA+imm]
        regs[rD] = ALU_result(funct, regs[rD], ram[regs[rA] + argument]);
        cycles = 5;
    }

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
    else {
        // Indexed addressing: OP rD, [rA+imm]
        byte rB = extract_bitfield(opcode, 7, 4);
        word address = regs[rA] + argument;
        ram[address] = ALU_result(funct, ram[address], regs[rA]);
        cycles = 5;
    }

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
        Flags.Z = not result;
        Flags.S = bool(result&MSB);
        // V and C are undefined
        break;

    case 0b11: // sra
        result = int16_t(regs[rA]) >> shamt;
        Flags.Z = not result;
        Flags.S = bool(result&MSB);
        // V and C are undefined
        break;
    
    default:
        printf("Unreachable shift op: 0x%X\n", op);
        exit(EXIT_FAILURE);
        break;
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
        word data = ram[regs[rA] + argument];
        if (extract_bitfield(data, 7, 7)) data |= 0xFF00; // Sign extend
        regs[rD] = data;
        return 3;
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
        assert(FLG & 0xF0 == 0); // Top bits should be 0
        return 3;
    
    default:
        ILLEGAL_OP(opcode);
        break;
    }

    printf("Unreachable code! Opcode = 0x%X\n", opcode);
    exit(EXIT_FAILURE);
    return 0;
}

// Execute a jump. Returns the used cycles
int CPU::exec_JMP(word opcode) {
    byte cond = extract_bitfield(opcode, 11, 8);

    if (not is_condition_met(cond)) return 2; // Jump is not taken

    // Jump is taken
    if (extract_bitfield(opcode, 12, 12) == 0) {
        // Jump to address in register
        byte rA = extract_bitfield(opcode, 3, 0);
        PC = regs[rA];
    }
    else {
        // Jump to immediate address
        PC = fetch_argument();
    }
    PC--; // Workaround due to the PC being incremented on each instruction
    return 2;
}

// Execute a call/ret operation. Returns the used cycles
int CPU::exec_CALL(word opcode) {
    return 1;
}

// Called whenever the CPU attempts to execute an illegal opcode
void CPU::ILLEGAL_OP(word opcode) {
    printf("\n");
    printf("Instruction 0x%X not handled!\n", opcode);
    exit(EXIT_FAILURE);
}


// Reset CPU
void CPU::reset() {
    PC = 0x0000;
    user_mode = false;
    // Initialize ROM as garbage
    for (int i = 0; i < 0x10000; i++) {
        rom_h[i] = i << (i%16);
        rom_l[i] = i;
    }
    // todo: reset I/O counter
}

// Run CPU for a number of clock cycles
void CPU::execute(int32_t cycles) {
    while (cycles > 0) {
        // Fetch opcode
        word opcode = user_mode ? ram[PC++] : rom_h[PC];

        // Todo: find a better way to do this
        printf("Executing opcode %X\n", extract_bitfield(opcode, 15, 8));

        // Decode instruction
        switch (extract_bitfield(opcode, 15, 13)) {
        case 0b000:
            // ALU or shift operation
            if (extract_bitfield(opcode, 12, 12) == 0) {
                // 0000... -> ALU op
                cycles -= exec_ALU_reg(opcode);
            }
            else {
                // 0001... -> sll
                cycles -= exec_SHFT(opcode);
            }
            break;

        case 0b001:
            // Shift operation
            cycles -= exec_SHFT(opcode);
            break;

        case 0b010:
            // ALU operation (operand in memory)
            cycles -= exec_ALU_m_op(opcode);
            break;

        case 0b011:
            // ALU operation (destination in memory)
            cycles -= exec_ALU_m_dest(opcode);
            break;

        case 0b100:
            // Memory operation
            cycles -= exec_MEM(opcode);
            break;

        case 0b110:
            // Jump
            cycles -= exec_JMP(opcode);
            break;

        case 0b111:
            // Call/ret opcode
            cycles -= exec_CALL(opcode);
            break;
        
        default:
            // Illegal opcode
            ILLEGAL_OP(opcode);
            break;
        }

        // Increment PC at the end of instruction
        PC++;
    }
}
