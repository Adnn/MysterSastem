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
    void step();
    //void execute();

protected:
    value_8b fetch();

public:
    /// \todo Should be protected
    void handleFlag(value_8b aReference);

private:
    RegisterSet mRegisters;
    Memory &mMemory;    
};

}}} // namespace ad::sms::components

#endif
