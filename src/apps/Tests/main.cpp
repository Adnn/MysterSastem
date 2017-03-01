#include <Components/Memory.h>
#include <Components/z80.h>

using namespace ad::sms;

/// \todo should be possible to give const segments to Memory
/*const*/ components::value_8b program[] =
{
    0b01111000, // LD A, B
    0b01001100, // LD C, H
    0b01100001, // LD H, C
    0b01010011, // LD D, E
    0b01101101, // LD L, L /// \todo does that work ?

    0b00010110, 0b10000000, // LD D, 128
    0b00010110, 0b00000110, // LD D, 6 

    0b01010110, // LD D, (HL)
    0xDD, 0b01011110, 0b11111111, // LD E, (IX-1)
    0xFD, 0b01011110, 0b01111111, // LD E, (IY+127)

    0b01110101, // LD (HL), L
    0xDD, 0b01110000, 0b11001101, // LD (IX-51), B
    0xFD, 0b01110000, 0b00110011, // LD (IY+51), B

    0x36, 0b00110011, // LD (HL), 51
    0x36, 0b11001101, // LD (HL), 205 
    0xDD, 0x36, 0b11001101, 0b11001101, // LD (IX-51), 205 
    0xFD, 0x36, 0b00000000, 0b11111111, // LD (IX+0), 255 

    0x0A, // LD A, (BC)
    0x1A, // LD A, (DE)
    0x3A, 0b00000001, 0b10000000, // LD A, (32769)

    0x02, // LD (BC), A
    0x12, // LD (DE), A
    0x32, 0b11111111, 0b00000000, // LD (255), A

    0xED, 0x57, // LD A, I
    0xED, 0x5F, // LD A, R
    0xED, 0x47, // LD I, A
    0xED, 0x4F, // LD R, A
};

constexpr std::uint16_t programSize = sizeof(program)/sizeof(components::value_8b);

int main(int argc, char **argv)
{
    components::Memory memory(program);
    components::z80 cpu(memory);

    while (static_cast<components::value_16b>(cpu.registers().PC()) != programSize)
    {
        cpu.step();
    }

    return 0;
}
