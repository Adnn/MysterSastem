#include "z80.h"

#include "Memory.h"

#include <string>

#include <cassert>


using namespace ad::sms::components;

void setBit(value_8b &aValue, std::size_t aPosition)
{
    aValue |= (0x01 << aPosition);
}

void zeroBit(value_8b &aValue, std::size_t aPosition)
{
    aValue &= ~(0x01 << aPosition);
}

void editBit(value_8b &aValue, std::size_t aPosition, bool aSet)
{
    if(aSet)
    {
        setBit(aValue, aPosition);
    }
    else
    {
        zeroBit(aValue, aPosition);
    }
}

class bitmask
{
private:
    static constexpr std::size_t PATTERN_SIZE = 10; // without final \0

public:
    template <std::size_t N_length>
    constexpr bitmask(const char (&aPattern)[N_length])
    {
        static_assert(N_length==PATTERN_SIZE+1, "The pattern must be of the format: '0bxxxxxxxx'");
        //static_assert(aPattern[0] == '0', "Invalid pattern");
        //static_assert(aPattern[1] == 'b', "Invalid pattern");

        // 'position' is the bit position in the resulting integer
        // it corresponds to the symbol at index 9-position in the pattern string
        for(std::string::size_type position = 0;
            position != 8;
            ++position)
        {
            switch (aPattern[(PATTERN_SIZE-1)-position])
            {
                default:
                    throw std::invalid_argument(std::string{aPattern} + "does not describe a valid bitmask.");
                case '0':
                    mask.first  |= (0x01 << position);
                    mask.second &= ~(0x01 << position);
                    break;
                case '1':
                    mask.first  |= (0x01 << position);
                    mask.second |= (0x01 << position);
                    break;
                case 'x':
                    mask.first  &= ~(0x01 << position);
                    mask.second &= ~(0x01 << position);
                    break;
            }
        }
    }

    bool checkOp(opcode_t aOpcode) const
    {
        return (aOpcode & mask.first) == mask.second;
    }

private:
    // first: bit pattern with 1 for position we want equal, and 0 for positions that are variables
    // second: bit patern with 0 and 1 matching the positions of interest, and also 0 for all variables positions
    std::pair<opcode_t, opcode_t> mask = { {0x00}, {0xFF} };
};


constexpr opcode_t DD   = 0xDD;
/// \todo In the doc (p85), FD is given as 0b11111111, get confirmation of the typo
constexpr opcode_t FD   = 0xFD;
constexpr opcode_t ED   = 0xED;

constexpr bitmask LD_r_r    = {"0b01xxxxxx"};
constexpr bitmask LD_r_n    = {"0b00xxx110"};
// note: ria for 'register indirect addressing'
constexpr bitmask LD_r_ria  = {"0b01xxx110"};
constexpr bitmask LD_ria_r  = {"0b01110xxx"};
constexpr opcode_t LD_ria_n = 0x36;

constexpr opcode_t LD_A_pBC = 0x0A;
constexpr opcode_t LD_A_pDE = 0x1A;
constexpr opcode_t LD_A_pnn = 0x3A;
constexpr opcode_t LD_pBC_A = 0x02;
constexpr opcode_t LD_pDE_A = 0x12;
constexpr opcode_t LD_pnn_A = 0x32;

constexpr opcode_t LD_A_I = 0x57;
constexpr opcode_t LD_A_R = 0x5F;
constexpr opcode_t LD_I_A = 0x47;
constexpr opcode_t LD_R_A = 0x4F;


bool checkOp(opcode_t aOpcode, opcode_t aReference)
{
    return aOpcode == aReference;
}

bool checkOp(opcode_t aOpcode, bitmask aReference)
{
    return aReference.checkOp(aOpcode);
}

struct SpecialState
{
     Prefix prefix = Prefix::None;
};

value_16b &z80::indexRegister(Prefix aPrefix)
{
    switch(aPrefix)
    {
        case Prefix::DD:
            return mIX;
        case Prefix::FD:
            return mIY;
    }
}

value_8b z80::advance()
{
    return mMemory[mPC++];
}

enum Flags
{
    C,
    N,
    PV,
    // unused
    H=PV+2,
    // unused
    Z=H+2,
    S
};

void z80::handleFlag(value_8b aReference)
{
    editBit(mActiveRegisters.F(), S, (asDisplacement(aReference) < 0));
    editBit(mActiveRegisters.F(), Z, (aReference == 0));
    zeroBit(mActiveRegisters.F(), H);
    editBit(mActiveRegisters.F(), PV, mIFF2);
    zeroBit(mActiveRegisters.F(), N);
}

void z80::execute()
{
    SpecialState state;

    /// \todo Read opcode from memory
    opcode_t opcode = advance();

    if (opcode == DD || opcode == FD)
    {
        state.prefix = (opcode == DD ? Prefix::DD : Prefix::FD);
        opcode = advance();
    }

    // MetaData
    // LD ${r_dest}, ${r_source}
    // 1M 4T
    if ( checkOp(opcode, LD_r_r) )
    {
        load(mActiveRegisters.identify8(opcode, Shift::None),
             mActiveRegisters.identify8(opcode, Shift::Third));
    }

    // MetaData
    // LD ${r_dest}, ${n}
    // 2M 7(4,3)T
    else if ( checkOp(opcode, LD_r_n) )
    {
        load(advance(),
             mActiveRegisters.identify8(opcode, Shift::Third));
    }

    else if (checkOp(opcode, LD_r_ria))
    {
        // MetaData
        // LD ${r_dest}, (HL) 
        // 2M 7(4,?6)T
        if (state.prefix == Prefix::None)
        {
            load(mMemory[mActiveRegisters.HL()],
                 mActiveRegisters.identify8(opcode, Shift::Third));
        }

        // MetaData
        // LD ${r_dest}, (${index_r}+${d}) 
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            load(mMemory[indexRegister(state.prefix)+asDisplacement(advance())],
                 mActiveRegisters.identify8(opcode, Shift::Third));
        }
    }

    else if (checkOp(opcode, LD_ria_r))
    {
        // MetaData
        // LD (HL), ${r_source}
        // 2M 7(4,3)T
        if (state.prefix == Prefix::None)
        {
            load(mActiveRegisters.identify8(opcode, Shift::None),
                 mMemory[mActiveRegisters.HL()]);
        }

        // MetaData
        // LD (${index_r}+${d}), ${r_source}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            load(mActiveRegisters.identify8(opcode, Shift::None),
                 mMemory[indexRegister(state.prefix)+asDisplacement(advance())]);
        }
    }

    else if (checkOp(opcode, LD_ria_n))
    {
        // MetaData
        // LD (HL), ${n}
        // 3M 10(4,3,3)T
        if (state.prefix == Prefix::None)
        {
            load(advance(),
                 mMemory[mActiveRegisters.HL()]);
        }

        // MetaData
        // LD (${index_r}+${d}), ${n}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            signed_displacement_8b d = asDisplacement(advance());
            load(advance(),
                 mMemory[indexRegister(state.prefix)+d]);
        }
    }

    else if (checkOp(opcode, LD_A_pBC))
    {
        // MetaData
        // LD A, (BC)
        // 2M 7(4,3)T
        load(mMemory[mActiveRegisters.BC()], mActiveRegisters.A());
    }

    else if (checkOp(opcode, LD_A_pDE))
    {
        // MetaData
        // LD A, (DE)
        // 2M 7(4,3)T
        load(mMemory[mActiveRegisters.DE()], mActiveRegisters.A());
    }

    else if (checkOp(opcode, LD_A_pnn))
    {
        // MetaData
        // LD A, (${nn})
        // 4M 13(4,3,3,3)T
        value_16b nn(advance(), advance());
        load(mMemory[nn], mActiveRegisters.A());
    }

    else if (checkOp(opcode, LD_pBC_A))
    {
        // MetaData
        // LD (BC), A
        // 2M 7(4,3)T
        load(mActiveRegisters.A(), mMemory[mActiveRegisters.BC()]);
    }

    else if (checkOp(opcode, LD_pDE_A))
    {
        // MetaData
        // LD (DE), A
        // 2M 7(4,3)T
        load(mActiveRegisters.A(), mMemory[mActiveRegisters.DE()]);
    }

    else if (checkOp(opcode, LD_pnn_A))
    {
        // MetaData
        // LD (${nn}), A
        // 4M 13(4,3,3,3)T
        value_16b nn(advance(), advance());
        load(mActiveRegisters.A(), mMemory[nn]);
    }

    else if (checkOp(opcode, ED))
    {
        opcode = advance();
        if (checkOp(opcode, LD_A_I))
        {
            // MetaData
            // LD A, I
            // 2M 9(4,5)T
            load(mI, mActiveRegisters.A());
            handleFlag(mI);
            /// \todo "If an interrupt occurs during execution of this instruction, the Parity flag contains a 0."
        }

        else if (checkOp(opcode, LD_A_R))
        {
            // MetaData
            // LD A, R
            // 2M 9(4,5)T
            load(mR, mActiveRegisters.A());
            handleFlag(mR);
            /// \todo "If an interrupt occurs during execution of this instruction, the Parity flag contains a 0."
        }

        else if (checkOp(opcode, LD_I_A))
        {
            // MetaData
            // LD I, A
            // 2M 9(4,5)T
            load(mActiveRegisters.A(), mI);
        }

        else if (checkOp(opcode, LD_R_A))
        {
            // MetaData
            // LD R, A
            // 2M 9(4,5)T
            load(mActiveRegisters.A(), mR);
        }
    }
}
