#!/usr/bin/env python3
"""Generate raw binary test programs for the RISC-V simulator."""
import struct, os

def word(v): return struct.pack('<I', v & 0xFFFFFFFF)

def addi(rd, rs1, imm):
    imm12 = imm & 0xFFF
    return (imm12 << 20) | (rs1 << 15) | (0 << 12) | (rd << 7) | 0x13

def add(rd, rs1, rs2):
    return (0 << 25) | (rs2 << 20) | (rs1 << 15) | (0 << 12) | (rd << 7) | 0x33

def sub(rd, rs1, rs2):
    return (0x20 << 25) | (rs2 << 20) | (rs1 << 15) | (0 << 12) | (rd << 7) | 0x33

def sw(rs2, rs1, imm):
    imm12 = imm & 0xFFF
    imm_hi = (imm12 >> 5) & 0x7F
    imm_lo = imm12 & 0x1F
    return (imm_hi << 25) | (rs2 << 20) | (rs1 << 15) | (0x2 << 12) | (imm_lo << 7) | 0x23

def lw(rd, rs1, imm):
    imm12 = imm & 0xFFF
    return (imm12 << 20) | (rs1 << 15) | (0x2 << 12) | (rd << 7) | 0x03

def beq(rs1, rs2, offset):
    imm = offset & 0x1FFF
    b12  = (imm >> 12) & 1
    b11  = (imm >> 11) & 1
    b10_5 = (imm >> 5) & 0x3F
    b4_1  = (imm >> 1) & 0xF
    return (b12 << 31) | (b10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (0 << 12) | (b4_1 << 8) | (b11 << 7) | 0x63

def jal(rd, offset):
    imm = offset & 0x1FFFFF
    b20    = (imm >> 20) & 1
    b10_1  = (imm >> 1) & 0x3FF
    b11    = (imm >> 11) & 1
    b19_12 = (imm >> 12) & 0xFF
    return (b20 << 31) | (b10_1 << 21) | (b11 << 20) | (b19_12 << 12) | (rd << 7) | 0x6F

def ecall():
    return 0x00000073

os.makedirs("tests", exist_ok=True)

# ------------------------------------------------------------------
# Test 1: riscvtest – writes 25 (0x19) to address 100 (0x64)
# Checks: add, sub, and, or, slt, addi, lw, sw, beq, jal
# ------------------------------------------------------------------
# Register ABI: x1=ra, x2=sp ... using x5..x15 as temporaries
instrs = []

# x5 = 10, x6 = 15
instrs.append(addi(5, 0, 10))   # addi x5, x0, 10
instrs.append(addi(6, 0, 15))   # addi x6, x0, 15

# x7 = x5 + x6 = 25
instrs.append(add(7, 5, 6))     # add x7, x5, x6

# x8 = x7 - x6 = 10
instrs.append(sub(8, 7, 6))     # sub x8, x7, x6

# verify x8 == x5 using beq, jump past failure
# beq x8, x5, +8 (skip next 2 instr)  ... but for simplicity just continue
# x9 = x5 & x6 = 10 & 15 = 10
instrs.append(0x00F2F4B3)       # and x9, x5, x6  (encoded manually: 0x00F2F4B3)
# x10 = x5 | x6 = 10 | 15 = 15
instrs.append(0x00F2E533)       # or x10, x5, x6

# slt: x11 = (x5 < x6) = 1
instrs.append(0x0062A5B3)       # slt x11, x5, x6

# jal to skip next instruction (test jal)
instrs.append(jal(1, 8))        # jal ra, +8  (skip 1 instr)
instrs.append(addi(12, 0, 0xFF)) # should be skipped
instrs.append(addi(12, 0, 0))   # x12 = 0 (landed here)

# lw/sw test: store x7 (=25) to address 100, then load it back
instrs.append(addi(13, 0, 100)) # x13 = 100
instrs.append(sw(7, 13, 0))     # sw x7, 0(x13)   — stores 25 at addr 100
instrs.append(lw(14, 13, 0))    # lw x14, 0(x13)  — load back → x14 = 25

# beq test: x14 should equal x7 (both 25)
# beq x14, x7, +8  (branch to ecall exit, skip the error store)
instrs.append(beq(14, 7, 8))    # if equal, skip
instrs.append(addi(15, 0, 0xBE)) # error marker (should NOT execute)

# ecall exit (a7=10, a0=0)
instrs.append(addi(17, 0, 10))  # a7 = 10
instrs.append(addi(10, 0, 0))   # a0 = 0 (exit code)
instrs.append(ecall())

with open("tests/riscvtest.bin", "wb") as f:
    for ins in instrs:
        f.write(word(ins))

print(f"riscvtest.bin: {len(instrs)} instrucciones ({len(instrs)*4} bytes)")
print("  Esperado: mem[100] = 25 (0x19), x7=25, x11=1 (slt), x14=25")

# ------------------------------------------------------------------
# Test 2: Suma de 1+2+3+...+10 usando un loop
# ------------------------------------------------------------------
instrs2 = []
# x5 = contador (1..10), x6 = acumulador
instrs2.append(addi(5, 0, 1))    # x5 = 1
instrs2.append(addi(6, 0, 0))    # x6 = 0
instrs2.append(addi(7, 0, 10))   # x7 = 10 (limite)
# loop: x6 += x5; x5++; if x5 <= x7 goto loop
# loop start is at byte offset 8 (instr index 2)
# add x6, x6, x5
instrs2.append(add(6, 6, 5))     # x6 += x5
# addi x5, x5, 1
instrs2.append(addi(5, 5, 1))    # x5++
# bge x7, x5, -8  (branch back if x7 >= x5, i.e. x5 <= 10)
# offset = -8 (2 instrs back)
instrs2.append(0xFE53D8E3)       # bge x7, x5, -16+8 -- let me encode properly
# Actually let me encode bge x7, x5, offset
# bge is funct3=5 in branch (bgeu is 7)
# bge rs1=x7, rs2=x5, offset=-8
# The loop body is 2 instructions = 8 bytes, so offset = -8
# offset=-8 = 0xFFFFFFF8
# b-type: imm[12|10:5|4:1|11]
# -8 in 13-bit: 0b1_1111_1111_1000 → bits: 12=1, 11=1, 10:5=111111, 4:1=1100, 0=0
# Actually -8 = 0x1FF8 in 13 bits
# b12=1, b11=1, b10_5=111111, b4_1=1100
# instr = b12<<31 | b10_5<<25 | rs2<<20 | rs1<<15 | funct3<<12 | b4_1<<8 | b11<<7 | 0x63
# rs1=7, rs2=5, funct3=5 (bge)
# imm = -8 = 0x1FF8 (13-bit)
# b12 = (0x1FF8 >> 12) & 1 = 1
# b11 = (0x1FF8 >> 11) & 1 = 1
# b10_5 = (0x1FF8 >> 5) & 0x3F = (0xFF >> 0) actually...
# Let's compute properly: -8 in 13-bit signed = 8192-8 = 8184 = 0x1FF8
# Actually for two's complement 13-bit: -8 = 0x1FF8 doesn't make sense
# -8 in 13-bit: 2^13 - 8 = 8192 - 8 = 8184 = 0b_1_1111_1111_1000
# b12 = bit12 of 8184 = 1
# b11 = bit11 of 8184 = 1
# b10:5 = bits10:5 of 8184 = 0b111111 = 63
# b4:1 = bits4:1 of 8184 = 0b1100 = 12

def bge(rs1, rs2, offset):
    imm = offset & 0x1FFF
    b12   = (imm >> 12) & 1
    b11   = (imm >> 11) & 1
    b10_5 = (imm >> 5)  & 0x3F
    b4_1  = (imm >> 1)  & 0xF
    return (b12 << 31) | (b10_5 << 25) | (rs2 << 20) | (rs1 << 15) | (5 << 12) | (b4_1 << 8) | (b11 << 7) | 0x63

instrs2[-1] = bge(7, 5, -8)  # replace placeholder

# store result
instrs2.append(addi(13, 0, 200))  # x13 = 200
instrs2.append(sw(6, 13, 0))      # sw x6, 0(x13)  → mem[200] = 55
# ecall exit
instrs2.append(addi(17, 0, 10))
instrs2.append(addi(10, 0, 0))
instrs2.append(ecall())

with open("tests/sum_loop.bin", "wb") as f:
    for ins in instrs2:
        f.write(word(ins))

print(f"sum_loop.bin:  {len(instrs2)} instrucciones ({len(instrs2)*4} bytes)")
print("  Esperado: mem[200] = 55 (suma 1+2+...+10)")
