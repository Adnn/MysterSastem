#ifndef ad_sms_components_AddressingModes_h
#define ad_sms_components_AddressingModes_h

#include "DataTypes.h"

#include <iostream>
#include <string>


namespace ad {
namespace sms {
namespace components {

class Memory; // forward

//
// Immediate
//
template <class T_value>
struct Immediate
{
    operator T_value &()
    {
        return value;
    }

    T_value value;
};

template <class T_outputStream, class T_value>
T_outputStream &operator<<(T_outputStream &aOs, const Immediate<T_value> &aImmediate)
{
    return aOs << static_cast<unsigned>(aImmediate.value);
}

//
// Register
//
template <class T_value>
struct Register
{
    operator T_value &()
    {
        return value;
    }

    T_value &value;
    std::string name;
};

template <class T_value>
Register<T_value> makeRegister(T_value &aValue, std::string aName)
{
    return Register<T_value>{ aValue, std::move(aName) };
}

template <class T_outputStream, class T_value>
T_outputStream &operator<<(T_outputStream &aOs, const Register<T_value> &aRegister)
{
    return aOs << aRegister.name;
}

//
// Indexed
//
struct Indexed
{
    operator std::uint16_t()
    {
        /// \todo What happens in the z80 with overflow/underflow ?
        return static_cast<value_16b>(register_16b) + signedDisplacement;
    }

    Register<value_16b> register_16b;
    signed_8b signedDisplacement;
};

template <class T_outputStream>
T_outputStream &operator<<(T_outputStream &aOs, const Indexed &aIndexed)
{
    return aOs << aIndexed.register_16b
               << std::showpos << static_cast<int>(aIndexed.signedDisplacement) << std::noshowpos;
}

//
// MemoryAccess
//
template <class T_value>
struct MemoryAccess
{
    operator value_8b &()
    {
        return memory[Address{address}];
    }

    Memory &memory;
    T_value address;
};

template <class T_value>
MemoryAccess<T_value> makeMemoryAccess(Memory &aMemory, T_value aValue)
{
    return MemoryAccess<T_value>{ aMemory, std::move(aValue) };
}

template <class T_outputStream, class T_value>
T_outputStream &operator<<(T_outputStream &aOs, const MemoryAccess<T_value> &aMemoryAccess)
{
    return aOs << '(' << aMemoryAccess.address << ')';
}


}}} // namespace ad::sms::components

#endif
