
#include "sm83.hpp"
#include "bus.hpp"

int main(void)
{
    Bus bus;
    Sm83 cpu(bus);

    // Todo : load ROM

    return 0;
}