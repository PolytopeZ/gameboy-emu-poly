
#include "sm83.hpp"
#include "bus.hpp"
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::fprintf(stderr, "usage: %s <rom.gb>\n", argv[0]);
        return 1;
    }

    Bus bus;
    Sm83 cpu(bus);

    // Init power-up seq
    // Todo : function init/reset
    cpu.PC = 0x100;
    cpu.SP = 0xFFFE;
    cpu.AF.val = 0x01B0;
    cpu.BC.val = 0x0013;
    cpu.DE.val = 0x00D8;
    cpu.HL.val = 0x014D;

    // Load ROM
    // Todo : function load_rom
    std::FILE *f = std::fopen(argv[1], "rb");
    if (!f)
    {
        std::fprintf(stderr, "Couldn't open ROM file at %s\n", argv[1]);
        std::exit(1);
    }
    std::fread(bus.mem.data(), 1, bus.mem.size(), f);
    std::fclose(f);

    // Todo : remove, only for debug
    for (int i = 0; i < 10000000; ++i)
    {
        cpu.step();

        if (bus.mem[0xFF02] == 0x81)
        {
            std::putchar(bus.mem[0xFF01]);
            bus.mem[0xFF02] = 0;
        }
    }

    return 0;
}