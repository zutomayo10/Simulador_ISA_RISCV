#include "cpu.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <stdexcept>

static const char* REG_NAMES[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

CPU::CPU(Memory& m) : pc(0), halted(false), exitCode(0), mem(m) {
    std::memset(regs, 0, sizeof(regs));
    regs[2] = 0x7FFFFFFC;
}

void CPU::reset() {
    std::memset(regs, 0, sizeof(regs));
    regs[2] = 0x7FFFFFFC;
    pc       = 0;
    halted   = false;
    exitCode = 0;
}

const char* CPU::regName(int n) {
    return (n >= 0 && n < 32) ? REG_NAMES[n] : "??";
}

int CPU::regIndex(const std::string& name) {
    if (name.size() >= 2 && (name[0] == 'x' || name[0] == 'X')) {
        try {
            int n = std::stoi(name.substr(1));
            if (n >= 0 && n < 32) return n;
        } catch (...) {}
    }
    for (int i = 0; i < 32; ++i)
        if (name == REG_NAMES[i]) return i;
    return -1;
}

void CPU::handleEcall() {
    uint32_t syscall = getReg(17);
    switch (syscall) {
    case 1:
        std::cout << (int32_t)getReg(10);
        std::cout.flush();
        break;
    case 4: {
        uint32_t addr = getReg(10);
        char c;
        while ((c = (char)mem.read8(addr++)) != '\0')
            std::cout << c;
        std::cout.flush();
        break;
    }
    case 5: {
        int32_t val = 0;
        std::cin >> val;
        setReg(10, (uint32_t)val);
        break;
    }
    case 10:
        halted   = true;
        exitCode = (int)(int32_t)getReg(10);
        std::cout << "\nPrograma finalizado (ecall 10). Codigo de salida: " << exitCode << "\n";
        break;
    case 11:
        std::cout << (char)getReg(10);
        std::cout.flush();
        break;
    case 17:
        halted   = true;
        exitCode = (int)(int32_t)getReg(10);
        std::cout << "\nPrograma finalizado (ecall 17). Codigo de salida: " << exitCode << "\n";
        break;
    default:
        std::cerr << "[ecall " << syscall << " no implementado]\n";
        break;
    }
}

void CPU::step() {
    if (halted) {
        std::cout << "La CPU esta detenida. Use 'reset' para reiniciar.\n";
        return;
    }

    uint32_t instr  = mem.read32(pc);
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7)  & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;
    uint32_t shamt  = (instr >> 20) & 0x1F;


    int32_t immI = (int32_t)instr >> 20;

    int32_t immS = ((int32_t)(instr & 0xFE000000) >> 20) |
                   ((instr >> 7) & 0x1F);

    int32_t immB = ((int32_t)(instr & 0x80000000) >> 19) |
                   ((instr & 0x00000080) << 4)            |
                   ((instr >> 20) & 0x7E0)                |
                   ((instr >> 7)  & 0x1E);

    uint32_t immU = instr & 0xFFFFF000;

    int32_t immJ = ((int32_t)(instr & 0x80000000) >> 11) |
                   (instr & 0x000FF000)                   |
                   ((instr >> 9)  & 0x00000800)           |
                   ((instr >> 20) & 0x000007FE);

    uint32_t nextPC = pc + 4;

    switch (opcode) {

  
    case 0x03: {
        uint32_t addr = getReg(rs1) + (uint32_t)immI;
        switch (funct3) {
        case 0x0: setReg(rd, (uint32_t)(int32_t)(int8_t) mem.read8 (addr)); break; // lb
        case 0x1: setReg(rd, (uint32_t)(int32_t)(int16_t)mem.read16(addr)); break; // lh
        case 0x2: setReg(rd, mem.read32(addr)); break;                              // lw
        case 0x4: setReg(rd, (uint32_t)mem.read8 (addr)); break;                   // lbu
        case 0x5: setReg(rd, (uint32_t)mem.read16(addr)); break;                   // lhu
        default: throw std::runtime_error("funct3 desconocido en LOAD");
        }
        break;
    }

    // OP-IMM
    case 0x13: {
        switch (funct3) {
        case 0x0: setReg(rd, getReg(rs1) + (uint32_t)immI); break; // addi
        case 0x1: setReg(rd, getReg(rs1) << shamt); break;         // slli
        case 0x2: setReg(rd, (int32_t)getReg(rs1) < immI ? 1u : 0u); break; // slti
        case 0x3: setReg(rd, getReg(rs1) < (uint32_t)immI ? 1u : 0u); break; // sltiu
        case 0x4: setReg(rd, getReg(rs1) ^ (uint32_t)immI); break; // xori
        case 0x5:
            if (funct7 == 0x00)
                setReg(rd, getReg(rs1) >> shamt);                          // srli
            else
                setReg(rd, (uint32_t)((int32_t)getReg(rs1) >> shamt));    // srai
            break;
        case 0x6: setReg(rd, getReg(rs1) | (uint32_t)immI); break; // ori
        case 0x7: setReg(rd, getReg(rs1) & (uint32_t)immI); break; // andi
        }
        break;
    }


    case 0x17:
        setReg(rd, pc + immU);
        break;

    // STORE
    case 0x23: {
        uint32_t addr = getReg(rs1) + (uint32_t)immS;
        switch (funct3) {
        case 0x0: mem.write8 (addr, (uint8_t) getReg(rs2)); break; // sb
        case 0x1: mem.write16(addr, (uint16_t)getReg(rs2)); break; // sh
        case 0x2: mem.write32(addr, getReg(rs2)); break;           // sw
        default: throw std::runtime_error("funct3 desconocido en STORE");
        }
        break;
    }

    //OP (R-type)
    case 0x33: {
        switch (funct3) {
        case 0x0:
            if (funct7 == 0x00) setReg(rd, getReg(rs1) + getReg(rs2)); // add
            else                setReg(rd, getReg(rs1) - getReg(rs2)); // sub
            break;
        case 0x1: setReg(rd, getReg(rs1) << (getReg(rs2) & 0x1F)); break;  // sll
        case 0x2: setReg(rd, (int32_t)getReg(rs1) < (int32_t)getReg(rs2) ? 1u : 0u); break; // slt
        case 0x3: setReg(rd, getReg(rs1) < getReg(rs2) ? 1u : 0u); break;  // sltu
        case 0x4: setReg(rd, getReg(rs1) ^ getReg(rs2)); break;             // xor
        case 0x5:
            if (funct7 == 0x00)
                setReg(rd, getReg(rs1) >> (getReg(rs2) & 0x1F));                        // srl
            else
                setReg(rd, (uint32_t)((int32_t)getReg(rs1) >> (getReg(rs2) & 0x1F)));  // sra
            break;
        case 0x6: setReg(rd, getReg(rs1) | getReg(rs2)); break; // or
        case 0x7: setReg(rd, getReg(rs1) & getReg(rs2)); break; // and
        }
        break;
    }

    //LUI
    case 0x37:
        setReg(rd, immU);
        break;


    case 0x63: {
        bool taken = false;
        switch (funct3) {
        case 0x0: taken = getReg(rs1) == getReg(rs2); break; // beq
        case 0x1: taken = getReg(rs1) != getReg(rs2); break; // bne
        case 0x4: taken = (int32_t)getReg(rs1) < (int32_t)getReg(rs2); break;  // blt
        case 0x5: taken = (int32_t)getReg(rs1) >= (int32_t)getReg(rs2); break; // bge
        case 0x6: taken = getReg(rs1) < getReg(rs2); break;  // bltu
        case 0x7: taken = getReg(rs1) >= getReg(rs2); break; // bgeu
        default: throw std::runtime_error("funct3 desconocido en BRANCH");
        }
        if (taken) nextPC = pc + (uint32_t)immB;
        break;
    }

    
    case 0x67: {
        uint32_t target = (getReg(rs1) + (uint32_t)immI) & ~1u;
        setReg(rd, nextPC);
        nextPC = target;
        break;
    }

    
    case 0x6F: {
        setReg(rd, nextPC);
        nextPC = pc + (uint32_t)immJ;
        break;
    }

  
    case 0x73: {
        if (funct3 == 0) {
            uint32_t imm12 = instr >> 20;
            if (imm12 == 0) {
                handleEcall();
            } else if (imm12 == 1) { // ebreak
                halted = true;
                std::ostringstream s;
                s << "0x" << std::hex << std::setw(8) << std::setfill('0') << pc;
                std::cout << "ebreak: CPU detenida en PC=" << s.str() << "\n";
                return;
            }
        }
        break;
    }

    
    case 0x0F:
        break;

    default: {
        std::ostringstream err;
        err << "Opcode desconocido 0x" << std::hex << opcode
            << " (instruccion 0x" << std::setw(8) << std::setfill('0') << instr
            << " en PC=0x" << std::setw(8) << std::setfill('0') << pc << ")";
        throw std::runtime_error(err.str());
    }
    }

    pc = nextPC;
}
