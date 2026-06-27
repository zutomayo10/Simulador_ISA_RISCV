#pragma once
#include <cstdint>
#include <string>


std::string disassemble(uint32_t addr, uint32_t instr);

const char* regABIName(int n);
