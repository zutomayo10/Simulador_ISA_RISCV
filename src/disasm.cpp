#include "disasm.h"
#include <sstream>
#include <iomanip>

static const char* REG_NAMES[32] = {
    "zero","ra","sp","gp","tp","t0","t1","t2",
    "s0","s1","a0","a1","a2","a3","a4","a5",
    "a6","a7","s2","s3","s4","s5","s6","s7",
    "s8","s9","s10","s11","t3","t4","t5","t6"
};

const char* regABIName(int n) {
    return (n >= 0 && n < 32) ? REG_NAMES[n] : "??";
}

static std::string hex(uint32_t v) {
    std::ostringstream o;
    o << "0x" << std::hex << v;
    return o.str();
}

std::string disassemble(uint32_t addr, uint32_t instr) {
    uint32_t opcode = instr & 0x7F;
    uint32_t rd     = (instr >> 7)  & 0x1F;
    uint32_t funct3 = (instr >> 12) & 0x7;
    uint32_t rs1    = (instr >> 15) & 0x1F;
    uint32_t rs2    = (instr >> 20) & 0x1F;
    uint32_t funct7 = (instr >> 25) & 0x7F;
    uint32_t shamt  = (instr >> 20) & 0x1F;

    int32_t immI = (int32_t)instr >> 20;
    int32_t immS = ((int32_t)(instr & 0xFE000000) >> 20) | ((instr >> 7) & 0x1F);
    int32_t immB = ((int32_t)(instr & 0x80000000) >> 19) |
                   ((instr & 0x00000080) << 4)            |
                   ((instr >> 20) & 0x7E0)                |
                   ((instr >> 7)  & 0x1E);
    uint32_t immU = instr & 0xFFFFF000;
    int32_t immJ = ((int32_t)(instr & 0x80000000) >> 11) |
                   (instr & 0x000FF000)                   |
                   ((instr >> 9)  & 0x00000800)           |
                   ((instr >> 20) & 0x000007FE);

    std::ostringstream o;

    auto R  = [](int n) { return REG_NAMES[n & 0x1F]; };
    auto ri = [&](int rn, int32_t im) -> std::string {
        std::ostringstream t; t << im << "(" << R(rn) << ")"; return t.str();
    };

    switch (opcode) {
    case 0x03: { // LOAD
        const char* mn[] = {"lb","lh","lw","?","lbu","lhu","?","?"};
        o << mn[funct3] << " " << R(rd) << ", " << ri(rs1, immI);
        break;
    }
    case 0x13: { // OP-IMM
        switch (funct3) {
        case 0:
            if (instr == 0x00000013) { o << "nop"; break; }
            o << "addi "  << R(rd) << ", " << R(rs1) << ", " << immI; break;
        case 1: o << "slli "  << R(rd) << ", " << R(rs1) << ", " << shamt; break;
        case 2: o << "slti "  << R(rd) << ", " << R(rs1) << ", " << immI; break;
        case 3: o << "sltiu " << R(rd) << ", " << R(rs1) << ", " << immI; break;
        case 4: o << "xori "  << R(rd) << ", " << R(rs1) << ", " << immI; break;
        case 5:
            if (funct7 == 0) o << "srli " << R(rd) << ", " << R(rs1) << ", " << shamt;
            else             o << "srai " << R(rd) << ", " << R(rs1) << ", " << shamt;
            break;
        case 6: o << "ori "  << R(rd) << ", " << R(rs1) << ", " << immI; break;
        case 7: o << "andi " << R(rd) << ", " << R(rs1) << ", " << immI; break;
        }
        break;
    }
    case 0x17: o << "auipc " << R(rd) << ", " << hex(immU >> 12); break;
    case 0x23: { // STORE
        const char* mn[] = {"sb","sh","sw"};
        if (funct3 <= 2) o << mn[funct3] << " " << R(rs2) << ", " << ri(rs1, immS);
        else             o << "store?";
        break;
    }
    case 0x33: { // OP (R-type)
        switch (funct3) {
        case 0: o << (funct7 ? "sub" : "add") << " " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 1: o << "sll "  << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 2: o << "slt "  << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 3: o << "sltu " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 4: o << "xor "  << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 5: o << (funct7 ? "sra" : "srl") << " " << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 6: o << "or "   << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        case 7: o << "and "  << R(rd) << ", " << R(rs1) << ", " << R(rs2); break;
        }
        break;
    }
    case 0x37: o << "lui " << R(rd) << ", " << hex(immU >> 12); break;
    case 0x63: { // BRANCH
        const char* mn[] = {"beq","bne","?","?","blt","bge","bltu","bgeu"};
        uint32_t target = addr + (uint32_t)immB;
        o << mn[funct3] << " " << R(rs1) << ", " << R(rs2) << ", " << hex(target);
        break;
    }
    case 0x67: { // JALR
        if (rd == 0 && rs1 == 1 && immI == 0)
            o << "ret";
        else if (rd == 0 && immI == 0)
            o << "jr " << R(rs1);
        else
            o << "jalr " << R(rd) << ", " << immI << "(" << R(rs1) << ")";
        break;
    }
    case 0x6F: { // JAL
        uint32_t target = addr + (uint32_t)immJ;
        if (rd == 0)      o << "j "    << hex(target);
        else if (rd == 1) o << "call " << hex(target);
        else              o << "jal "  << R(rd) << ", " << hex(target);
        break;
    }
    case 0x73: {
        if      (instr == 0x00000073) o << "ecall";
        else if (instr == 0x00100073) o << "ebreak";
        else                          o << "system";
        break;
    }
    case 0x0F: o << "fence"; break;
    default:
        o << "unknown(0x" << std::hex << std::setw(8) << std::setfill('0') << instr << ")";
    }

    return o.str();
}
