#ifndef ad_sms_components_TextState_h
#define ad_sms_components_TextState_h


#include "json.h"
#include "Memory.h"
#include "z80.h"

#include <unordered_map>
#include <stack>


namespace ad {
namespace sms {
namespace components {


class Operator_base
{
public:
    void enterSection(const std::string &aJsonKey)
    {}
    void exitSection()
    {}
};

class Printer : public Operator_base
{
public:
    typedef void return_type;

    template <class T_Value>
    return_type operate(T_Value &aMachineValue, const std::string &aJsonKey, const T_Value &aJsonValue)
    {
        // The unary + is a trick to promote potential char types to int
        std::cout << "For key '" << aJsonKey << "' stored value is " << +aMachineValue
                  << " and json value is " << +aJsonValue
                  << std::endl;
    }
};

class Assigner : public Operator_base
{
public:
    typedef void return_type;

    template <class T_Value>
    return_type operate(T_Value &aMachineValue, const std::string &aJsonKey, const T_Value &aJsonValue)
    {
        aMachineValue = aJsonValue;
    }
};

class JsonDumper : public Operator_base
{
public:
    JsonDumper() = default;
    JsonDumper(const JsonDumper &) = delete;
    JsonDumper & operator=(const JsonDumper &) = delete;

    typedef void return_type;

    void enterSection(const std::string &aJsonKey)
    {
        auto emplaceResult = current().emplace(aJsonKey, json::object());
        mStack.emplace(&(*emplaceResult.first));
    }

    void existSection()
    {
        mStack.pop();
    }

    template <class T_Value>
    return_type operate(T_Value &aMachineValue, const std::string &aJsonKey, const T_Value &aJsonValue)
    {
        current().emplace(aJsonKey, aMachineValue);
    }

    const json &dump()
    {
        return mRoot;
    }

private:
    json &current()
    {
        return *mStack.top();
    }

private:
    json mRoot;
    // No initializer list constructors for container adaptors...
    std::stack<json*> mStack{ std::deque<json*>{&mRoot} };
};



template <class T>
struct cpu_value
{
    typedef T type;
};

template <class R> 
struct cpu_value<Register<R>>
{
    typedef R type;
};

template <class T> 
using cpu_value_t = typename cpu_value<T>::type;


void from_json(const json & aJson, value_16b& aValue)
{
    aValue = aJson.get<std::uint16_t>();
}


void to_json(json & aJson, const value_16b& aValue)
{
    aJson = static_cast<std::uint16_t>(aValue);    
}


#define REG(ID) \
    #ID, [&aRegisterSet, &aJson](T_Operator aOperator)                                          \
         {                                                                                      \
             typedef cpu_value_t<std::remove_reference_t<decltype(aRegisterSet.ID())>> value_t;  \
             aOperator.template operate<value_t>                                                \
                              (aRegisterSet.ID(),                                               \
                               #ID,                                                             \
                               aJson.value());                                                  \
         }

template <class T_Operator>
void accessText(T_Operator &&aOperator, RegisterSet &aRegisterSet, json::const_iterator &aJson)
{
    /// \todo Make that static, will prevent from capturint anything in the lambdas
    std::unordered_map<std::string, std::function<void(T_Operator)>> textToValue = {
        { REG(A) },
        { REG(F) },
        { REG(AF) },
        { REG(B) },
        { REG(C) },
        { REG(BC) },
        { REG(D) },
        { REG(E) },
        { REG(DE) },
        { REG(H) },
        { REG(L) },
        { REG(HL) },

        { REG(I) },
        { REG(R) },
        { REG(PC) },
        { REG(IX) },
        { REG(IY) },
        { REG(SP) },
        { REG(IFF1) },
        { REG(IFF2) },
    };

    // Actually lookup in the map, and invoke the corresponding lambda
    textToValue.at(aJson.key())(std::forward<T_Operator>(aOperator));
}

template <class T_Operator>
void accessText(T_Operator &&aOperator, Memory &aMemory, json::const_iterator &aJson)
{
    // 3rd argument 0 to auto-detect the base
    Address address = std::stoi(aJson.key(), 0, 0);
    aOperator.template operate<value_8b>(aMemory[address], aJson.key(), aJson.value());
}


template <class T_Operator, class T_component>
void actSection(T_Operator &&aOperator, const json &aJson, const std::string &aSectionName,
                T_component &aComponent)
{
    json::const_iterator json_section = aJson.find(aSectionName);
    if (json_section != aJson.end())
    {
        aOperator.enterSection(aSectionName);
        for (json::const_iterator it = json_section->begin(); it != json_section->end(); ++it)
        {
            accessText(std::forward<T_Operator>(aOperator), aComponent, it);
        }
        aOperator.exitSection();
    }
}

template <class T_Operator>
typename std::remove_reference_t<T_Operator>::return_type
actOnState(T_Operator &&aOperator, const json &aJson, z80 &aCpu, Memory &aMemory)
{
    actSection(std::forward<T_Operator>(aOperator), aJson, "cpu", aCpu.registers());
    actSection(std::forward<T_Operator>(aOperator), aJson, "memory", aMemory);
}


}}} // namespace ad::sms::components


#endif
