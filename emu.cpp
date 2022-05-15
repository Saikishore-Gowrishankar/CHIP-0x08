/*
 *	CHIP-0x08 Emulator
 *
 *	Copyright 2022 Saikishore Gowrishankar. All rights reserved.
 *	
 *	All owned trademarks belond to their respective owners. Lawyers love tautologies
 */
#include <cstdint> /*Integer definitions*/
#include <cstring>

/*
    Table of Opcodes

    All instructions are 2-bytes long and can be classified by their opcode. The forementioned
    macro (X-macro) serves as documentation for the instruction set, as well as a concise way
    to implement the opcode decoding/executing table (inspired by Bisqwit)

    The instruction bitfields are defined as per the following specification: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Fx07

    nnn or addr - A 12-bit value, the lowest 12 bits of the instruction
    n or nibble - A 4-bit value, the lowest 4 bits of the instruction
    x - A 4-bit value, the lower 4 bits of the high byte of the instruction
    y - A 4-bit value, the upper 4 bits of the low byte of the instruction
    kk or byte - An 8-bit value, the lowest 8 bits of the instruction

    Additionally, the following bitfields are used:
    u - A 4-bit value, the upper 4 bits of the high byte of the instruction (opcode class identifier)
*/
//o("", "", (), ()) /* */\

#define OPCODE_TABLE \
    /* General operations */ \
    o("cls",	   "00E0",  (u == 0x0 && nnn == 0xE0), (PC = nnn;)) /* Clear screen */\
    o("drw Vx, Vy, nibble", "Dxyn", (true), (;)) /* Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision. */\
    /* Branch instructions */\
    o("jp, V0, addr", "Bnnn", (u == 0xB), (PC = nnn;)) /* Jump to location nnn + V0. */\
    o("sys addr",  "0nnn",  (u == 0x0 && nnn != 0xE0), (PC = nnn;)) /* Jump to a machine code routine at nnn.*/\
    o("ret",       "0nEE",  (u == 0x0 && nnn == 0xEE), (PC = nnn;)) /* Return from subroutine */\
    o("jp addr",   "1nnn",  (u == 0x1), 	       (PC = nnn;)) /* Return from subroutine */\
    o("call addr", "2nnn",  (u == 0x2), (PC = nnn;)) /* Return from subroutine */\
    /* Skip instruction */\
    o("se Vx, byte", "3xkk", (true), (;)) /* Skip next instruction if Vx = kk. */\
    o("sne Vx, byte", "4xkk", (true), (;)) /* Skip next instruction if Vx != kk. */\
    o("sne Vx, Vy", "5xy0", (true), (;)) /* Skip next instruction if Vx = Vy. */\
    o("sne Vx, Vy", "9xy0", (true), (;)) /* Skip next instruction if Vx != Vy. */\
    o("skp Vx", "Ex9E", (true), (;)) /* Skip next instruction if key with the value of Vx is pressed. */\
    o("sknp Vx", "ExA1", (true), (;)) /* Skip next instruction if key with the value of Vx is NOT pressed. */\
    o("sne Vx, Vy", "9xy0", (true), (;)) /* Skip next instruction if Vx != Vy. */\
    /* Load */\
    o("ld Vx, Vy", "8xy0", (true), (;)) /* Set Vx = Vy. */\
    o("ld I, addr", "Annn", (true), (;)) /* Set I = nnn. */\
    o("ld Vx, DT", "Fx07", (true), (;)) /* Set Vx = delay timer value. */\
    o("ld Vx, K", "Fx0A", (true), (;)) /* Wait for a key press, store the value of the key in Vx. */\
    o("ld DT, Vx", "Fx15", (true), (;)) /* Set delay timer = Vx. */\
    o("ld ST, Vx", "Fx18", (true), (;)) /* Set sound timer = Vx. */\
    o("ld F, Vx", "Fx29", (true), (;)) /* Set I = location of sprite for digit Vx. */\
    o("ld B, Vx", "Fx33", (true), (;)) /* Store BCD representation of Vx in memory locations I, I+1, and I+2. */\
    o("ld [I], Vx", "Fx55", (true), (;)) /* Store registers V0 through Vx in memory starting at location I. */\
    o("ld Vx, [I]", "Fx65", (true), (;)) /* Read registers V0 through Vx from memory starting at location I. */\
    o("ld Vx, byte", "6xkk", (true), (;)) /* Set Vx = kk */\
    /* Arithmetic */\
    o("add Vx, byte", "7xkk", (true), (;)) /* Set Vx = Vx + kk. */\
    o("add Vx, Vy", "8xy4", (true), (;)) /* Set Vx = Vx + Vy, set VF = carry. */\
    o("add i, Vx", "Fx1E", (true), (;)) /* Set I = I + Vx. */\
    o("sub Vx, Vy", "8xy5", (true), (;)) /* Set Vx = Vx - Vy, set VF = NOT borrow. */\
    o("subn Vx, Vy", "8xy7", (true), (;)) /* Set Vx = Vy - Vx, set VF = NOT borrow */\
    /* Bitwise operations */\
    o("or Vx, Vy", "8xy1", (true), (;)) /* Set Vx = Vx OR Vy. */\
    o("and Vx, Vy", "8xy2", (true), (;)) /* Set Vx = Vx AND Vy. */\
    o("xor Vx, Vy", "8xy3", (true), (;)) /* Set Vx = Vx XOR Vy. */\
    o("shr Vx {, Vy}", "8xy6", (true), (;)) /* Set Vx = Vx SHR 1. */\
    o("shl Vx {, Vy}", "8xyE", (true), (;)) /* Set Vx = Vx SHL 1. */\
    o("rnd Vx, byte", "Cxkk", (true), (;)) /* Set Vx = random byte AND kk. */\

class Chip8
{
public:
    Chip8();
private:
    union
    {
        // CHIP-8 only accesses 4KB of RAM. In implementations using CHIP-8, such as COSMAC VIP,
        // There is up to 64KB but only 4KB is accessible.
        uint8_t RAM[0x1000];
        struct
        {
            uint8_t Reserved[0x200]; //Reserved for Interpreter
            uint8_t User[0xCA0]      //User Program space (3232 bytes, as per COSMAC VIP spec)
            uint8_t Stack[0x30]      //Stack (48 bytes, up to 12 levels of nested subroutines)
            uint8_t Mem[0x20];       //Interpreter work area
            uint8_t V[0x10];         //General purpose registers V0 - VF
            uint8_t VRAM[0x100];     //Display memory
        };
    };

    //Special purpose registers (not stored in RAM to follow COSMAC documentation)
    uint16_t PC;                  /* Instruction pointer */
    uint16_t I;                   /* Store address       */
    uint8_t SP;			  /* Stack pointer       */
    uint8_t Delay, uint8_t Sound; /* Timer registers     */
};[A