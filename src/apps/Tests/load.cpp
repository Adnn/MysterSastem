#include <Components/Memory.h>
#include <Components/z80.h>

#include "3rdparty/catch.hpp"

#include <regex>

using namespace ad::sms;

/// \todo should be possible to give const segments to Memory
/*const*/ components::value_8b program[] =
{
    0b01111000, // LD A, B
    0b01001100, // LD C, H
    0b01100001, // LD H, C
    0b01010011, // LD D, E
    0b01101101, // LD L, L /// \todo does that work ?

    0b00010110, 0b10000000, // LD D, 128
    0b00010110, 0b00000110, // LD D, 6 

    0b01010110, // LD D, (HL)
    0xDD, 0b01011110, 0b11111111, // LD E, (IX-1)
    0xFD, 0b01011110, 0b01111111, // LD E, (IY+127)

    0b01110101, // LD (HL), L
    0xDD, 0b01110000, 0b11001101, // LD (IX-51), B
    0xFD, 0b01110000, 0b00110011, // LD (IY+51), B

    0x36, 0b00110011, // LD (HL), 51
    0x36, 0b11001101, // LD (HL), 205 
    0xDD, 0x36, 0b11001101, 0b11001101, // LD (IX-51), 205 
    0xFD, 0x36, 0b00000000, 0b11111111, // LD (IY+0), 255 

    0x0A, // LD A, (BC)
    0x1A, // LD A, (DE)
    0x3A, 0b00000001, 0b10000000, // LD A, (32769)

    0x02, // LD (BC), A
    0x12, // LD (DE), A
    0x32, 0b11111111, 0b00000000, // LD (255), A

    0xED, 0x57, // LD A, I
    0xED, 0x5F, // LD A, R
    0xED, 0x47, // LD I, A
    0xED, 0x4F, // LD R, A
};

const std::string mnemonics[] = {
    "LD A, B",
    "LD C, H",
    "LD H, C",
    "LD D, E",
    "LD L, L",

    "LD D, 80H",
    "LD D, 6H",

    "LD D, (HL)",
    "LD E, (IX-1)",
    "LD E, (IY+127)",

    "LD (HL), L",
    "LD (IX-51), B",
    "LD (IY+51), B",

    "LD (HL), 33H",
    "LD (HL), CDH",
    "LD (IX-51), CDH",
    "LD (IY+0), FFH",

    "LD A, (BC)",
    "LD A, (DE)",
    "LD A, (32769)",

    "LD (BC), A",
    "LD (DE), A",
    "LD (255), A",

    "LD A, I",
    "LD A, R",
    "LD I, A",
    "LD R, A",
};

constexpr std::uint16_t programSize = sizeof(program)/sizeof(components::value_8b);

struct Parsed
{
    std::string disassembly;
    int line;
};

Parsed parseDisassembly(const std::string & aDisassembly)
{
    static const std::regex regex(R"#((\d+): (.+))#");
    std::smatch matches;

    std::regex_match(aDisassembly, matches, regex);
    return {matches[2], std::stoi(matches[1])};
}

TEST_CASE("8-Bit load instructions are correctly disassembled", "[disassembler]")
{
    std::array<components::value_8b, 8*1024> workRam;
    components::Memory memory(program, workRam.data());
    components::z80 cpu(memory);
    std::size_t mnemonicId = 0;

    std::regex regex(R"#((\d+): (.+))#");
    std::smatch matches;

    std::ostringstream oss;
    components::Disassembler disassembler{oss};

    while (static_cast<components::value_16b>(cpu.registers().PC()) != programSize)
    {
        oss.str("");

        cpu.step(disassembler);

        std::string disassembly = oss.str();
        std::regex_match(disassembly, matches, regex);

        INFO("Was decoded at line: " + matches[1].str())
        REQUIRE(mnemonics[mnemonicId++] == matches[2]);
    }
}


struct TestLine
{
    std::vector<components::value_8b> machineCode;
    std::string disassembly;
    std::function<bool(components::Memory &, components::z80 &)> functionalTest;
    std::function<void(components::Memory &, components::z80 &)> preliminary
        = [](components::Memory &memory, components::z80 &cpu){};
};

#define TST [](components::Memory &memory, components::z80 &cpu)
#define PRE [](components::Memory &memory, components::z80 &cpu)

std::vector<TestLine> Load16Bits = {
    {
        {0b00100001, 0x00, 0x50},
        "LD HL, 5000H",
        TST{
            return cpu.registers().HL().value == 0x5000;
        }
    },
    {
        {0xDD, 0x21, 0xA2, 0x45},
        "LD IX, 45A2H",
        TST{
            return cpu.registers().IX().value == 0x45A2;
        }
    },
    {
        {0xFD, 0x21, 0x33, 0x77},
        "LD IY, 7733H",
        TST{
            return cpu.registers().IY().value == 0x7733;
        }
    },
    {
        {0x2A, 0x45, 0x45},
        "LD HL, (4545H)",
        TST{
            return cpu.registers().HL().value == 0xA137;
        },
        PRE{
            memory[0x4545] = 0x37;
            memory[0x4546] = 0xA1;
        }
    },
    {
		{0xED, 0b01001011, 0x30, 0x21},
        "LD BC, (2130H)",
        TST{
            return cpu.registers().BC().value == 0x7865;
        },
        PRE{
            memory[0x2130] = 0x65;
            memory[0x2131] = 0x78;
        }
    },
    {
        {0xDD, 0x2A, 0x66, 0x66},
        "LD IX, (6666H)",
        TST{
            return cpu.registers().IX().value == 0xDA92;
        },
        PRE{
            memory[0x6666] = 0x92;
            memory[0x6667] = 0xDA;
        }
    },
    {
        {0xFD, 0x2A, 0x66, 0x66},
        "LD IY, (6666H)",
        TST{
            return cpu.registers().IY().value == 0xDA92;
        },
        PRE{
            memory[0x6666] = 0x92;
            memory[0x6667] = 0xDA;
        }
    },
};

TEST_CASE("16-bit load instructions ...")
{
    std::array<components::value_8b, 8*1024> workRam;
    //components::Memory memory(nullptr, workRam.data());
    //components::z80 cpu(memory);

    std::ostringstream oss;
    components::Debugger debugger{oss};

    for(TestLine & line : Load16Bits)
    {
        oss.str("");
        components::Memory memory = components::Memory(line.machineCode.data(), workRam.data());
        components::z80 cpu(memory);

        line.preliminary(memory, cpu);

        cpu.step(debugger);

        Parsed parsed = parseDisassembly(oss.str());
        INFO("Was decoded at line: " + std::to_string(parsed.line))
        REQUIRE(line.disassembly == parsed.disassembly);

        REQUIRE(line.functionalTest(memory, cpu));
    }
}
