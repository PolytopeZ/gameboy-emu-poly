#pragma once
#include <cstdint>
#include "bus.hpp"

struct RegisterPair
{
    union
    {
        uint16_t val;
        struct
        {
            // little-endian so low byte first
            uint8_t lo;
            uint8_t hi;
        };
    };
};

// Register F ZNHC____
enum Flag : uint8_t
{
    FLAG_Z = 1 << 7, // Zero flag
    FLAG_N = 1 << 6, // Sub flag
    FLAG_H = 1 << 5, // Half Carry flag
    FLAG_C = 1 << 4, // Carry flag
};

class Sm83
{
public:
    Sm83(Bus &bus) : bus(bus) {}

    RegisterPair AF; // Accumulator & Flags
    RegisterPair BC;
    RegisterPair DE;
    RegisterPair HL;

    uint8_t &A = AF.hi;
    uint8_t &F = AF.lo;
    uint8_t &B = BC.hi;
    uint8_t &C = BC.lo;
    uint8_t &D = DE.hi;
    uint8_t &E = DE.lo;
    uint8_t &H = HL.hi;
    uint8_t &L = HL.lo;

    // Helpers for flags
    void setFlag(Flag f, bool on)
    {
        if (on)
        {
            F |= f;
        }
        else
        {
            F &= ~f;
        }
    }

    bool getFlag(Flag f) const
    {
        return F & f;
    }

    // Helpers for bus
    uint16_t read16(uint16_t addr) const
    {
        return bus.read8(addr) | (bus.read8(addr + 1) << 8);
    }

    void write16(uint16_t addr, uint16_t val)
    {
        bus.write8(addr, val & 0xFF);
        bus.write8(addr + 1, val >> 8);
    }

private:
    Bus &bus;
};