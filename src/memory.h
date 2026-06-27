#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <array>

class Memory {
public:
    static constexpr uint32_t PAGE_BITS = 12;
    static constexpr uint32_t PAGE_SIZE = 1u << PAGE_BITS;

    Memory() = default;

    uint8_t  read8(uint32_t addr) const;
    uint16_t read16(uint32_t addr) const;
    uint32_t read32(uint32_t addr) const;

    void write8(uint32_t addr, uint8_t val);
    void write16(uint32_t addr, uint16_t val);
    void write32(uint32_t addr, uint32_t val);

    bool loadFromFile(const std::string& path, uint32_t baseAddr = 0);
    void reset();
    void dump(uint32_t start, uint32_t end) const;

private:
    std::unordered_map<uint32_t, std::array<uint8_t, PAGE_SIZE>> pages;
};
