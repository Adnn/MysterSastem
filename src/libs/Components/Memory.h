#ifndef ad_sms_components_Memory_h
#define ad_sms_components_Memory_h

#include "DataTypes.h"


namespace ad {
namespace sms {
namespace components {

/// \brief Represents memory with a 16bits address bus and an 8bits data bus.
class Memory
{
    static constexpr int gRamStart = 0xC000;

public:
    Memory(value_8b *aSlotData, value_8b *aWorkRam) :
            mSlotData(aSlotData),
            mWorkRam(aWorkRam)
    {}

    value_8b &operator[](Address aAddress)
    {
        if (aAddress.mValue < gRamStart)
        {
            return mSlotData[aAddress.mValue];
        }
        else
        {
            return mWorkRam[aAddress.mValue];
        }
    }

private:
    value_8b *mSlotData;
    value_8b *mWorkRam;

};

}}} // namespace ad::sms::components

#endif
