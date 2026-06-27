#include "memory.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

static std::string fmtAddr(uint32_t v) {
    std::ostringstream o;
    o << "0x" << std::hex << std::setw(8) << std::setfill('0') << v;
    return o.str();
}

uint8_t Memory::read8(uint32_t addr) const {
    uint32_t pageNum = addr >> PAGE_BITS;
    auto it = pages.find(pageNum);
    if (it == pages.end()) return 0;
    return it->second[addr & (PAGE_SIZE - 1)];
}

uint16_t Memory::read16(uint32_t addr) const {
    return (uint16_t)read8(addr) |
           ((uint16_t)read8(addr + 1) << 8);
}

uint32_t Memory::read32(uint32_t addr) const {
    return (uint32_t)read8(addr)           |
           ((uint32_t)read8(addr + 1) << 8)  |
           ((uint32_t)read8(addr + 2) << 16) |
           ((uint32_t)read8(addr + 3) << 24);
}

void Memory::write8(uint32_t addr, uint8_t val) {
    uint32_t pageNum = addr >> PAGE_BITS;
    pages[pageNum][addr & (PAGE_SIZE - 1)] = val;
}

void Memory::write16(uint32_t addr, uint16_t val) {
    write8(addr,     val & 0xFF);
    write8(addr + 1, (val >> 8) & 0xFF);
}

void Memory::write32(uint32_t addr, uint32_t val) {
    write8(addr,     val & 0xFF);
    write8(addr + 1, (val >> 8)  & 0xFF);
    write8(addr + 2, (val >> 16) & 0xFF);
    write8(addr + 3, (val >> 24) & 0xFF);
}

bool Memory::loadFromFile(const std::string& path, uint32_t baseAddr) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "Error: No se pudo abrir '" << path << "'\n";
        return false;
    }

    uint32_t addr = baseAddr;
    char byte;
    while (file.read(&byte, 1)) {
        write8(addr++, (uint8_t)byte);
    }

    std::cout << "\"" << path << "\" cargado a memoria ("
              << (addr - baseAddr) << " bytes en " << fmtAddr(baseAddr) << ").\n";
    return true;
}

void Memory::reset() {
    pages.clear();
}

void Memory::dump(uint32_t start, uint32_t end) const {
    if (end < start) {
        std::cout << "Error: direccion final menor que inicial.\n";
        return;
    }
    std::cout << "Memoria (" << fmtAddr(start) << " - " << fmtAddr(end) << "):\n";

    for (uint32_t addr = start; addr <= end; ++addr) {
        if ((addr - start) % 16 == 0)
            std::cout << "  " << fmtAddr(addr) << ": ";
        std::ostringstream byt;
        byt << std::hex << std::setw(2) << std::setfill('0') << (unsigned)read8(addr);
        std::cout << byt.str();
        if (addr < end) {
            if ((addr - start) % 16 == 15) std::cout << "\n";
            else std::cout << " ";
        }
    }
    std::cout << "\n";
}
