#ifndef ad_sms_components_DataTypes_h
#define ad_sms_components_DataTypes_h

#include <array>


namespace ad {
namespace sms {
namespace components {

typedef unsigned char       opcode_t;

typedef unsigned char           value_8b;
//typedef std::array<value_8b, 2> value_16b;

/// \todo Confirm that this class is not with UB
/// We store it with the most significant byte first, even though the architecture seems little endian.
struct value_16b
{
    value_16b() = default;

    value_16b(std::uint16_t aInitial) :
        store({static_cast<value_8b>(aInitial >> 8),
               static_cast<value_8b>(aInitial & 0x00FF)})
    {}

    value_16b(value_8b aLeastSignificant, value_8b aMostSignificant) :
        store({static_cast<value_8b>(aMostSignificant),
               static_cast<value_8b>(aLeastSignificant)})
    {}

    operator std::uint16_t() const 
    {
        return (store[0] << 8) | (store[1]);
    }

    value_16b operator++(int)
    {
        std::uint16_t tmp(*this);
        *this = value_16b(tmp+1);
        return tmp;
    }

    std::array<value_8b, 2> store = {0, 0};
};

typedef bool flipflop_t;

/// \brief Signed value represented as a 2's complement signed number on 8bits.
/// Notably usefull to represent displacement from index registers.
typedef std::int8_t signed_8b;

/// \todo Confirm that this function is not with UB
inline signed_8b asSigned(value_8b aData)
{
    // is this violating the aliasing rule, as the destination is not necesarilly char or unsigned char ?
    //return reinterpret_cast<signed_displacement_8b &>(aData);
    
    signed_8b result;
    *reinterpret_cast<unsigned char*>(&result) = aData;
    return result;
}

class Address
{
    friend class Memory;

public:
    explicit Address(std::uint16_t aValue) :
            mValue(aValue)
    {}

    explicit Address(value_16b aValue) :
            mValue(aValue)
    {}

private:
    std::uint16_t mValue;
};

}}} // namespace ad::sms::components

#endif
