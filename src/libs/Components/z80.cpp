#include "z80.h"

#include "Memory.h"

#include <iostream>
#include <sstream>
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

value_8b z80::fetch()
{
    return mMemory[Address(static_cast<value_16b&>(registers().PC())++)];
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
    editBit(registers().F(), S, (asSigned(aReference) < 0));
    editBit(registers().F(), Z, (aReference == 0));
    zeroBit(registers().F(), H);
    editBit(registers().F(), PV, registers().IFF2());
    zeroBit(registers().F(), N);
}

struct Instruction
{
    unsigned int mOperations;
    unsigned int mClockPeriods;
};

inline void load(value_8b aSource, value_8b &aDestination)
{
    aDestination = aSource;
}

template <class T_source, class T_destination>
class Load : public Instruction
{
public:
    Load(unsigned int aM, unsigned int aT, T_source aSource, T_destination aDestination) :
                Instruction{aM, aT},
            mSource(std::move(aSource)),
            mDestination(std::move(aDestination))
    {}

    void execute() 
    {
        load(mSource, mDestination);
    }

    std::string disassemble()
    {
        std::ostringstream oss;
        oss << "LD " << mDestination << ", " << mSource;
        return oss.str();
    }

//private:
    T_source        mSource;
    T_destination   mDestination;
};

class LoadAndFlag : public Load<Register<value_8b>, Register<value_8b>>
{
    typedef Load<Register<value_8b>, Register<value_8b>> parent_type;

public:
    LoadAndFlag(unsigned int aM, unsigned int aT, Register<value_8b> aSource, Register<value_8b> aDestination,
                z80 &aCpu) :
                Load(aM, aT, std::move(aSource), std::move(aDestination)),
            cpu(aCpu)
    {}

    void execute() 
    {
        parent_type::execute();
        cpu.handleFlag(mSource);
    }

    z80 &cpu;
};

template <class T_source, class T_destination>
Load<T_source, T_destination> makeLoad(unsigned aM, unsigned aT, T_source aSource, T_destination aDestination)
{
    return {aM, aT, std::move(aSource), std::move(aDestination)};
}

LoadAndFlag makeLoadAndFlag(unsigned aM, unsigned aT, Register<value_8b> aSource, Register<value_8b> aDestination,
                            z80 &aCpu)
{
    return {aM, aT, std::move(aSource), std::move(aDestination), aCpu};
}

#define STEP(x) auto inst(x); std::cout << __LINE__ << ": " << inst.disassemble() << std::endl; /*inst.execute();*/ return;

void z80::step()
{
    SpecialState state;

    /// \todo Read opcode from memory
    opcode_t opcode = fetch();

    if (opcode == DD || opcode == FD)
    {
        state.prefix = (opcode == DD ? Prefix::DD : Prefix::FD);
        opcode = fetch();
    }


    /*
     * 8-Bit Load Group
     */
    // MetaData
    // LD ${r_dest}, ${r_source}
    // 1M 4T
    if ( checkOp(opcode, LD_r_r)
      && ((opcode & 0b00000111) != 0b00000110) //otherwise it catches LD_r_ria case...
      && ((opcode & 0b00111000) != 0b00110000) //otherwise it catches LD_ria_r case...
       )
    {
        STEP(makeLoad(1, 4,
                      registers().identify8(opcode, Shift::None),
                      registers().identify8(opcode, Shift::Third))); 
    }

    // MetaData
    // LD ${r_dest}, ${n}
    // 2M 7(4,3)T
    else if ( checkOp(opcode, LD_r_n) 
           && ((opcode & 0b00111000) != 0b00110000) //otherwise it catches LD_ria_n case...
            )
    {
        STEP(makeLoad(2, 7,
                      Immediate<value_8b>{fetch()},
                      registers().identify8(opcode, Shift::Third))); 
    }

    else if (checkOp(opcode, LD_r_ria))
    {
        // MetaData
        // LD ${r_dest}, (HL) 
        // 2M 7(4,?6)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad(2, 7,
                          makeMemoryAccess(mMemory, registers().HL()),
                          registers().identify8(opcode, Shift::Third)));
        }

        // MetaData
        // LD ${r_dest}, (${index_r}+${d}) 
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            STEP(makeLoad(5, 19,
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), asSigned(fetch())}),
                          registers().identify8(opcode, Shift::Third)));
        }
    }

    else if (checkOp(opcode, LD_ria_r))
    {
        // MetaData
        // LD (HL), ${r_source}
        // 2M 7(4,3)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad(2, 7,
                          registers().identify8(opcode, Shift::None),
                          makeMemoryAccess(mMemory, registers().HL())));
        }

        // MetaData
        // LD (${index_r}+${d}), ${r_source}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            STEP(makeLoad(5, 19,
                          registers().identify8(opcode, Shift::None),
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), asSigned(fetch()) })));
        }
    }

    else if (checkOp(opcode, LD_ria_n))
    {
        // MetaData
        // LD (HL), ${n}
        // 3M 10(4,3,3)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad(3, 10,
                          Immediate<value_8b>{ fetch() },
                          makeMemoryAccess(mMemory, registers().HL())));
        }

        // MetaData
        // LD (${index_r}+${d}), ${n}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            signed_8b d = asSigned(fetch());
            STEP(makeLoad(5, 19,
                          Immediate<value_8b>{ fetch() },
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), d })));
        }
    }

    else if (checkOp(opcode, LD_A_pBC))
    {
        // MetaData
        // LD A, (BC)
        // 2M 7(4,3)T
        STEP(makeLoad(2, 7,
                      makeMemoryAccess(mMemory, registers().BC()),
                      registers().A()));
    }

    else if (checkOp(opcode, LD_A_pDE))
    {
        // MetaData
        // LD A, (DE)
        // 2M 7(4,3)T
        STEP(makeLoad(2, 7,
                      makeMemoryAccess(mMemory, registers().DE()),
                      registers().A()));
    }

    else if (checkOp(opcode, LD_A_pnn))
    {
        // MetaData
        // LD A, (${nn})
        // 4M 13(4,3,3,3)T
        value_16b nn(fetch(), fetch());
        STEP(makeLoad(4, 13,
                      makeMemoryAccess(mMemory, nn),
                      registers().A()));
    }

    else if (checkOp(opcode, LD_pBC_A))
    {
        // MetaData
        // LD (BC), A
        // 2M 7(4,3)T
        STEP(makeLoad(2, 7,
                      registers().A(),
                      makeMemoryAccess(mMemory, registers().BC())));
    }

    else if (checkOp(opcode, LD_pDE_A))
    {
        // MetaData
        // LD (DE), A
        // 2M 7(4,3)T
        STEP(makeLoad(2, 7,
                      registers().A(),
                      makeMemoryAccess(mMemory, registers().DE())));
    }

    else if (checkOp(opcode, LD_pnn_A))
    {
        // MetaData
        // LD (${nn}), A
        // 4M 13(4,3,3,3)T
        value_16b nn(fetch(), fetch());
        STEP(makeLoad(4, 13,
                      registers().A(),
                      makeMemoryAccess(mMemory, nn)));
    }

    else if (checkOp(opcode, ED))
    {
        opcode = fetch();
        if (checkOp(opcode, LD_A_I))
        {
            // MetaData
            // LD A, I
            // 2M 9(4,5)T
            STEP(makeLoadAndFlag(2, 9,
                                 registers().I(),
                                 registers().A(),
                                 *this));
            /// \todo "If an interrupt occurs during execution of this instruction, the Parity flag contains a 0."
        }

        else if (checkOp(opcode, LD_A_R))
        {
            // MetaData
            // LD A, R
            // 2M 9(4,5)T
            STEP(makeLoadAndFlag(2, 9,
                                 registers().R(),
                                 registers().A(),
                                 *this));
            /// \todo "If an interrupt occurs during execution of this instruction, the Parity flag contains a 0."
        }

        else if (checkOp(opcode, LD_I_A))
        {
            // MetaData
            // LD I, A
            // 2M 9(4,5)T
            STEP(makeLoad(2, 9,
                          registers().A(), registers().I()));
        }

        else if (checkOp(opcode, LD_R_A))
        {
            // MetaData
            // LD R, A
            // 2M 9(4,5)T
            STEP(makeLoad(2, 9,
                          registers().A(), registers().R()));
        }
    }


    /*
     * 16-Bit Load Group
     */
}
