#include "sm83.hpp"
#include <cstdio>
#include <cstdlib>

// Op implementation
int Sm83::op_00_NOP(uint8_t opcode)
{
    return 4;
}

int Sm83::op_JR_e8(uint8_t opcode)
{
    int8_t offset = (int8_t)bus.read8(PC++);
    PC += offset;
    return 12;
}

int Sm83::op_JR_cc_e8(uint8_t opcode)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    int8_t offset = (int8_t)bus.read8(PC++);
    if (checkCond(cc))
    {
        PC += offset;
        return 12;
    }
    return 8;
}

int Sm83::op_JP_a16(uint8_t opcode)
{
    PC = read16(PC);
    return 16;
}

int Sm83::op_JP_cc_a16(uint8_t opcode)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint16_t addr = read16(PC);

    PC += 2; // skip the opcode we read
    if (checkCond(cc))
    {
        PC = addr;
        return 16;
    }
    return 12;
}
int Sm83::op_JP_HL(uint8_t opcode)
{
    PC = HL.val;
    return 4;
}

int Sm83::op_CALL_a16(uint8_t opcode)
{
    uint16_t addr = read16(PC);
    PC += 2;
    SP -= 2;
    write16(SP, PC);
    PC = addr;
    return 24;
}

int Sm83::op_CALL_cc_a16(uint8_t opcode)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    uint16_t addr = read16(PC);
    PC += 2;
    if (checkCond(cc))
    {
        SP -= 2;
        write16(SP, PC);
        PC = addr;
        return 24;
    }
    return 12;
}

int Sm83::op_RET(uint8_t opcode)
{
    PC = read16(SP);
    SP += 2;
    return 16;
}

int Sm83::op_RET_cc(uint8_t opcode)
{
    uint8_t cc = (opcode >> 3) & 0x3;
    if (checkCond(cc))
    {
        PC = read16(SP);
        SP += 2;
        return 20;
    }
    return 8;
}

int Sm83::op_RETI(uint8_t opcode)
{
    PC = read16(SP);
    SP += 2;
    return 16;
}

int Sm83::op_RST(uint8_t opcode)
{
    uint16_t index = opcode & 0x38;
    SP -= 2;
    write16(SP, PC);
    PC = index;
    return 16;
}

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

int Sm83::op_LD_rr_d16(uint8_t opcode)
{
    uint8_t rr = (opcode >> 4) & 0x3;
    uint16_t val = read16(PC);
    PC += 2;
    switch (rr)
    {
    case 0:
        BC.val = val;
        break;
    case 1:
        DE.val = val;
        break;
    case 2:
        HL.val = val;
        break;
    case 3:
        SP = val;
        break;
    }
    return 12;
}

int Sm83::op_LD_r_d8(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0x7;
    uint8_t val = bus.read8(PC++);
    setR(dst, val);
    if (dst == 6)
    {
        return 12;
    }
    else
    {
        return 8;
    }
}

int Sm83::op_LD_HLI_HLD_A(uint8_t opcode)
{
    bool isLoad = opcode & 0x08;
    bool isDec = opcode & 0x10;

    if (isLoad)
        A = bus.read8(HL.val);
    else
        bus.write8(HL.val, A);

    HL.val += isDec ? -1 : 1;
    return 8;
}

int Sm83::op_LD_BC_DE_A(uint8_t opcode)
{
    bool isLoad = opcode & 0x08;
    bool isDE = opcode & 0x10;
    uint16_t addr;

    if (isDE)
    {
        addr = DE.val;
    }
    else
    {
        addr = BC.val;
    }

    if (isLoad)
    {
        A = bus.read8(addr);
    }
    else
    {
        bus.write8(addr, A);
    }

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

// 0=NZ,1=Z,2=NC,3=C
bool Sm83::checkCond(uint8_t cc) const
{
    switch (cc)
    {
    case 0:
        return !getFlag(FLAG_Z);
    case 1:
        return getFlag(FLAG_Z);
    case 2:
        return !getFlag(FLAG_C);
    case 3:
        return getFlag(FLAG_C);
    }
    return false; // unreachable, cc is 2 bits
}

// Handlers
const std::array<Sm83::Handler, 256> Sm83::opcodeTable = []
{
    std::array<Handler, 256> t{};
    t.fill(&Sm83::op_unknown);
    t[0x00] = &Sm83::op_00_NOP;
    t[0x18] = &Sm83::op_JR_e8;
    t[0xC3] = &Sm83::op_JP_a16;
    t[0xE9] = &Sm83::op_JP_HL;
    t[0xCD] = &Sm83::op_CALL_a16;
    t[0xC9] = &Sm83::op_RET;
    t[0xD9] = &Sm83::op_RETI;
    t[0x22] = &Sm83::op_LD_HLI_HLD_A;
    t[0x2A] = &Sm83::op_LD_HLI_HLD_A;
    t[0x32] = &Sm83::op_LD_HLI_HLD_A;
    t[0x3A] = &Sm83::op_LD_HLI_HLD_A;
    t[0x02] = &Sm83::op_LD_BC_DE_A;
    t[0x0A] = &Sm83::op_LD_BC_DE_A;
    t[0x12] = &Sm83::op_LD_BC_DE_A;
    t[0x1A] = &Sm83::op_LD_BC_DE_A;

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

    for (uint8_t dst = 0; dst < 8; ++dst)
    {
        t[0x06 + dst * 8] = &Sm83::op_LD_r_d8;
    }

    for (uint8_t rr = 0; rr < 4; ++rr)
    {
        t[0x01 + rr * 0x10] = &Sm83::op_LD_rr_d16;
    }

    for (uint8_t cc = 0; cc < 4; cc++)
    {
        t[0x20 + cc * 8] = &Sm83::op_JR_cc_e8;
        t[0xC2 + cc * 8] = &Sm83::op_JP_cc_a16;
        t[0xC4 + cc * 8] = &Sm83::op_CALL_cc_a16;
        t[0xC0 + cc * 8] = &Sm83::op_RET_cc;
    }

    for (uint8_t n = 0; n < 8; ++n)
    {
        t[0xC7 + n * 8] = &Sm83::op_RST;
    }

    return t;
}();
