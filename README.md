# CHIP-0x08
An work-in-progress interpreter, assembler, and disassembler for the CHIP-8 instrution set

# Emulator / Interpreter
CHIP-0x08 emulates CHIP-8 instructions in an instruction-accurate manner. CHIP-8 instructions are fixed in length and the ROMS are loaded into a buffer and instructions are executed linearly. The back-end platform layer for the emulator will use SDL to render graphics to the screen. The instructions are defined in a static table within the source code as a macro.

# Disassembly
CHIP-0x08 will provide both Linear Sweep disassembly and Recursive Travel Disassembly.

A linear sweep disassembler for CHIP-8 assembly code files. This class provides a basic linear sweep algorithm that sequentially steps through the ROM file in 2-byte increments, parses the instruction, and dumps the disassembled mnemonics to an output stream. However, in the presence of branch instructions, linear sweep may fail and incorrectly interpret data as opcodes, thus resulting in an incorrect output.

A recursive traversal disassembler for CHIP-8 assembly code files. CHIP-8 has fixed-length instructions that are two-bytes long, consisting of an opcode and data/address bits. This class provides an interface for a recursive traversal disassembler. Instead of disassembling code sequentially, it recursively
steps through control flow of the code when it branches. Essentially, we can diagram the transfer of control flow via branches using a tree, and recursive traversal will traverse that tree depth-first. CHIP-8 supports the following branch instructions:
  * Unconditional branch to machine code routine (typically unused)
  * Unconditional branch to a literal address in memory
  * Unconditional branch to literal address in memory + value of V0
The first two branches are handles trivially, as the algorithm performs a recursive call at the branch target and decodes the instructions, adding them to a map along with their address. For the third case, the value of V0 must be tracked internally through the disassembly, via the instructions that modify it. Then, the branch target can be determined. There are no conditional branches on this architecture, which simplifies the algorithm and makes it more accurate.
