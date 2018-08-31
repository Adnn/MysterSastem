#ifndef ad_sms_components_RegisterSet_h
#define ad_sms_components_RegisterSet_h

#include "AddressingModes.h"
#include "DataTypes.h"


namespace ad {
namespace sms {
namespace components {

#define PAIRING(X, Y, route)   \
    Register<value_8b> X()     { return makeRegister(mActiveRegisters.route.m##X##Y.high(), #X); } \
    Register<value_8b> Y()     { return makeRegister(mActiveRegisters.route.m##X##Y.low(), #Y); } \
    Register<value_16b> X##Y() { return makeRegister(mActiveRegisters.route.m##X##Y, #X #Y); }

struct GeneralRegisters
{
    value_16b mBC;
    value_16b mDE;
    value_16b mHL;
};

struct RegisterSet
{
    PAIRING(A, F, mArithmeticRegisters)
    PAIRING(B, C, mGeneralRegisters)
    PAIRING(D, E, mGeneralRegisters)
    PAIRING(H, L, mGeneralRegisters)

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

    Register<value_16b> IX()
    {
        return makeRegister(mIX, "IX");
    }

    Register<value_16b> IY()
    {
        return makeRegister(mIY, "IY");
    }

    Register<value_16b> SP()
    {
        return makeRegister(mSP, "SP");
    }

    flipflop_t &IFF1()
    {
        return mIFF1;
    }

    flipflop_t &IFF2()
    {
        return mIFF2;
    }

    Register<value_8b> identify8(opcode_t aSource, Shift aShift)
    {
        switch (aSource >> static_cast<Shift_underlying>(aShift) & (0b00000111))
        {
            default:
                throw std::invalid_argument("Unhandled value.");
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

    /// \brief Identify a 16 bit register from bits 4 and 5 of an opcode.
    /// This mapping is referred to as 'dd' in the manual.
    Register<value_16b> identify_dd(opcode_t aSource)
    {
        switch (aSource >> 4 & (0b0000011))
        {
            default:
                throw std::logic_error("Control cannot reach here.");
            case 0b00:
                return BC();
            case 0b01:
                return DE();
            case 0b10:
                return HL();
            case 0b11:
                return SP();
        }
    }

    /// \brief Identify a 16 bit register from bits 4 and 5 of an opcode.
    /// This mapping is referred to as 'qq' in the manual.
    Register<value_16b> identify_qq(opcode_t aSource)
    {
        switch (aSource >> 4 & (0b0000011))
        {
            default:
                throw std::logic_error("Control cannot reach here.");
            case 0b00:
                return BC();
            case 0b01:
                return DE();
            case 0b10:
                return HL();
            case 0b11:
                return AF();
        }
    }

    Register<value_16b> getIndex(Prefix aPrefix)
    {
        switch(aPrefix)
        {
            case Prefix::None:
                return HL();
            case Prefix::DD:
                return IX();
            case Prefix::FD:
                return IY();
        }
    }

private:
    value_16b mAF;
    value_16b mAFPrime;

    GeneralRegisters mGeneralRegisters;
    GeneralRegisters mGeneralRegistersPrime;

    struct
    {
        struct { // extra structure, so access to AF requires the same number of indirections than other pairs.
            value_16b        &mAF;
        } mArithmeticRegisters;
        GeneralRegisters &mGeneralRegisters;
    } mActiveRegisters { {mAF}, mGeneralRegisters};

    /// \todo Check for the initialization values
    value_16b mPC = 0;
    value_16b mIX = 0;
    value_16b mIY = 0;
    value_16b mSP = 0;

    /// \todo Check for the initialization values
    value_8b mI = 0;
    value_8b mR = 0; /// \todo What is this register ? How does it behave ?

    /// \todo Check for the initialization values
    flipflop_t mIFF1 = true;
    flipflop_t mIFF2 = true;
};


}}} // namespace ad::sms::components

#endif
