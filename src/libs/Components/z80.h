#ifndef ad_sms_components_z80_h
#define ad_sms_components_z80_h

#include "DataTypes.h"

#include <iostream>
#include <string>


namespace ad {
namespace sms {
namespace components {

class Memory; // Forward

enum class Prefix
{
    None,
    DD,
    FD
};

typedef unsigned Shift_underlying;
enum class Shift : Shift_underlying
{
    None = 0,
    Third = 3,
};

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

#define PAIRING(X, Y)   \
    Register<value_8b> X()     { return makeRegister(m##X##Y.store[0], #X); } \
    Register<value_8b> Y()     { return makeRegister(m##X##Y.store[1], #Y); } \
    Register<value_16b> X##Y() { return makeRegister(m##X##Y, #X #Y); }

struct RegisterSet
{
    PAIRING(A, F)
    PAIRING(B, C)
    PAIRING(D, E)
    PAIRING(H, L)

    Register<value_8b> identify8(opcode_t aSource, Shift aShift)
    {
        switch (aSource >> static_cast<Shift_underlying>(aShift) & (0b00000111))
        {
            case 0b111:
                return A();
            case 0b000:
                return B();
            case 0b001:
                return C();
            case 0b010:
                return D();
            case 0b011:
                return E();
            case 0b100:
                return H();
            case 0b101:
                return L();
        }
    }

private:
    value_16b mAF;
    value_16b mBC;
    value_16b mDE;
    value_16b mHL;
};

class z80
{
public:
    z80(Memory &aMemory) :
            mMemory(aMemory)
    {}

    void step();
    //void execute();

    Register<value_8b> I()
    {
        return makeRegister(mI, "I");
    }

    Register<value_8b> R()
    {
        return makeRegister(mR, "R");
    }

    Register<value_16b> PC()
    {
        return makeRegister(mPC, "PC");
    }

protected:
    value_8b fetch();
    Register<value_16b> indexRegister(Prefix aPrefix);

public:
    /// \todo Should be protected
    void handleFlag(value_8b aReference);

private:
    /// \todo Check for the initialization values
    value_16b mPC = 0;
    value_16b mIX = 0;
    value_16b mIY = 0;

    /// \todo Check for the initialization values
    value_8b mI = 0;
    value_8b mR = 0; /// \todo What is this register ? How does it behave ?

    /// \todo Check for the initialization values
    flipflop_t mIFF1 = true;
    flipflop_t mIFF2 = true;

    RegisterSet mRegisters;
    RegisterSet mRegistersPrime;
    RegisterSet &mActiveRegisters = mRegisters;

    Memory &mMemory;    
};

}}} // namespace ad::sms::components

#endif
