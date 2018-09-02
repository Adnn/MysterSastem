#include <Components/Memory.h>
#include <Components/TextState.h>
#include <Components/z80.h>

#include "3rdparty/catch.hpp"

#include <regex>
#include <sstream>


using namespace ad::sms;


static const json gRefJson = {
    {"cpu", {
        {"B", 0xAB},
        {"A", 0xAA},
        {"C", 0xAC},
        {"HL", 0xBACA},
        {"IFF1", false},
        {"IFF2", true},
    }},
    {"memory", {
        {"0x14A4", 0x15},
        {"8", 0x15},
    }},
};


TEST_CASE("Text operators")
{
    components::SmsMemory smsMemory;
    components::z80 cpu(smsMemory.memory());

    SECTION("Assigner")
    {
        components::Assigner assigner;

        SECTION("can modify the cpu state")
        {
            actOnState(assigner, gRefJson, cpu, smsMemory.memory());

            json::const_reference cpuRef = gRefJson["cpu"];
            CHECK(cpu.registers().A().value == cpuRef["A"]);
            CHECK(cpu.registers().B().value == cpuRef["B"]);
            CHECK(cpu.registers().HL().value == cpuRef["HL"]);
            CHECK(cpu.registers().IFF1() == cpuRef["IFF1"]);
            CHECK(cpu.registers().IFF2() == cpuRef["IFF2"]);
        }

        SECTION("can modify the memory state")
        {
            actOnState(assigner, gRefJson, cpu, smsMemory.memory());

            json::const_reference memoryRef = gRefJson["memory"];
            CHECK(smsMemory.memory()[0x14A4] == memoryRef["0x14A4"]);
            CHECK(smsMemory.memory()[8] == memoryRef["8"]);
        }
    } 

    SECTION("JsonDumper")
    {
        components::Assigner assigner;
        components::JsonDumper dumper;

        actOnState(assigner, gRefJson, cpu, smsMemory.memory());
        actOnState(dumper, gRefJson, cpu, smsMemory.memory());

        REQUIRE(dumper.dump() == gRefJson);
    }
}

