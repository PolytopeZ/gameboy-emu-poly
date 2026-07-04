#pragma once
#include <cstdint>
#include <array>
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

    uint16_t PC;
    uint16_t SP;

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

    // Cycle
    int step()
    {
        uint8_t opcode = bus.read8(PC++);
        Handler h = opcodeTable[opcode];
        return (this->*h)(opcode);
    }

private:
    Bus &bus;

    using Handler = int (Sm83::*)(uint8_t opcode);
    static const std::array<Handler, 256> opcodeTable;

    uint8_t getR(uint8_t idx) const; // idx 0..7 -> B,C,D,E,H,L,(HL),A
    void setR(uint8_t idx, uint8_t val);

    int op_00_NOP(uint8_t opcode);
    int op_06_LD_B_d8(uint8_t opcode);
    int op_LD_r_r(uint8_t opcde);

    int op_unknown(uint8_t opcode);
};