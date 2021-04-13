#include "CPU.h"

// Returns the value encoded between bit_left and bit_right (both included)
word CPU::extract_bitfield(word original, byte bit_left, byte bit_right) {
    assert(bit_left >= bit_right);
    word mask = (uint(1) << (bit_left+1 - bit_right)) - 1;
    return (original >> bit_right) & mask;
}


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
        result = A+B + C;
        true_result = uint32_t(A) + uint32_t(B) + uint32_t(C);
        break;
    case 0b111: // subb
        result = A-B - C;
        true_result = uint32_t(A) - uint32_t(B) - uint32_t(C);
        true_result = ~true_result; // borrow is the opposite of carry
        break;
    default:
        printf("Unreachable ALU funct: 0x%X\n", funct);
        exit(EXIT_FAILURE);
    }

    Z = not result;
    C = bool(true_result & 0x10000);
    V = (A&MSB == B&MSB) and (A&MSB != result&MSB);
    N = bool(result&MSB);

    // todo: remove assert
    assert(V == (A&MSB and B&MSB and not result&MSB) or (not A&MSB and not B&MSB and result&MSB));

    return result;
}

// Execute an ALU operation (operands in registers). Returns the used cycles
int CPU::exec_ALU_reg(word opcode) {
    bool immediate_mode = extract_bitfield(opcode, 11, 11);
    byte funct = extract_bitfield(opcode, 10, 8);
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);

    word argument = user_mode ? ram[PC] : rom_h[PC];

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

    word argument = user_mode ? ram[PC] : rom_h[PC];
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
    byte rD = extract_bitfield(opcode, 7, 4);
    byte rA = extract_bitfield(opcode, 3, 0);
    return 1;
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
        Z = not result;
        N = bool(result&MSB);
        // V and C are undefined
        break;

    case 0b11: // sra
        result = int16_t(regs[rA]) >> shamt;
        Z = not result;
        N = bool(result&MSB);
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
    return 1;
}

// Execute a jump. Returns the used cycles
int CPU::exec_JMP(word opcode) {
    return 1;
}

// Execute a call/ret operation. Returns the used cycles
int CPU::exec_CALL(word opcode) {
    return 1;
}

// Called whenever the CPU attempts to execute an illegal opcode
void CPU::ILLEGAL_OP(word opcode) {
    printf("Instruction opcode 0x%X not handled!\n", opcode);
    exit(EXIT_FAILURE);
}


// Reset CPU
void CPU::reset() {
    PC = 0x0000;
    user_mode = false;
    // todo: reset I/O counter
}

// Run CPU for a number of clock cycles
void CPU::execute(int32_t cycles) {
    while (cycles > 0) {
        // Fetch opcode
        word opcode = user_mode ? ram[PC++] : rom_h[PC];

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
