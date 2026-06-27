#!/usr/bin/env bash
# Build the RISC-V simulator using WSL g++
set -e
mkdir -p build
g++ -std=c++17 -Wall -Wextra -O2 -Isrc \
    src/main.cpp src/memory.cpp src/cpu.cpp src/disasm.cpp \
    -o build/riscv-sim
echo "Compilacion exitosa: build/riscv-sim"
