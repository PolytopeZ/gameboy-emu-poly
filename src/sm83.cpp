#include "sm83.hpp"
#include <cstdio>
#include <cstdlib>

// Op implementation
int Sm83::op_00_NOP(uint8_t opcode)
{
    return 4;
}

int Sm83::op_06_LD_B_d8(uint8_t opcode)
{
    B = bus.read8(PC++);
    return 8;
}

int Sm83::op_unknown(uint8_t opcode)
{
    std::fprintf(stderr, "Unknown opcode at PC=%04X\n", PC - 1);
    std::exit(1);
}

// Helpers
uint8_t Sm83::getR(uint8_t idx) const
{
    switch (idx)
    {
    case 0:
        return B;
    case 1:
        return C;
    case 2:
        return D;
    case 3:
        return E;
    case 4:
        return H;
    case 5:
        return L;
    case 6:
        return bus.read8(HL.val);
    case 7:
        return A;
    }
    return 0xFF; // idx is 3 bits, not reachable
}

void Sm83::setR(uint8_t idx, uint8_t val)
{
    switch (idx)
    {
    case 0:
        B = val;
        break;
    case 1:
        C = val;
        break;
    case 2:
        D = val;
        break;
    case 3:
        E = val;
        break;
    case 4:
        H = val;
        break;
    case 5:
        L = val;
        break;
    case 6:
        bus.write8(HL.val, val);
        break;
    case 7:
        A = val;
        break;
    }
}

// Handlers
const std::array<Sm83::Handler, 256> Sm83::opcodeTable = []
{
    std::array<Handler, 256> t{};
    t.fill(&Sm83::op_unknown);
    t[0x00] = &Sm83::op_00_NOP;
    t[0x06] = &Sm83::op_06_LD_B_d8;

    for (uint8_t dst = 0; dst < 8; ++dst)
    {
        for (uint8_t src = 0; src < 8; ++src)
        {
            if (dst == 6 && src == 6)
            {
                // 0x76 = 01 110 110
                // HALT   LD dst src
                continue;
            }
            t[0x40 + dst * 8 + src] = &Sm83::op_LD_r_r;
        }
    }
    return t;
}();

int Sm83::op_LD_r_r(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0x7;
    uint8_t src = opcode & 0x7;
    setR(dst, getR(src));
    if (dst == 6 || src == 6)
    {
        return 8; // HL = extra cycles
    }
    else
    {
        return 4;
    }
}
