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
    bool IME = false; // Interrupt master enable ei = 1, di = 0, reti = 1+ret

    using Handler = int (Sm83::*)(uint8_t opcode);
    static const std::array<Handler, 256> opcodeTable;

    uint8_t getR(uint8_t idx) const; // idx 0..7 -> B,C,D,E,H,L,(HL),A
    void setR(uint8_t idx, uint8_t val);

    bool checkCond(uint8_t cc) const; // cc: 0=NZ,1=Z,2=NC,3=C

    int op_00_NOP(uint8_t opcode);
    int op_LD_r_d8(uint8_t opcode); // 0x06/0x0E/0x16/0x1E/0x26/0x2E/0x36/0x3E
    int op_LD_r_r(uint8_t opcde);
    int op_LD_rr_d16(uint8_t opcode);    // 0x01/0x11/0x21/0x31
    int op_LD_HLI_HLD_A(uint8_t opcode); // 0x22/0x2A/0x32/0x3A
    int op_LD_BC_DE_A(uint8_t opcode);   // 0x02/0x0A/0x12/0x1A
    int op_LD_a16_A(uint8_t opcode);     // 0xEA/0xFA
    int op_LDH_a8_A(uint8_t opcode);     // 0xE0/0xF0
    int op_LD_C_A(uint8_t opcode);       // 0xE2/0xF2

    int op_JR_e8(uint8_t opcode);    // Ox18
    int op_JR_cc_e8(uint8_t opcode); // 0x20 0x28 0x30 0x38

    int op_JP_a16(uint8_t opcode);    // 0xC3
    int op_JP_cc_a16(uint8_t opcode); // 0xC2 0xCA 0xD2 0xDA
    int op_JP_HL(uint8_t opcode);     // 0xE9

    int op_CALL_a16(uint8_t opcode);    // 0xCD
    int op_CALL_cc_a16(uint8_t opcode); // 0xC4 0xCC 0xD4 0xDC

    int op_RET(uint8_t opcode);    // 0xC9
    int op_RET_cc(uint8_t opcode); // 0xC0 0xC8 0xD0 0xD8
    int op_RETI(uint8_t opcode);   // 0xD9

    int op_RST(uint8_t opcode); // 0xC7 0xCF 0xD7 0xDF 0xE7 0xEF 0xF7 0xFF

    int op_INC_r(uint8_t opcode); // 0x04/0x0C/0x14/0x1C/0x24/0x2C/0x34/0x3C
    int op_DEC_r(uint8_t opcode); // 0x05/0x0D/0x15/0x1D/0x25/0x2D/0x35/0x3D

    int op_DI(uint8_t opcode); // 0xF3
    int op_EI(uint8_t opcode); // 0xFB

    int op_PUSH_rr(uint8_t opcode); // 0xC5/0xD5/0xE5/0xF5
    int op_POP_rr(uint8_t opcode);  // 0xC1/0xD1/0xE1/0xF1

    int op_INC_rr(uint8_t opcode); // 0x03/0x13/0x23/0x33
    int op_DEC_rr(uint8_t opcode); // 0x0B/0x1B/0x2B/0x3B

    int op_ALU_A_r(uint8_t opcode);      // 0x80-0xBF
    void aluOp(uint8_t op, uint8_t val); // shared ADD/ADC/SUB/SBC/AND/XOR/OR/CP logic
    int op_ALU_A_d8(uint8_t opcode);     // 0xC6/0xCE/0xD6/0xDE/0xE6/0xEE/0xF6/0xFE

    int op_unknown(uint8_t opcode);
};