#pragma once
#include "memory.h"
#include <cstdint>
#include <string>

class CPU {
public:
    uint32_t regs[32];
    uint32_t pc;
    bool     halted;
    int      exitCode;

    explicit CPU(Memory& mem);

    void reset();
    void step();

    uint32_t getReg(int n) const { return n == 0 ? 0u : regs[n]; }
    void     setReg(int n, uint32_t v) { if (n != 0) regs[n] = v; }

    static const char* regName(int n);
    static int  regIndex(const std::string& name);

private:
    Memory& mem;
    void handleEcall();
};
