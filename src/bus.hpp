#pragma once
#include <cstdint>
#include <array>

class Bus
{
public:
    std::array<uint8_t, 0x10000> mem{}; // 0xFFFF+1

    uint8_t read8(uint16_t addr) const
    {
        return mem[addr];
    }

    void write8(uint16_t addr, uint8_t val)
    {
        mem[addr] = val;
    }
};