
/*

vRISC rel 0 - very Restricted Instruction Set

	REGISTERS
A,B,C,D,E,F,G,H		General Purpose
S					Segment register
T					Temp register
(utils)
P0					Program segment
P1					Program counter

	OPCODES
(mov)
00 NN:		T = [N = number]
01 AB:		A = B

(math)
02 AB:		A += B
03 AB:		A -= B
04 AB:		A *= B
05 AB:		A /= B
06 AB:		A %= B

(bitwise)
07 AB:		A &= B
08 AB:		A |= B
09 AB:		A ^= B
0A AB:		A ~= B
0B AB:		A<<= B
0C AB:		A>>= B

(logic)
0D AB:		T = (A == B)
0E AB:		T = (A > B)

(conditions)
0F NN:		Jump to NN on segment S if (T != 0)

(memory)
10 NN:		T = memory[(256 * S) NN]     read memory on segment S
11 NN:		memory[(256 * S) + NN] = T   write memory on segment S

	MEMORY
There are 65536 bytes of memory (256 segmenst of 256 bytes).
The first 1024 bytes are reserved for the screen buffer,
then two bytes are used for input; the rest of the memory is free.

[01024] Screen
[00002] Input
[00002] Program size
[64508] Program and work memory

*/

#include <stdio.h>
#include <stdint.h>

typedef uint8_t u8;
typedef uint16_t u16;


/* "HARDWARE" */

// Registers
typedef enum
{
	REG_A = 0,
	REG_B = 1,
	REG_C = 2,
	REG_D = 3,
	REG_E = 4,
	REG_F = 5,
	REG_G = 6,
	REG_H = 7,
	REG_S = 8,
	REG_T = 9,
	REG_P0 = 10,
	REG_P1 = 11
} REG_t;

u8 reg[10];	// 10 registers ain't enough right?

// Memory
u8 mem[256][256];

// Opcode table
typedef void (*opcode)(u8 arg);
opcode opcode_table[0xFF];

/* OPCODEs implementation */

#define ARG_R1 (arg >> 4)
#define ARG_R2 (arg & 0xf)

void OPCODE_00(u8 arg) {
	reg[REG_T] = arg;
}
void OPCODE_01(u8 arg) {
	reg[ARG_R1] = reg[ARG_R2];
}
void OPCODE_02(u8 arg) {
	reg[ARG_R1] += reg[ARG_R2];
}
void OPCODE_03(u8 arg) {
	reg[ARG_R1] -= reg[ARG_R2];
}
void OPCODE_04(u8 arg) {
	reg[ARG_R1] *= reg[ARG_R2];
}
void OPCODE_05(u8 arg) {
	reg[ARG_R1] /= reg[ARG_R2];
}
void OPCODE_06(u8 arg) {
	reg[ARG_R1] %= reg[ARG_R2];
}
void OPCODE_07(u8 arg) {
	reg[ARG_R1] &= reg[ARG_R2];
}
void OPCODE_08(u8 arg) {
	reg[ARG_R1] |= reg[ARG_R2];
}
void OPCODE_09(u8 arg) {
	reg[ARG_R1] ^= reg[ARG_R2];
}
void OPCODE_0A(u8 arg) {
	reg[ARG_R1] ~= reg[ARG_R2];
}
void OPCODE_0B(u8 arg) {
	reg[ARG_R1] <<= reg[ARG_R2];
}
void OPCODE_0C(u8 arg) {
	reg[ARG_R1] >>= reg[ARG_R2];
}
void OPCODE_0D(u8 arg) {
	reg[REG_T] = (reg[ARG_R1] == reg[ARG_R2]);
}
void OPCODE_0E(u8 arg) {
	reg[REG_T] = (reg[ARG_R1] > reg[ARG_R2]);
}
void OPCODE_0F(u8 arg) {
	if (reg[REG_T]) {reg[REG_P0] = reg[REG_S]; reg[REG_P1] = arg;};
}
void OPCODE_10(u8 arg) {
	reg[REG_T] = mem[reg[REG_S]][arg];
}
void OPCODE_11(u8 arg) {
	mem[reg[REG_S]][arg] = reg[REG_T];
}

/* EMULATOR core */

void tinyVM_setup(u8 * program, u16 size)
{
	// opcodes
	memset(opcode_table, 0, sizeof(opcode_table));
	opcode_table[0x00] = (opcode)&OPCODE_00;
	opcode_table[0x01] = (opcode)&OPCODE_01;
	opcode_table[0x02] = (opcode)&OPCODE_02;
	opcode_table[0x03] = (opcode)&OPCODE_03;
	opcode_table[0x04] = (opcode)&OPCODE_04;
	opcode_table[0x05] = (opcode)&OPCODE_05;
	opcode_table[0x06] = (opcode)&OPCODE_06;
	opcode_table[0x07] = (opcode)&OPCODE_07;
	opcode_table[0x08] = (opcode)&OPCODE_08;
	opcode_table[0x09] = (opcode)&OPCODE_09;
	opcode_table[0x0A] = (opcode)&OPCODE_0A;
	opcode_table[0x0B] = (opcode)&OPCODE_0B;
	opcode_table[0x0C] = (opcode)&OPCODE_0C;
	opcode_table[0x0D] = (opcode)&OPCODE_0D;
	opcode_table[0x0E] = (opcode)&OPCODE_0E;
	opcode_table[0x0F] = (opcode)&OPCODE_0F;
	opcode_table[0x10] = (opcode)&OPCODE_10;
	opcode_table[0x11] = (opcode)&OPCODE_11;
	
	// memory
	memset((u8*)mem, 0, sizeof(mem));
	
	// registers
	memset(reg, 0, sizeof(reg));
	
	// copy the program into memory
	mem[4][254] = size >> 8;
	mem[4][255] = size & 0xFF;
	
	memcpy((u8*)&mem[5][0], program, size);
	
	// set initial register values
	reg[REG_P0] = 5;
	reg[REG_P1] = 0;
	
	reg[REG_S] = 5;
}

void tinyVM_run()
{
	while(1)
	{
		// fetch opcode and argument
		u8 opc = mem[reg[REG_P0]][reg[REG_P1 + 0]]; // opcode is at offset 0 of program counter,
		u8 arg = mem[reg[REG_P0]][reg[REG_P1 + 1]]; // argument is at offset 1
		
		printf("opc: %u, arg: %u\n", opc, arg);
		
		// increment program counter
		if (reg[REG_P1] == 254)
			reg[REG_P0]++;
		reg[REG_P1] += 2;
		
		// Execute opcode
		opcode_table[opc](arg);
	}
}

int main(int argc, char *argv[])
{
	/*
	
	jmp $
	
	pseudocode:
start:			; offset 0
	mov T, 1
	je start
	
	code:
	0001
	0F00

	*/
	
	u16 prog[] =
	{
		0x0001,
		0x0F00
	};
	
	tinyVM_setup(prog, sizeof(prog));
	tinyVM_run();
}




