#ifndef ad_sms_components_z80_h
#define ad_sms_components_z80_h

#include "RegisterSet.h"
#include "DataTypes.h"


namespace ad {
namespace sms {
namespace components {

class Memory; // Forward

class z80
{
public:
    z80(Memory &aMemory) :
            mMemory(aMemory)
    {}

    //
    // Accessors
    //
    RegisterSet &registers()
    {
        return mRegisters;
    }

    //
    // Operation
    //
    template <class T_process>
    typename T_process::return_type step(T_process &aProcess);
    //void execute();

protected:
    value_8b fetch();

    Immediate<value_16b> fetchImmediate16();

public:
    /// \todo Should be protected
    void handleFlag(value_8b aReference);

private:
    RegisterSet mRegisters;
    Memory &mMemory;    
};


/*
 * Processes
 */
struct Disassembler
{
    typedef void return_type;

template <class T_instruction>
    void process(T_instruction && aInstruction, int aLineNumber)
    {
        mOs << aLineNumber << ": " << aInstruction.disassemble();
    }

    std::ostream &mOs;
};

struct Debugger
{
    typedef void return_type;

    template <class T_instruction>
    void process(T_instruction && aInstruction, int aLineNumber)
    {
        mOs << aLineNumber << ": " << aInstruction.disassemble();
        aInstruction.execute();
    }

    std::ostream &mOs;
};
}}} // namespace ad::sms::components

#endif
