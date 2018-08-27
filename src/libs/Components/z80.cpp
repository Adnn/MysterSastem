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

//
// 8-Bit load
//
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

//
// 16-Bit Load
//
constexpr bitmask LD_dd_nn  = {"0b00xx0001"};
constexpr bitmask LD_dd_pnn = {"0b01xx1011"};
constexpr bitmask LD_pnn_dd = {"0b00xx0011"};

constexpr opcode_t LD_idx_nn    = 0x21;
constexpr opcode_t LD_idx_pnn   = 0x2A;

constexpr opcode_t LD_SP_idx   = 0xF9;

constexpr bitmask PUSH_qq = {"0b11xx0101"};
constexpr opcode_t PUSH_idx = 0xE5;

constexpr bitmask POP_qq = {"0b11xx0001"};
constexpr opcode_t POP_idx = 0xE1;

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
     bool   ED = false;
};

value_8b z80::fetch()
{
    return mMemory[Address(static_cast<value_16b&>(registers().PC())++)];
}

Immediate<value_16b> z80::fetchImmediate16()
{
    return Immediate<value_16b>{ {fetch(), fetch()} };
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

struct Timings
{
    int mOperations;
    int mClockPeriods;
};

struct Instruction
{
    Timings mTimings;
};

inline void load(value_8b aSource, value_8b &aDestination)
{
    aDestination = aSource;
}

inline void load(value_16b aSource, value_16b &aDestination)
{
    aDestination = aSource;
}

template <class T_value>
inline void load(value_16b aSource, MemoryAccess<T_value> &aDestination)
{
    aDestination.assign16(aSource);
}

template <class T_source, class T_destination>
class Load : public Instruction
{
public:
    Load(Timings aTimings, T_source aSource, T_destination aDestination) :
                Instruction{aTimings},
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
    LoadAndFlag(Timings aTimings, Register<value_8b> aSource, Register<value_8b> aDestination,
                z80 &aCpu) :
                Load(aTimings, std::move(aSource), std::move(aDestination)),
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
Load<T_source, T_destination> makeLoad(Timings aTimings, T_source aSource, T_destination aDestination)
{
    return {aTimings, std::move(aSource), std::move(aDestination)};
}

LoadAndFlag makeLoadAndFlag(Timings aTimings, Register<value_8b> aSource, Register<value_8b> aDestination,
                            z80 &aCpu)
{
    return {aTimings, std::move(aSource), std::move(aDestination), aCpu};
}

template <const char * N_op>
class StackBase : public Instruction
{
public:
    StackBase(Timings aTimings, Register<value_16b> aOther,
              Memory &aMemory, Register<value_16b> aStackPointer) :
            Instruction{aTimings},
            mOther(std::move(aOther)),
            mMemory(aMemory),
            mStackPointer(aStackPointer)
    {}

    std::string disassemble()
    {
        std::ostringstream oss;
        oss << N_op << mOther;
        return oss.str();
    }

    Register<value_16b> mOther;
    Memory &mMemory;
    value_16b &mStackPointer;
};

char PUSH[] = "PUSH";
class Push : public StackBase<PUSH>
{
public:
    using StackBase::StackBase;

    void execute()
    {
        load(static_cast<value_16b&>(mOther).high(), mMemory[Address{static_cast<std::uint16_t>(mStackPointer-1)}]);
        load(static_cast<value_16b&>(mOther).low(),  mMemory[Address{static_cast<std::uint16_t>(mStackPointer-2)}]);
        static_cast<value_16b&>(mOther) -= 2;
    }
};

char POP[] = "POP";
class Pop : public StackBase<POP>
{
public:
    using StackBase::StackBase;

    void execute()
    {
        load(mMemory[Address{static_cast<std::uint16_t>(mStackPointer)}],   static_cast<value_16b&>(mOther).low());
        load(mMemory[Address{static_cast<std::uint16_t>(mStackPointer+1)}], static_cast<value_16b&>(mOther).high());
        static_cast<value_16b&>(mOther) += 2;
    }
};

//#define STEP(x) auto inst(x); mOs << __LINE__ << ": " << inst.disassemble()/*<< std::endl*/; inst.execute(); return;
#define STEP(x) return aProcess.process(x, __LINE__);

template <class T_process>
typename T_process::return_type z80::step(T_process &aProcess)
{
    SpecialState state;

    /// \todo Read opcode from memory
    opcode_t opcode = fetch();

    if (opcode == DD || opcode == FD)
    {
        state.prefix = (opcode == DD ? Prefix::DD : Prefix::FD);
        opcode = fetch();
    }
    else if (opcode == ED)
    {
        state.ED = true;
        opcode = fetch();
    }


    /*
     * 8-Bit Load Group
     */
    // MetaData
    // LD ${r_dest}, ${r_source}
    // 1M 4T
    if ( checkOp(opcode, LD_r_r) && !state.ED
      && ((opcode & 0b00000111) != 0b00000110) //otherwise it catches LD_r_ria case...
      && ((opcode & 0b00111000) != 0b00110000) //otherwise it catches LD_ria_r case...
       )
    {
        STEP(makeLoad({1, 4},
                      registers().identify8(opcode, Shift::None),
                      registers().identify8(opcode, Shift::Third))); 
    }

    // MetaData
    // LD ${r_dest}, ${n}
    // 2M 7(4,3)T
    if ( checkOp(opcode, LD_r_n) 
           && ((opcode & 0b00111000) != 0b00110000) //otherwise it catches LD_ria_n case...
            )
    {
        STEP(makeLoad({2, 7},
                      Immediate<value_8b>{fetch()},
                      registers().identify8(opcode, Shift::Third))); 
    }

    if (checkOp(opcode, LD_r_ria))
    {
        // MetaData
        // LD ${r_dest}, (HL) 
        // 2M 7(4,?6)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad({2, 7},
                          makeMemoryAccess(mMemory, registers().HL()),
                          registers().identify8(opcode, Shift::Third)));
        }

        // MetaData
        // LD ${r_dest}, (${index_r}+${d}) 
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            STEP(makeLoad({5, 19},
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), asSigned(fetch())}),
                          registers().identify8(opcode, Shift::Third)));
        }
    }

    if (checkOp(opcode, LD_ria_r))
    {
        // MetaData
        // LD (HL), ${r_source}
        // 2M 7(4,3)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad({2, 7},
                          registers().identify8(opcode, Shift::None),
                          makeMemoryAccess(mMemory, registers().HL())));
        }

        // MetaData
        // LD (${index_r}+${d}), ${r_source}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            STEP(makeLoad({5, 19},
                          registers().identify8(opcode, Shift::None),
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), asSigned(fetch()) })));
        }
    }

    if (checkOp(opcode, LD_ria_n))
    {
        // MetaData
        // LD (HL), ${n}
        // 3M 10(4,3,3)T
        if (state.prefix == Prefix::None)
        {
            STEP(makeLoad({3, 10},
                          Immediate<value_8b>{ fetch() },
                          makeMemoryAccess(mMemory, registers().HL())));
        }

        // MetaData
        // LD (${index_r}+${d}), ${n}
        // 5M 19 (4, 4, 3, 5, 3)T
        else
        {
            signed_8b d = asSigned(fetch());
            STEP(makeLoad({5, 19},
                          Immediate<value_8b>{ fetch() },
                          makeMemoryAccess(mMemory, Indexed{ registers().getIndex(state.prefix), d })));
        }
    }

    if (checkOp(opcode, LD_A_pBC))
    {
        // MetaData
        // LD A, (BC)
        // 2M 7(4,3)T
        STEP(makeLoad({2, 7},
                      makeMemoryAccess(mMemory, registers().BC()),
                      registers().A()));
    }

    if (checkOp(opcode, LD_A_pDE))
    {
        // MetaData
        // LD A, (DE)
        // 2M 7(4,3)T
        STEP(makeLoad({2, 7},
                      makeMemoryAccess(mMemory, registers().DE()),
                      registers().A()));
    }

    if (checkOp(opcode, LD_A_pnn))
    {
        // MetaData
        // LD A, (${nn})
        // 4M 13(4,3,3,3)T
        value_16b nn(fetch(), fetch());
        STEP(makeLoad({4, 13},
                      makeMemoryAccess(mMemory, nn),
                      registers().A()));
    }

    if (checkOp(opcode, LD_pBC_A))
    {
        // MetaData
        // LD (BC), A
        // 2M 7(4,3)T
        STEP(makeLoad({2, 7},
                      registers().A(),
                      makeMemoryAccess(mMemory, registers().BC())));
    }

    if (checkOp(opcode, LD_pDE_A))
    {
        // MetaData
        // LD (DE), A
        // 2M 7(4,3)T
        STEP(makeLoad({2, 7},
                      registers().A(),
                      makeMemoryAccess(mMemory, registers().DE())));
    }

    if (checkOp(opcode, LD_pnn_A))
    {
        // MetaData
        // LD (${nn}), A
        // 4M 13(4,3,3,3)T
        value_16b nn(fetch(), fetch());
        STEP(makeLoad({4, 13},
                      registers().A(),
                      makeMemoryAccess(mMemory, nn)));
    }

    if (state.ED)
    {
        if (checkOp(opcode, LD_A_I))
        {
            // MetaData
            // LD A, I
            // 2M 9(4,5)T
            STEP(makeLoadAndFlag({2, 9},
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
            STEP(makeLoadAndFlag({2, 9},
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
            STEP(makeLoad({2, 9},
                          registers().A(), registers().I()));
        }

        else if (checkOp(opcode, LD_R_A))
        {
            // MetaData
            // LD R, A
            // 2M 9(4,5)T
            STEP(makeLoad({2, 9},
                          registers().A(), registers().R()));
        }
    }


    /*
     * 16-Bit Load Group
     */
    if ((state.prefix==Prefix::None) && checkOp(opcode, LD_dd_nn))
    {
        // MetaData
        // LD ${dd}, ${nn}
        // 2M 10(4,3,3)T
        STEP(makeLoad({2, 10},
                      fetchImmediate16(), registers().identify_dd(opcode)));
    }

    if ((state.prefix!=Prefix::None) && checkOp(opcode, LD_idx_nn))
    {
        // MetaData
        // LD ${index_r}, ${nn}
        // 4M 14(4,4,3,3)T
        STEP(makeLoad({4,14},
                      fetchImmediate16(), registers().getIndex(state.prefix)));
    }

    if (checkOp(opcode, LD_idx_pnn))
    {
        if (0b1000 & opcode)
        {
            // if (!state.prefix)
            // MetaData
            // LD HL, (${nn})
            // 5M 16(4,3,3,3,3)T
            
            // if (state.prefix)
            // MetaData
            // LD ${index_r}, (${nn})
            // 6M 20(4,4,3,3,3,3)T
            auto timings = (state.prefix==Prefix::None) ? Timings{6, 20} : Timings{5, 16};
            STEP(makeLoad(timings,
                          makeMemoryAccess(mMemory, fetchImmediate16()),
                          registers().getIndex(state.prefix)));
        }
        else
        {
            // if (!state.prefix)
            // MetaData
            // LD (${nn}), HL
            // 6M 20(4,4,3,3,3,3)T
            
            // if (state.prefix)
            // MetaData
            // LD (${nn}), ${index_r}
            // 6M 20(4,4,3,3,3,3)T
            STEP(makeLoad({6, 20},
                          registers().getIndex(state.prefix),
                          makeMemoryAccess(mMemory, fetchImmediate16())));
        }
    }

    if (state.ED && checkOp(opcode, LD_dd_pnn))
    {
        // MetaData
        // LD ${dd}, (${nn})
        // 6M 20(4,4,3,3,3,3)T
        STEP(makeLoad({6, 20},
                      makeMemoryAccess(mMemory, fetchImmediate16()),
                      registers().identify_dd(opcode)));
    }

    if (state.ED && checkOp(opcode, LD_pnn_dd))
    {
        // MetaData
        // LD (${nn}), ${dd}
        // 6M 20(4,4,3,3,3,3)T
        STEP(makeLoad({6, 20},
                      registers().identify_dd(opcode),
                      makeMemoryAccess(mMemory, fetchImmediate16())));
    }

    if (checkOp(opcode, LD_SP_idx))
    {
        // MetaData
        // LD SP, ${index_r}
        // 2M 10(4,6)T
        STEP(makeLoad({2, 10},
                      registers().identify_dd(opcode),
                      registers().SP()));
    }

    if (state.prefix==Prefix::None && checkOp(opcode, PUSH_qq))
    {
        // MetaData
        // PUSH ${qq}
        // 3M 11(5,3,3)T
        STEP(Push({3, 11},
                  registers().identify_qq(opcode), mMemory, registers().SP()));
    }

    if (state.prefix!=Prefix::None && checkOp(opcode, PUSH_idx))
    {
        // MetaData
        // PUSH ${idx}
        // 4M 15(4,5,3,3)T
        STEP(Push({4, 15},
                  registers().getIndex(state.prefix), mMemory, registers().SP()));
    }

    if (state.prefix==Prefix::None && checkOp(opcode, POP_qq))
    {
        // MetaData
        // POP ${qq}
        // 3M 10(4,3,3)T
        STEP(Pop({3, 10},
                 registers().identify_qq(opcode), mMemory, registers().SP()));
    }

    if (state.prefix!=Prefix::None && checkOp(opcode, POP_idx))
    {
        // MetaData
        // POP ${idx}
        // 4M 14(4,4,3,3)T
        STEP(Pop({4, 14},
                 registers().getIndex(state.prefix), mMemory, registers().SP()));
    }
}

template Disassembler::return_type z80::step<Disassembler>(Disassembler &aProcess);
template Debugger::return_type z80::step<Debugger>(Debugger &aProcess);
