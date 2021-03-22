_from notes dated 2017-05-02_

# Z80 archi

the Z80 has an 8bit data bus, with a 16bit addressing capacity.
It seems designed to interface with external memory (with the full 16 bit addressing capacity), or with external I/O devices (only the lower half of the address bus should be considered for adressing).
The second mode with IO, is it what the SMS architecture design as "ports" ?

It seems little endian (at least for addressing)

Registers

Accumulator: A Flag: F (x2 A' F') (8bits)

Two set of 6 general purpose registers. May be used individually or as 16 bits register pairs
B C
D E
H L (x2 B' C' ...)

Special purpose registers:
Stack Pointer SP (16)
Program Counter PC (16)
2 index registers IX IY (16 each)
Memory Refresh Register R (8) unclear to me atm. It seems to be used to refresh dynamic memories, but can also be read by the programmer (used in programs for "random" value it seems).
Interrupt Page Address Register I (8) Store the MSB of the indirect address to call in response to an interrupt (interrupting device provides the LSB)

Instruction register: as each instruction is fetched from memory, it is placed there and decoded by the CPU control section.

-- pinout:

-- addressing
Adressing refers to "how the data is accessed"

* Immediate means the operand IS the data value (i.e. a literal)
* Indirect means that the address to the data is held into a register (not "directly" given as operand to the op code)
* Relative means that the operand is a signed displacement from a point of reference (determined by the instruction)
* Implied means that the opcode determines the location of the data (usually a register. eg. Accumulator)

On Z80
* Immediate & Immediate Extendend (resp 8b and 16b values)
* "Modified Page Zero Addressing", for the special RESTART instruction (3bits of the opcode determine which of the 8 hardcoded address to restart from)
* Relative Addressing, relative to the Op Code of the following instruction (allows relocatable code) The displacement is a signed 2's complement number added to A+2
* Extended Addressing is the direct addressing mode: 16bit operand value is the address (little endian)
* Indexed Addressing, the data address is obtained adding signed 2's complement 8b operand to one of the 2 index register. Allow relocatable code. (IX+d) or (IY+d). Nota: it is actually a form of Register Indirect Addressing.
* Register Addressing means the opcode contains bits determining a register
* Implied Addressing (always the same register)
* Register Indirect Addressing means a 16 bit register contains the address to the data. (HL) or (BC)...

Notation: (nn) means "content of memory at nn" (i.e. pointer indirection)


# Instructions:

|Symbol|Field Name|
|---|---|
|C|Carry Flag|
|N|Add/Subtract|
|P/V|Parity/Overflow Flag|
|H|Half Carry Flag|
|Z|Zero Flag|
|S|Sign Flag|
|X|Not Used|

Bits: S Z x H x P/V N C

## Factorisation (to check while covering all instructions)
* DD indexed IX
* FD index IY

* For LD
  * ${r_dest} register id'd by bits [2, 4]
  * ${r_source} register id'd by bits [5, 7]
  * ${n} or ${d} is the 8 bits immediately following. When both are present, d comes before n. ${d} as to be interpreted as a signed 2's complement
  * ${index_r} is given by prefix
    * DD prefix mean indexed on IX
    * FD prefix mean indexed on IY
  * ${nn} is an immediate value, little-endian (low-order byte first)
  * ${dd} and ${qq} is a 16bit register id'd by bits [4, 5]

## Load and Exchange

Load:
LD DEST, SOURCE
PUSH (all pairs, except SP)
POP (all pair, except SP)

Exchange:
Between AF -- A'F' and also between the 3 other pair of registers.
Also some exchange between DE and HL, and (SP) and IX,IY

## Block transfer and search
(HL):source (DE):destination BC: byte counter

Transfer:
LDI (Load Increment): moves one byte, increment HL & DE, decrement BC
LDIR(Load Increment and Repeat): repeats LDI until BC==0
LDD and LDDR are similar but decrement HL & DE

Search:
(HL):source A: reference value, BC: byte counter
CPI (Compare and Increment): compare (HL) to A, store result in one of the flag bits, ++HL, --BC
CPIR (... and Repeat): repeat CPI until either:
* a match is found
* BC reaches 0
CPD and CPDR are similar but decrement HL

## Arithmetic and logical

-- Implementation proposition for addressing modes

load(Value source, Address destination)
swap(Address source, Address destination)

add(Value, Value, Destination)
sub

shift bit both dir


class Value
* ctor from bit patterns with interpretation (eg signed 2 compl...)

class Address
* writeTo
* converts implicitly to Value (read from address)

class Register
* converts implicitly to Value (accessing the value)
* converts implicitly to Address (the address of the register itself)
* `operator*` (or method "indirect()") returns an Address (the address is the value of the register)

class Relative(Value ref, Value offset)

## TODO: interrupt handling
LD A,I: If an interrupt occurs during execution of this instruction, the Parity flag (P/V) contains a 0.

# ERRATUMS
z80 doc:
* p83 T7 != 4+6
* p85 FD != 0b11111111
* p76 N appears twice, but H is not present. Was assumed that bit 4 is the H (and bit 1 is the N).
* p109 LD (nn), HL timing is indicated as 6M 20T, whereas LD HL, (nn) is indicated as 5, 16. Is it a mistake ?
* p113 LD SP, HL. timing is 6M 20T, which is (much) more than the comparable LD SP, IX (2M 10T)
* p117 PUSH IX, M cycles is written 3, but seems to be 4

# Further questions
* What happens when a displacement added to one of the IX or IY register overflows or underflows ?
