#ifndef ad_sms_components_z80_h
#define ad_sms_components_z80_h

#include <array>


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

    value_16b &operator++(int)
    {
        std::uint16_t tmp(*this);
        *this = value_16b(++tmp);
        return *this;
    }

    std::array<value_8b, 2> store = {0, 0};
};

typedef bool flipflop_t;

/// \brief A displacement, represented as a 2's complement signed number on 8bits.
typedef std::int8_t signed_displacement_8b;

/// \todo Confirm that this function is not with UB
signed_displacement_8b asDisplacement(value_8b aData)
{
    // is this violating the aliasing rule, as the destination is not necesarilly char or unsigned char ?
    //return reinterpret_cast<signed_displacement_8b &>(aData);
    
    signed_displacement_8b result;
    *reinterpret_cast<unsigned char*>(&result) = aData;
    return result;
}

#define PAIRING(X, Y)   \
    value_8b &X()     { return m##X##Y.store[0]; } \
    value_8b &Y()     { return m##X##Y.store[1]; } \
    value_16b &X##Y() { return m##X##Y; }

typedef unsigned Shift_underlying;
enum class Shift : Shift_underlying
{
    None = 0,
    Third = 3,
};

struct RegisterSet
{
    PAIRING(A, F)
    PAIRING(B, C)
    PAIRING(D, E)
    PAIRING(H, L)

    value_8b &identify8(opcode_t aSource, Shift aShift)
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

inline void load(value_8b aSource, value_8b &aDestination)
{
    aDestination = aSource;
}


class z80
{
public:
    z80(Memory &aMemory) :
            mMemory(aMemory)
    {}

    void execute();

protected:
    value_8b advance();
    value_16b &indexRegister(Prefix aPrefix);

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
