#include "z80.h"

using namespace ad::sms::components;

constexpr opcode_t LD_r_r = 0b01000000;

void z80::execute(Memory &aMemory)
{
    /// \todo Read opcode from memory
    opcode_t opcode = 0;

    if ( (opcode & LD_r_r) == LD_r_r)
    {
       load(mActiveRegisters.identify8(opcode, Shift::None),
            mActiveRegisters.identify8(opcode, Shift::Third));
    }
}
