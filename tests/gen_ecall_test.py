#!/usr/bin/env python3
"""Generate a test that uses ecall to print numbers and a string."""
import struct, os

def word(v): return struct.pack('<I', v & 0xFFFFFFFF)

def addi(rd, rs1, imm):
    return ((imm & 0xFFF) << 20) | (rs1 << 15) | (rd << 7) | 0x13

def lui(rd, imm20):
    return ((imm20 & 0xFFFFF) << 12) | (rd << 7) | 0x37

def ecall(): return 0x00000073

os.makedirs("tests", exist_ok=True)

# Layout:
# Instructions at 0x0000
# String data at 0x0100: "Hola RISC-V!\n\0"
STRING_ADDR = 0x100
STRING      = b"Hola RISC-V!\n\0"

instrs = []

# print_int: a7=1, a0=42
instrs.append(addi(10, 0, 42))  # a0 = 42
instrs.append(addi(17, 0, 1))   # a7 = 1 (print_int)
instrs.append(ecall())

# print_char: a7=11, a0='\n'=10
instrs.append(addi(10, 0, 10))  # a0 = '\n'
instrs.append(addi(17, 0, 11))  # a7 = 11 (print_char)
instrs.append(ecall())

# print_string: a7=4, a0=STRING_ADDR
instrs.append(addi(10, 0, STRING_ADDR))  # a0 = 0x100
instrs.append(addi(17, 0, 4))            # a7 = 4 (print_string)
instrs.append(ecall())

# print_int: print 2025
# lui a0, 0  then addi a0, a0, 2025 — but 2025 > 2047, so use a different approach
# 2025 = 0x7E9, fits in 12-bit signed (max 2047), so just addi works
instrs.append(addi(10, 0, 2025))  # a0 = 2025 (0x7E9, fits in 12-bit)
instrs.append(addi(17, 0, 1))     # a7 = 1
instrs.append(ecall())

# print_char '\n'
instrs.append(addi(10, 0, 10))
instrs.append(addi(17, 0, 11))
instrs.append(ecall())

# exit
instrs.append(addi(17, 0, 10))  # a7 = 10
instrs.append(addi(10, 0, 0))   # a0 = 0
instrs.append(ecall())

# Build binary: pad instructions to 0x100, then string data
instr_bytes = b"".join(word(i) for i in instrs)
pad = b"\x00" * (STRING_ADDR - len(instr_bytes))
binary = instr_bytes + pad + STRING

with open("tests/ecall_test.bin", "wb") as f:
    f.write(binary)

print(f"ecall_test.bin: {len(binary)} bytes")
print("Esperado: '42\\nHola RISC-V!\\n2025\\n'")
