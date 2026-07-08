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
    IME = true;
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

int Sm83::op_LD_a16_A(uint8_t opcode)
{
    bool isLoad = opcode & 0x10;
    uint16_t addr = read16(PC);
    PC += 2;

    if (isLoad)
    {
        A = bus.read8(addr);
    }
    else
    {
        bus.write8(addr, A);
    }

    return 16;
}

int Sm83::op_LDH_a8_A(uint8_t opcode)
{
    bool isLoad = opcode & 0x10;
    uint8_t offset = bus.read8(PC++);
    uint16_t addr = 0xFF00 + offset;

    if (isLoad)
    {
        A = bus.read8(addr);
    }
    else
    {
        bus.write8(addr, A);
    }

    return 12;
}

int Sm83::op_LD_C_A(uint8_t opcode)
{
    bool isLoad = opcode & 0x10;
    uint16_t addr = 0xFF00 + C;

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

int Sm83::op_INC_r(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0x7;
    uint8_t val = getR(dst);
    uint8_t result = val + 1;
    setR(dst, result);

    setFlag(FLAG_Z, result == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, (val & 0x0F) == 0x0F);

    if (dst == 6)
    {
        return 12;
    }
    else
    {
        return 4;
    }
}

int Sm83::op_DEC_r(uint8_t opcode)
{
    uint8_t dst = (opcode >> 3) & 0x7;
    uint8_t val = getR(dst);
    uint8_t result = val - 1;
    setR(dst, result);

    setFlag(FLAG_Z, result == 0);
    setFlag(FLAG_N, true);
    setFlag(FLAG_H, (val & 0x0F) == 0x00);

    if (dst == 6)
    {
        return 12;
    }
    else
    {
        return 4;
    }
}

int Sm83::op_DI(uint8_t opcode)
{
    IME = false;
    return 4;
}

int Sm83::op_EI(uint8_t opcode)
{
    IME = true;
    return 4;
}

int Sm83::op_PUSH_rr(uint8_t opcode)
{
    uint8_t rr = (opcode >> 4) & 0x3;
    uint16_t val;
    switch (rr)
    {
    case 0:
        val = BC.val;
        break;
    case 1:
        val = DE.val;
        break;
    case 2:
        val = HL.val;
        break;
    case 3:
        val = AF.val;
        break;
    }

    SP -= 2;
    write16(SP, val);
    return 16;
}

int Sm83::op_POP_rr(uint8_t opcode)
{
    uint8_t rr = (opcode >> 4) & 0x3;
    uint16_t val = read16(SP);
    SP += 2;

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
        AF.val = val & 0xFFF0;
        break;
    }

    return 12;
}

int Sm83::op_INC_rr(uint8_t opcode)
{
    uint8_t rr = (opcode >> 4) & 0x3;
    switch (rr)
    {
    case 0:
        BC.val++;
        break;
    case 1:
        DE.val++;
        break;
    case 2:
        HL.val++;
        break;
    case 3:
        SP++;
        break;
    }
    return 8;
}

int Sm83::op_DEC_rr(uint8_t opcode)
{
    uint8_t rr = (opcode >> 4) & 0x3;
    switch (rr)
    {
    case 0:
        BC.val--;
        break;
    case 1:
        DE.val--;
        break;
    case 2:
        HL.val--;
        break;
    case 3:
        SP--;
        break;
    }
    return 8;
}

void Sm83::aluOp(uint8_t op, uint8_t val)
{
    switch (op)
    {
    case 0: // ADD
    {
        uint16_t res = A + val;
        setFlag(FLAG_H, ((A & 0xF) + (val & 0xF)) > 0xF);
        setFlag(FLAG_C, res > 0xFF);
        A = (uint8_t)res;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, false);
        break;
    }
    case 1: // ADC - Add carry-in
    {
        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
        uint16_t res = A + val + carry;
        setFlag(FLAG_H, ((A & 0xF) + (val & 0xF) + carry) > 0xF);
        setFlag(FLAG_C, res > 0xFF);
        A = (uint8_t)res;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, false);
        break;
    }
    case 2: // SUB
        setFlag(FLAG_H, (A & 0xF) < (val & 0xF));
        setFlag(FLAG_C, A < val);
        A = A - val;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, true);
        break;
    case 3: // SBC - Sub borrow-in
    {
        uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
        int res = A - val - carry;
        setFlag(FLAG_H, (A & 0xF) < ((val & 0xF) + carry));
        setFlag(FLAG_C, res < 0);
        A = (uint8_t)res;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, true);
        break;
    }
    case 4: // AND
        A &= val;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, true);
        setFlag(FLAG_C, false);
        break;
    case 5: // XOR
        A ^= val;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, false);
        setFlag(FLAG_C, false);
        break;
    case 6: // OR
        A |= val;
        setFlag(FLAG_Z, A == 0);
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, false);
        setFlag(FLAG_C, false);
        break;
    case 7: // CP - Compare
    {
        setFlag(FLAG_H, (A & 0xF) < (val & 0xF));
        setFlag(FLAG_C, A < val);
        uint8_t result = A - val;
        setFlag(FLAG_Z, result == 0);
        setFlag(FLAG_N, true);
        break;
    }
    }
}

int Sm83::op_ALU_A_r(uint8_t opcode)
{
    uint8_t op = (opcode >> 3) & 0x7;
    uint8_t idx = opcode & 0x7;
    uint8_t val = getR(idx);

    aluOp(op, val);

    if (idx == 6)
    {
        return 8;
    }
    else
    {
        return 4;
    }
}

int Sm83::op_ALU_A_d8(uint8_t opcode)
{
    uint8_t op = (opcode >> 3) & 0x7;
    uint8_t val = bus.read8(PC++);

    aluOp(op, val);

    return 8;
}

int Sm83::op_CB_rot(uint8_t cb)
{
    uint8_t subop = (cb >> 3) & 0x7;
    uint8_t idx = cb & 0x7;
    uint8_t val = getR(idx);
    uint8_t res = 0;
    bool carry = false;

    switch (subop)
    {
    case 0: // RLC
        carry = val & 0x80;
        res = (val << 1) | (carry ? 1 : 0);
        break;
    case 1: // RRC
        carry = val & 0x01;
        res = (val >> 1) | (carry ? 0x80 : 0);
        break;
    case 2: // RL
        carry = val & 0x80;
        res = (val << 1) | (getFlag(FLAG_C) ? 1 : 0);
        break;
    case 3: // RR
        carry = val & 0x01;
        res = (val >> 1) | (getFlag(FLAG_C) ? 0x80 : 0);
        break;
    case 4: // SLA
        carry = val & 0x80;
        res = val << 1;
        break;
    case 5: // SRA
        carry = val & 0x01;
        res = (val >> 1) | (val & 0x80); // bit 7 preserved
        break;
    case 6: // SWAP
        res = (val << 4) | (val >> 4);
        break;
    case 7: // SRL
        carry = val & 0x01;
        res = val >> 1;
        break;
    }

    setR(idx, res);
    setFlag(FLAG_Z, res == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, carry);

    if (idx == 6)
    {
        return 16;
    }
    else
    {
        return 8;
    }
}

int Sm83::op_CB_BIT(uint8_t cb)
{
    uint8_t bit = (cb >> 3) & 0x7;
    uint8_t idx = cb & 0x7;
    uint8_t val = getR(idx);

    setFlag(FLAG_Z, !(val & (1 << bit)));
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, true);
    // C not changed

    if (idx == 6)
    {
        return 12;
    }
    else
    {
        return 8;
    }
}

int Sm83::op_CB_RES(uint8_t cb)
{
    uint8_t bit = (cb >> 3) & 0x7;
    uint8_t idx = cb & 0x7;
    setR(idx, getR(idx) & ~(1 << bit));
    if (idx == 6)
    {
        return 16;
    }
    else
    {
        return 8;
    }
}

int Sm83::op_CB_SET(uint8_t cb)
{
    uint8_t bit = (cb >> 3) & 0x7;
    uint8_t idx = cb & 0x7;
    setR(idx, getR(idx) | (1 << bit));
    if (idx == 6)
    {
        return 16;
    }
    else
    {
        return 8;
    }
}

int Sm83::op_unknown(uint8_t opcode)
{
    std::fprintf(stderr, "Unknown opcode 0x%02X at PC=%04X\n", opcode, PC - 1);
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

// CB Dispatcher
int Sm83::op_CB_prefix(uint8_t opcode)
{
    uint8_t cb = bus.read8(PC++);
    Handler h = cbTable[cb];
    return (this->*h)(cb);
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
    t[0xF3] = &Sm83::op_DI;
    t[0xFB] = &Sm83::op_EI;
    t[0x22] = &Sm83::op_LD_HLI_HLD_A;
    t[0x2A] = &Sm83::op_LD_HLI_HLD_A;
    t[0x32] = &Sm83::op_LD_HLI_HLD_A;
    t[0x3A] = &Sm83::op_LD_HLI_HLD_A;
    t[0x02] = &Sm83::op_LD_BC_DE_A;
    t[0x0A] = &Sm83::op_LD_BC_DE_A;
    t[0x12] = &Sm83::op_LD_BC_DE_A;
    t[0x1A] = &Sm83::op_LD_BC_DE_A;
    t[0xEA] = &Sm83::op_LD_a16_A;
    t[0xFA] = &Sm83::op_LD_a16_A;
    t[0xE0] = &Sm83::op_LDH_a8_A;
    t[0xF0] = &Sm83::op_LDH_a8_A;
    t[0xE2] = &Sm83::op_LD_C_A;
    t[0xF2] = &Sm83::op_LD_C_A;
    t[0xCB] = &Sm83::op_CB_prefix;

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

    for (uint8_t dst = 0; dst < 8; ++dst)
    {
        t[0x04 + dst * 8] = &Sm83::op_INC_r;
        t[0x05 + dst * 8] = &Sm83::op_DEC_r;
    }

    for (uint8_t rr = 0; rr < 4; ++rr)
    {
        t[0xC5 + rr * 0x10] = &Sm83::op_PUSH_rr;
        t[0xC1 + rr * 0x10] = &Sm83::op_POP_rr;
    }

    for (uint8_t rr = 0; rr < 4; ++rr)
    {
        t[0x03 + rr * 0x10] = &Sm83::op_INC_rr;
        t[0x0B + rr * 0x10] = &Sm83::op_DEC_rr;
    }

    for (uint8_t op = 0; op < 8; ++op)
    {
        for (uint8_t idx = 0; idx < 8; ++idx)
        {
            t[0x80 + op * 8 + idx] = &Sm83::op_ALU_A_r;
        }
    }

    for (uint8_t op = 0; op < 8; ++op)
    {
        t[0xC6 + op * 8] = &Sm83::op_ALU_A_d8;
    }
    return t;
}();

const std::array<Sm83::Handler, 256> Sm83::cbTable = []
{
    std::array<Handler, 256> t{};
    t.fill(&Sm83::op_unknown);

    for (uint8_t sub = 0; sub < 8; ++sub)
    {
        for (uint8_t idx = 0; idx < 8; ++idx)
        {
            t[sub * 8 + idx] = &Sm83::op_CB_rot;
        }
    }

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        for (uint8_t idx = 0; idx < 8; ++idx)
        {
            t[0x40 + bit * 8 + idx] = &Sm83::op_CB_BIT;
        }
    }

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        for (uint8_t idx = 0; idx < 8; ++idx)
        {
            t[0x80 + bit * 8 + idx] = &Sm83::op_CB_RES;
        }
    }

    for (uint8_t bit = 0; bit < 8; ++bit)
    {
        for (uint8_t idx = 0; idx < 8; ++idx)
        {
            t[0xC0 + bit * 8 + idx] = &Sm83::op_CB_SET;
        }
    }

    return t;
}();
