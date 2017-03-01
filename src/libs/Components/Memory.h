#ifndef ad_sms_components_Memory_h
#define ad_sms_components_Memory_h

#include "DataTypes.h"


namespace ad {
namespace sms {
namespace components {

/// \brief Represents memory with a 16bits address bus and an 8bits data bus.
class Memory
{
public:
    Memory(value_8b *aData) :
            mDataStore(aData)
    {}

    value_8b &operator[](Address aAddress)
    {
        return mDataStore[aAddress.mValue];
    }

private:
    value_8b *mDataStore;
};

}}} // namespace ad::sms::components

#endif
