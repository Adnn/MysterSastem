#ifndef ad_sms_components_z80_h
#define ad_sms_components_z80_h

#include <array>


namespace ad {
namespace sms {
namespace components {

class Memory; // Forward

typedef unsigned char       opcode_t;

typedef unsigned char           value_8b;
typedef std::array<value_8b, 2> value_16b;

//typedef std::int_fast8_t    int_8b;
//typedef std::int_fast16_t   int_16b;

//typedef std::int_fast8_t    register_8bits;
//typedef std::int_fast16_t   register_16bits;


//union RegisterPair
//{
//    struct
//    {
//        register_8bits left;
//        register_8bits right;
//    }
//    register_16bits pair;
//}

//struct Registers
//{
//
//    register_8bits &B()
//    {
//        return BC.left;
//    }
//
//    register_8bits &C()
//    {
//        return BC.right;
//    }
//
//    register_16bits &BC()
//    {
//        return BC.pair;
//    }
//private:
//    RegisterPair BC;
//}

#define PAIRING(X, Y)   \
    value_8b &X()     { return m##X##Y[0]; } \
    value_8b &Y()     { return m##X##Y[1]; } \
    value_16b &X##Y() { return m##X##Y; }

typedef unsigned Shift_underlying;
enum class Shift : Shift_underlying
{
    None = 0,
    Third = 3,
};

struct RegisterSet
{
    //value_8b &B()
    //{ return BC[0]; }

    //value_8b &C()
    //{ return BC[1]; }

    //value_16b &BC()
    //{ return BC.pair; }
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

//class RegisterIdentifier
//{
//public:
//    RegisterIdentifier(opcode_t aValue, unsigned int shift) :
//        mId(aValue >> shift & (0b00000111))
//    {}
//
//private:
//    std::bitset<3> mId;
//};

inline void load(value_8b aSource, value_8b &aDestination)
{
    aDestination = aSource;
}


class z80
{
public:
    void execute(Memory &aMemory);

protected:

private:
    RegisterSet mRegisters;
    RegisterSet mRegistersPrime;
    RegisterSet &mActiveRegisters = mRegisters;
};

}}} // namespace ad::sms::components

#endif
