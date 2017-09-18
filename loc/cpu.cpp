#include "cpu.h"
#include "memory.h"

#include <iostream>
#include <unordered_map>
#include <string>

static const byte NO_OPCODE = 0xDD;
static const word INTERRUPT_HANDLER_VBLANK = 0x0040;
static const word INTERRUPT_HANDLER_LCD    = 0x0048;
static const word INTERRUPT_HANDLER_TIMER  = 0x0050;
static const word INTERRUPT_HANDLER_SLINK  = 0x0058;
static const word INTERRUPT_HANDLER_JOYPAD = 0x0060;

static const std::unordered_map<byte, std::string> s_instrDisassembly = 
{
	{ 0x00, "NOP" },

	// 8 bit Loads
	{ 0x06, "LD B, n(8 bits)" },
	{ 0x0E, "LD C, n(8 bits)" },
	{ 0x16, "LD D, n(8 bits)" },
	{ 0x1E, "LD E, n(8 bits)" },
	{ 0x26, "LD H, n(8 bits)" },
	{ 0x2E, "LD L, n(8 bits)" },
	{ 0x7F, "LD A, A" },
	{ 0x78, "LD A, B" },
	{ 0x79, "LD A, C" },
	{ 0x7A, "LD A, D" },
	{ 0x7B, "LD A, E" },
	{ 0x7C, "LD A, H" },
	{ 0x7D, "LD A, L" },
	{ 0xF2, "LD A, (C)" },
	{ 0x7E, "LD A, (HL) "},
	{ 0x0A, "LD A, (BC)" },
	{ 0x1A, "LD A, (DE)" },
	{ 0xFA, "LD A, (nn)" },
	{ 0x3E, "LD A, #" },
	{ 0x7F, "LD A, A" },
	{ 0x47, "LD B, A" },
	{ 0x40, "LD B, B" },
	{ 0x41, "LD B, C" },
	{ 0x42, "LD B, D" },
	{ 0x43, "LD B, E" },
	{ 0x44, "LD B, H" },
	{ 0x45, "LD B, L" },
	{ 0x46, "LD B, (HL)" },
	{ 0x4F, "LD C, A" },
	{ 0x48, "LD C, B" },
	{ 0x49, "LD C, C" },
	{ 0x4A, "LD C, D" },
	{ 0x4B, "LD C, E" },
	{ 0x4C, "LD C, H" },
	{ 0x4D, "LD C, L" },
	{ 0x4E, "LD C, (HL)" },
	{ 0x57, "LD D, A" },
	{ 0x50, "LD D, B" },
	{ 0x51, "LD D, C" },
	{ 0x52, "LD D, D" },
	{ 0x53, "LD D, E" },
	{ 0x54, "LD D, H" },
	{ 0x55, "LD D, L" },
	{ 0x56, "LD D, (HL)" },
	{ 0x5F, "LD E, A" },
	{ 0x58, "LD E, B" },
	{ 0x59, "LD E, C" },
	{ 0x5A, "LD E, D" },
	{ 0x5B, "LD E, E" },
	{ 0x5C, "LD E, H" },
	{ 0x5D, "LD E, L" },
	{ 0x5E, "LD E, (HL)" },
	{ 0x66, "LD H, (HL)" },
	{ 0x67, "LD H, A" },
	{ 0x60, "LD H, B" },
	{ 0x61, "LD H, C" },
	{ 0x62, "LD H, D" },
	{ 0x63, "LD H, E" },
	{ 0x64, "LD H, H" },
	{ 0x65, "LD H, L" },
	{ 0x6F, "LD L, A" },
	{ 0x68, "LD L, B" },
	{ 0x69, "LD L, C" },
	{ 0x6A, "LD L, D" },
	{ 0x6B, "LD L, E" },
	{ 0x6C, "LD L, H" },
	{ 0x6D, "LD L, L" },
	{ 0x6E, "LD L, (HL)" },
	{ 0x77, "LD (HL), A" },
	{ 0x70, "LD (HL), B" },
	{ 0x71, "LD (HL), C" },
	{ 0x72, "LD (HL), D" },
	{ 0x73, "LD (HL), E" },
	{ 0x74, "LD (HL), H" },
	{ 0x75, "LD (HL), L" },
	{ 0xE0, "LDH (n), A" },
	{ 0xE2, "LD (C), A" },
	{ 0xF0, "LDH A, (n)"},
	{ 0x22, "LDI (HL), A" },
	{ 0x2A, "LDI A, (HL)" },
	{ 0x3A, "LDD A, (HL)" },
	{ 0x02, "LD (BC), A" },
	{ 0x12, "LD (DE), A" },
	{ 0xEA, "LD (nn), A" },
	{ 0x36, "LD (HL), n" },

	// 16 bit Loads
	{ 0x08, "LD nn, sp" },
	{ 0xF9, "LD sp, HL" },
	{ 0x01, "LD BC, nn(16 bits)" },
	{ 0x11, "LD DE, nn(16 bits)" },
	{ 0x21, "LD HL, nn(16 bits)" },
	{ 0x31, "LD SP, nn(16 bits)" },
	{ 0x73, "LD (HL), E" },
	{ 0x32, "LDD (HL), A"},
	{ 0xF8, "LDHL SP, n" },
	{ 0xF5, "PUSH AF" },
	{ 0xC5, "PUSH BC" },
	{ 0xD5, "PUSH DE" },
	{ 0xE5, "PUSH HL" },
	{ 0xF1, "POP AF" },
	{ 0xC1, "POP BC" },
	{ 0xD1, "POP DE" },
	{ 0xE1, "POP HL" },

	// 8 bit ALU
	{ 0x3C, "INC A" },
	{ 0x04, "INC B" },
	{ 0x0C, "INC C" },
	{ 0x14, "INC D" },
	{ 0x1C, "INC E" },
	{ 0x24, "INC H" },
	{ 0x2C, "INC L" },
	{ 0x34, "INC (HL)" },
	{ 0x3D, "DEC A" },
	{ 0x05, "DEC B" },
	{ 0x0D, "DEC C" },
	{ 0x15, "DEC D" },
	{ 0x1D, "DEC E" },
	{ 0x25, "DEC H" },
	{ 0x2D, "DEC L" },
	{ 0x35, "DEC HL" },
	{ 0x87, "ADD A, A" },
	{ 0x80, "ADD A, B" },
	{ 0x81, "ADD A, C" },
	{ 0x82, "ADD A, D" },
	{ 0x83, "ADD A, E" },
	{ 0x84, "ADD A, H" },
	{ 0x85, "ADD A, L" },
	{ 0x86, "ADD A, HL" },
	{ 0xC6, "ADD A, #" },
	{ 0xA7, "AND A, A" },
	{ 0xA0, "AND A, B" },
	{ 0xA1, "AND A, C" },
	{ 0xA2, "AND A, D" },
	{ 0xA3, "AND A, E" },
	{ 0xA4, "AND A, H" },
	{ 0xA5, "AND A, L" },
	{ 0xA6, "AND A, (HL)" },
	{ 0xE6, "AND A, #" },
	{ 0x97, "SUB A" },
	{ 0x90, "SUB B" },
	{ 0x91, "SUB C" },
	{ 0x92, "SUB D" },
	{ 0x93, "SUB E" },
	{ 0x94, "SUB H" },
	{ 0x95, "SUB L" },
	{ 0x96, "SUB (HL) "},
	{ 0xD6, "SUB #" },
	{ 0xBF, "CP A, A"},
	{ 0xB8, "CP A, B" },
	{ 0xB9, "CP A, C" },
	{ 0xBA, "CP A, D" },
	{ 0xBB, "CP A, E" },
	{ 0xBC, "CP A, H" },
	{ 0xBD, "CP A, L" },
	{ 0xFE, "CP A, #" },
	{ 0xBE, "CP A, HL" },
	{ 0xB7, "OR A, A" },
	{ 0xB0, "OR A, B" },
	{ 0xB1, "OR A, C" },
	{ 0xB2, "OR A, D" },
	{ 0xB3, "OR A, E" },
	{ 0xB4, "OR A, H" },
	{ 0xB5, "OR A, L" },
	{ 0xB6, "OR A, (HL) "},
	{ 0xF6, "OR A, #" },
	{ 0xAF, "XOR A, A" },
	{ 0xA8, "XOR A, B" },
	{ 0xA9, "XOR A, C" },
	{ 0xAA, "XOR A, D" },
	{ 0xAB, "XOR A, E" },
	{ 0xAC, "XOR A, H" },
	{ 0xAD, "XOR A, L" },
	{ 0xAE, "XOR A, (HL)" },
	{ 0xEE, "XOR A, #" },
	{ 0x8F, "ADC A, A" },
	{ 0x88, "ADC A, B" },
	{ 0x89, "ADC A, C" },
	{ 0x8A, "ADC A, D" },
	{ 0x8B, "ADC A, E" },
	{ 0x8C, "ADC A, H" },
	{ 0x8D, "ADC A, L" },
	{ 0x8E, "ADC A, (HL)" },
	{ 0xCE, "ADC A, #" },
	{ 0x9F, "SBC A, A" },
	{ 0x98, "SBC A, B" },
	{ 0x99, "SBC A, C" },
	{ 0x9A, "SBC A, D" },
	{ 0x9B, "SBC A, E" },
	{ 0x9C, "SBC A, H" },
	{ 0x9D, "SBC A, L" },
	{ 0x9E, "SBC A, (HL)" },
	{ 0xDE, "SBC A, n" },

	// 16-bit ALU
	{ 0x03, "INC BC" },
	{ 0x13, "INC DE" },
	{ 0x23, "INC HL" },
	{ 0x33, "INC SP" },
	{ 0x0B, "DEC BC" },
	{ 0x1B, "DEC DE" },
	{ 0x2B, "DEC HL" },
	{ 0x3B, "DEC SP" },
	{ 0x09, "ADD HL, BC"},
	{ 0x19, "ADD HL, DE" },
	{ 0x29, "ADD HL, HL" },
	{ 0x39, "ADD HL, SP" },
	{ 0xE8, "ADD SP, n" },

	// Rotates & Shifts
	{ 0x0F, "RRCA" },
	{ 0x17, "RLA" },
	{ 0x1F, "RRA" },
	{ 0x07, "RLCA" },

	// Calls
	{ 0xCD, "CALL nn" },
	{ 0xC4, "CALL NZ, nn" },
	{ 0xCC, "CALL Z, nn" },
	{ 0xD4, "CALL NC, nn" },
	{ 0xDC, "CALL C, nn" },

	// Jumps
	{ 0x10, "DJNZ" },
	{ 0xC3, "JP nn(16 bits)" },
	{ 0xE9, "JP (HL)" },
	{ 0xC2, "JP NZ, nn" },
	{ 0xCA, "JP Z, nn" },
	{ 0xD2, "JP NC, nn" },
	{ 0xDA, "JP C, nn" },
	{ 0x18, "JR n" },
	{ 0x20, "JR NZ,n" },
	{ 0x28, "JR Z,n" },
	{ 0x30, "JR NC,n" },
	{ 0x38, "JR C,n" },

	// Returns
	{ 0xC9, "RET" },
	{ 0xC0, "RET NZ" },
	{ 0xC8, "RET Z" },
	{ 0xD0, "RET NC" },
	{ 0xD8, "RET C" },
	{ 0xD9, "RETI" },

	// Misc
	{ 0x76, "HALT" },
	{ 0x27, "DAA" },
	{ 0xF3, "DI" },
	{ 0xFB, "EI" },
	{ 0xC7, "RST 0x00" },
	{ 0xCF, "RST 0x08" },
	{ 0xD7, "RST 0x10" },
	{ 0xDF, "RST 0x18" },
	{ 0xE7, "RST 0x20" },
	{ 0xEF, "RST 0x28" },
	{ 0xF7, "RST 0x30" },
	{ 0xFF, "RST 0x38" },

	{ 0x2F, "CPL" },
	{ 0x37, "SCF" },
	{ 0x3F, "CCF" }
};

static const byte coreInstructionTicks[256] = {
	2, 6, 4, 4, 2, 2, 4, 4, 10, 4, 4, 4, 2, 2, 4, 4, // 0x0
	2, 6, 4, 4, 2, 2, 4, 4,  4, 4, 4, 4, 2, 2, 4, 4, // 0x1
	0, 6, 4, 4, 2, 2, 4, 2,  0, 4, 4, 4, 2, 2, 4, 2, // 0x2
	4, 6, 4, 4, 6, 6, 6, 2,  0, 4, 4, 4, 2, 2, 4, 2, // 0x3
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x4
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x5
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x6
	4, 4, 4, 4, 4, 4, 2, 4,  2, 2, 2, 2, 2, 2, 4, 2, // 0x7
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x8
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0x9
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0xa
	2, 2, 2, 2, 2, 2, 4, 2,  2, 2, 2, 2, 2, 2, 4, 2, // 0xb
	0, 6, 0, 6, 0, 8, 4, 8,  0, 2, 0, 0, 0, 6, 4, 8, // 0xc
	0, 6, 0, 0, 0, 8, 4, 8,  0, 8, 0, 0, 0, 0, 4, 8, // 0xd
	6, 6, 4, 0, 0, 8, 4, 8,  8, 2, 8, 0, 0, 0, 4, 8, // 0xe
	6, 6, 4, 2, 0, 8, 4, 8,  6, 4, 8, 2, 0, 0, 4, 8  // 0xf
};

static const std::unordered_map<byte, std::string> s_bitOpcodeDisassembly =
{
	// Bits
	{ 0x47, "BIT 0A" },
	{ 0x4F, "BIT 1A" },
	{ 0x57, "BIT 2A" },
	{ 0x5F, "BIT 3A" },
	{ 0x67, "BIT 4A" },
	{ 0x6F, "BIT 5A" },
	{ 0x77, "BIT 6A" },
	{ 0x7F, "BIT 7A" },
	{ 0x40, "BIT 0B" },
	{ 0x48, "BIT 1B" },
	{ 0x50, "BIT 2B" },
	{ 0x58, "BIT 3B" },
	{ 0x60, "BIT 4B" },
	{ 0x68, "BIT 5B" },
	{ 0x70, "BIT 6B" },
	{ 0x78, "BIT 7B" },
	{ 0x41, "BIT 0C" },
	{ 0x49, "BIT 1C" },
	{ 0x51, "BIT 2C" },
	{ 0x59, "BIT 3C" },
	{ 0x61, "BIT 4C" },
	{ 0x69, "BIT 5C" },
	{ 0x71, "BIT 6C" },
	{ 0x79, "BIT 7C" },
	{ 0x42, "BIT 0D" },
	{ 0x4A, "BIT 1D" },
	{ 0x52, "BIT 2D" },
	{ 0x5A, "BIT 3D" },
	{ 0x62, "BIT 4D" },
	{ 0x6A, "BIT 5D" },
	{ 0x72, "BIT 6D" },
	{ 0x7A, "BIT 7D" },
	{ 0x43, "BIT 0E" },
	{ 0x4B, "BIT 1E" },
	{ 0x53, "BIT 2E" },
	{ 0x5B, "BIT 3E" },
	{ 0x63, "BIT 4E" },
	{ 0x6B, "BIT 5E" },
	{ 0x73, "BIT 6E" },
	{ 0x7B, "BIT 7E" },
	{ 0x64, "BIT 0H" },
	{ 0x6C, "BIT 1H" },
	{ 0x54, "BIT 2H" },
	{ 0x5C, "BIT 3H" },
	{ 0x64, "BIT 4H" },
	{ 0x6C, "BIT 5H" },
	{ 0x74, "BIT 6H" },
	{ 0x7C, "BIT 7H" },
	{ 0x45, "BIT 0L" },
	{ 0x4D, "BIT 1L" },
	{ 0x55, "BIT 2L" },
	{ 0x5D, "BIT 3L" },
	{ 0x65, "BIT 4L" },
	{ 0x6D, "BIT 5L" },
	{ 0x75, "BIT 6L" },
	{ 0x7D, "BIT 7L" },
	{ 0x46, "BIT 0HL" },
	{ 0x4E, "BIT 1HL" },
	{ 0x56, "BIT 2HL" },
	{ 0x5E, "BIT 3HL" },
	{ 0x66, "BIT 4HL" },
	{ 0x6E, "BIT 5HL" },
	{ 0x76, "BIT 6HL" },
	{ 0x7E, "BIT 7HL" },
	
	{ 0x87, "RES 0, A" },
	{ 0x8F, "RES 1, A" },
	{ 0x97, "RES 2, A" },
	{ 0x9F, "RES 3, A" },
	{ 0xA7, "RES 4, A" },
	{ 0xAF, "RES 5, A" },
	{ 0xB7, "RES 6, A" },
	{ 0xBF, "RES 7, A" },
	{ 0x80, "RES 0, B" },
	{ 0x88, "RES 1, B" },
	{ 0x90, "RES 2, B" },
	{ 0x98, "RES 3, B" },
	{ 0xA0, "RES 4, B" },
	{ 0xA8, "RES 5, B" },
	{ 0xB0, "RES 6, B" },
	{ 0xB8, "RES 7, B" },
	{ 0x81, "RES 0, C" },
	{ 0x89, "RES 1, C" },
	{ 0x91, "RES 2, C" },
	{ 0x99, "RES 3, C" },
	{ 0xA1, "RES 4, C" },
	{ 0xA9, "RES 5, C" },
	{ 0xB1, "RES 6, C" },
	{ 0xB9, "RES 7, C" },
	{ 0x82, "RES 0, D" },
	{ 0x8A, "RES 1, D" },
	{ 0x92, "RES 2, D" },
	{ 0x9A, "RES 3, D" },
	{ 0xA2, "RES 4, D" },
	{ 0xAA, "RES 5, D" },
	{ 0xB2, "RES 6, D" },
	{ 0xBA, "RES 7, D" },
	{ 0x83, "RES 0, E" },
	{ 0x8B, "RES 1, E" },	
	{ 0x93, "RES 2, E" },
	{ 0x9B, "RES 3, E" },
	{ 0xA3, "RES 4, E" },
	{ 0xAB, "RES 5, E" },
	{ 0xB3, "RES 6, E" },
	{ 0xBB, "RES 7, E" },
	{ 0x84, "RES 0, H" },
	{ 0x8C, "RES 1, H" },
	{ 0x94, "RES 2, H" },
	{ 0x9C, "RES 3, H" },
	{ 0xA4, "RES 4, H" },
	{ 0xAC, "RES 5, H" },
	{ 0xB4, "RES 6, H" },
	{ 0xBC, "RES 7, H" },
	{ 0x85, "RES 0, L" },
	{ 0x8D, "RES 1, L" },
	{ 0x95, "RES 2, L" },
	{ 0x9D, "RES 3, L" },
	{ 0xA5, "RES 4, L" },
	{ 0xAD, "RES 5, L" },
	{ 0xB5, "RES 6, L" },
	{ 0xBD, "RES 7, L" },
	{ 0x86, "RES 0, (HL)" },
	{ 0x8E, "RES 1, (HL)" },
	{ 0x96, "RES 2, (HL)" },
	{ 0x9E, "RES 3, (HL)" },
	{ 0xA6, "RES 4, (HL)" },
	{ 0xAE, "RES 5, (HL)" },
	{ 0xB6, "RES 6, (HL)" },
	{ 0xBE, "RES 7, (HL)" },
	{ 0x17, "RL A" },
	{ 0x10, "RL B" },
	{ 0x11, "RL C" },
	{ 0x12, "RL D" },
	{ 0x13, "RL E" },
	{ 0x14, "RL H" },
	{ 0x15, "RL L" },
	{ 0x16, "RL (HL)" },
	{ 0x1F, "RR A" },
	{ 0x18, "RR B" },
	{ 0x19, "RR C" },
	{ 0x1A, "RR D" },
	{ 0x1B, "RR E" },
	{ 0x1C, "RR H" },
	{ 0x1D, "RR L" },
	{ 0x1E, "RR (HL)" },
	{ 0x0F, "RRC A" },
	{ 0x08, "RRC B" },
	{ 0x09, "RRC C" },
	{ 0x0A, "RRC D" },
	{ 0x0B, "RRC E" },
	{ 0x0C, "RRC H" },
	{ 0x0D, "RRC L" },
	{ 0x0E, "RRC (HL) "},
	{ 0x07, "RLC A" },
	{ 0x00, "RLC B" },
	{ 0x01, "RLC C" },
	{ 0x02, "RLC D" },
	{ 0x03, "RLC E" },
	{ 0x04, "RLC H" },
	{ 0x05, "RLC L" },
	{ 0x06, "RLC (HL)" },

	// Sets
	{ 0xC7, "SET 0, A" },
	{ 0xCF, "SET 1, A" },
	{ 0xD7, "SET 2, A" },
	{ 0xDF, "SET 3, A" },
	{ 0xE7, "SET 4, A" },
	{ 0xEF, "SET 5, A" },
	{ 0xF7, "SET 6, A" },
	{ 0xFF, "SET 7, A" },
	{ 0xC0, "SET 0, B" },
	{ 0xC8, "SET 1, B" },
	{ 0xD0, "SET 2, B" },
	{ 0xD8, "SET 3, B" },
	{ 0xE0, "SET 4, B" },
	{ 0xE8, "SET 5, B" },
	{ 0xF0, "SET 6, B" },
	{ 0xF8, "SET 7, B" },
	{ 0xC1, "SET 0, C" },
	{ 0xC9, "SET 1, C" },
	{ 0xD1, "SET 2, C" },
	{ 0xD9, "SET 3, C" },
	{ 0xE1, "SET 4, C" },
	{ 0xE9, "SET 5, C" },
	{ 0xF1, "SET 6, C" },
	{ 0xF9, "SET 7, C" },
	{ 0xC2, "SET 0, D" },
	{ 0xCA, "SET 1, D" },
	{ 0xD2, "SET 2, D" },
	{ 0xDA, "SET 3, D" },
	{ 0xE2, "SET 4, D" },
	{ 0xEA, "SET 5, D" },
	{ 0xF2, "SET 6, D" },
	{ 0xFA, "SET 7, D" },
	{ 0xC3, "SET 0, E" },
	{ 0xCB, "SET 1, E" },
	{ 0xD3, "SET 2, E" },
	{ 0xDB, "SET 3, E" },
	{ 0xE3, "SET 4, E" },
	{ 0xEB, "SET 5, E" },
	{ 0xF3, "SET 6, E" },
	{ 0xFB, "SET 7, E" },
	{ 0xC4, "SET 0, H" },
	{ 0xCC, "SET 1, H" },
	{ 0xD4, "SET 2, H" },
	{ 0xDC, "SET 3, H" },
	{ 0xE4, "SET 4, H" },
	{ 0xEC, "SET 5, H" },
	{ 0xF4, "SET 6, H" },
	{ 0xFC, "SET 7, H" },
	{ 0xC5, "SET 0, L" },
	{ 0xCD, "SET 1, L" },
	{ 0xD5, "SET 2, L" },
	{ 0xDD, "SET 3, L" },
	{ 0xE5, "SET 4, L" },
	{ 0xED, "SET 5, L" },
	{ 0xF5, "SET 6, L" },
	{ 0xFD, "SET 7, L" },
	{ 0xC6, "SET 0, (HL)" },
	{ 0xCE, "SET 1, (HL)" },
	{ 0xD6, "SET 2, (HL)" },
	{ 0xDE, "SET 3, (HL)" },
	{ 0xE6, "SET 4, (HL)" },
	{ 0xEE, "SET 5, (HL)" },
	{ 0xF6, "SET 6, (HL)" },
	{ 0xFE, "SET 7, (HL)" },	
	

	// Shifts
	{ 0x27, "SLA A" },
	{ 0x20, "SLA B" },
	{ 0x21, "SLA C" },
	{ 0x22, "SLA D" },
	{ 0x23, "SLA E" },
	{ 0x24, "SLA H" },
	{ 0x25, "SLA L" },
	{ 0x26, "SLA (HL)" },
	{ 0x2F, "SRA A" },
	{ 0x28, "SRA B" },
	{ 0x29, "SRA C" },
	{ 0x2A, "SRA D" },
	{ 0x2B, "SRA E" },
	{ 0x2C, "SRA H" },
	{ 0x2D, "SRA L" },
	{ 0x2E, "SRA (HL)" },
	{ 0x3F, "SRL A" },
	{ 0x38, "SRL B" },
	{ 0x39, "SRL C" },
	{ 0x3A, "SRL D" },
	{ 0x3B, "SRL E" },
	{ 0x3C, "SRL H" },
	{ 0x3D, "SRL L" },
	{ 0x3E, "SRL (HL" },

	// Swap
	{ 0x37, "SWAP A" },
	{ 0x30, "SWAP B" },
	{ 0x31, "SWAP C" },
	{ 0x32, "SWAP D" },
	{ 0x33, "SWAP E" },
	{ 0x34, "SWAP H" },
	{ 0x35, "SWAP L" },
	{ 0x36, "SWAP (HL) "},
};

static const byte cbInstructionTicks[256] = {
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x0
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x1
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x2
	8, 8, 8, 8, 8,  8, 16, 8,  8, 8, 8, 8, 8, 8, 16, 8, // 0x3
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x4
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x5
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x6
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x7
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x8
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0x9
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xa
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xb
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xc
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xd
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8, // 0xe
	8, 8, 8, 8, 8,  8, 12, 8,  8, 8, 8, 8, 8, 8, 12, 8  // 0xf
};

Cpu::Cpu(Memory& memory)
	: _memory(memory)
	, _opcode(NO_OPCODE)
	, _isBitOpcode(false)
{
	resetCpu();
}

void Cpu::emulateCycle()
{
	if (_halted)
		return;
	
	_opcode      = _memory.readByte(_registers.pc++);
	_isBitOpcode = false;

	switch (_opcode)
	{
		
		case 0x00: // NOP
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// 8 Bit Loads
		case 0x06: // LD B,n
		{
			_registers.B = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x0E: // LD C,n
		{
			_registers.C = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x16: // LD D,n
		{
			_registers.D = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1E: // LD E,n
		{
			_registers.E = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x26: // LD H,n
		{
			_registers.H = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x2E: // LD L,n
		{
			_registers.L = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7F: // LD A, A
		{
			_registers.A = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x78: // LD A, B
		{
			_registers.A = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x79: // LD A, C
		{
			_registers.A = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7A: // LD A, D
		{
			_registers.A = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7B: // LD A, E
		{
			_registers.A = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7C: //LD A, H
		{
			_registers.A = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7D: // LD A, L
		{
			_registers.A = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF2: // LD A, (C)
		{
			_registers.A = _memory.readByte(0xFF00 + _registers.C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x7E: // LD A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x0A: // LD A, (BC)
		{
			_registers.A = _memory.readByte((_registers.B << 8) + _registers.C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1A: // LD A, (DE)
		{
			_registers.A = _memory.readByte((_registers.D << 8) + _registers.E);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xFA: // LD A, (nn)
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_registers.A = _memory.readByte(address);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x47: // LD B, A
		{
			_registers.B = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x40: // LD B, B
		{
			_registers.B = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x41: // LD B, C
		{
			_registers.B = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x42: // LD B, D
		{
			_registers.B = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x43: // LD B, E
		{
			_registers.B = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x44: // LD B, H
		{
			_registers.B = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x45: // LD B, L
		{
			_registers.B = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x46: // LD B, (HL)
		{
			_registers.B = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4F: // LD C, A
		{
			_registers.C = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x48: // LD C, B
		{
			_registers.C = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x49: // LD C, C
		{
			_registers.C = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4A: // LD C, D
		{
			_registers.C = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4B: // LD C, E
		{
			_registers.C = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4C: // LD C, H
		{
			_registers.C = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4D: // LD C, L
		{
			_registers.C = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x4E: // LD C, (HL)
		{
			_registers.C = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x57: // LD D, A
		{
			_registers.D = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x50: // LD D, B
		{
			_registers.D = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x51: // LD D, C
		{
			_registers.D = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x52: // LD D, D
		{
			_registers.D = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x53: // LD D, E
		{
			_registers.D = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x54: // LD D, H
		{
			_registers.D = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x55: // LD D, L
		{
			_registers.D = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x56: // LD D, (HL)
		{
			_registers.D = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5F: // LD E, A
		{
			_registers.E = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x58: // LD E, B
		{
			_registers.E = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x59: // LD E, C
		{
			_registers.E = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5A: // LD E, D
		{
			_registers.E = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5B: // LD E, E
		{
			_registers.E = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5C: // LD E, H
		{
			_registers.E = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5D: // LD E, L
		{
			_registers.E = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x5E: // LD E, (HL)
		{
			_registers.E = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x66: // LD H, (HL)
		{
			_registers.H = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x67: // LD H, A
		{
			_registers.H = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x60: // LD H, B
		{
			_registers.H = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x61: // LD H, C
		{
			_registers.H = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x62: // LD H, D
		{
			_registers.H = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x63: // LD H, E
		{
			_registers.H = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x64: // LD H, H
		{
			_registers.H = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x65: // LD H, L
		{
			_registers.H = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6F: // LD L, A
		{
			_registers.L = _registers.A;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x68: // LD L, B
		{
			_registers.L = _registers.B;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x69: // LD L, C
		{
			_registers.L = _registers.C;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6A: // LD L, D
		{
			_registers.L = _registers.D;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6B: // LD L, E
		{
			_registers.L = _registers.E;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6C: // LD L, H
		{
			_registers.L = _registers.H;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6D: // LD L, L
		{
			_registers.L = _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x6E: // LD L, (HL)
		{
			_registers.L = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x77: // LD (HL), A
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.A);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF8: // LDHL SP, n
		{
			signed char val = _memory.readByte(_registers.pc++);
			int result = _registers.sp + val;
			
			// Gearboy saves the day again
			if (((_registers.sp ^ val ^ result) & 0x100) == 0x100)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (((_registers.sp ^ val ^ result) & 0x10) == 0x10)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			
			_registers.H = ((result & 0xFF00) >> 8);
			_registers.L = result & 0x00FF;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x70: // LD (HL), B
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.B);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x71: // LD (HL), C
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x72: // LD (HL), D
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.D);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x73: // LD (HL), E
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.E);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x74: // LD (HL), H
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.H);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x75: // LD (HL), L
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x3E: // LD A, #
		{
			_registers.A = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE0: // LDH (n), A
		{
			byte address = _memory.readByte(_registers.pc++);
			_memory.writeByte(0xFF00 + address, _registers.A);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE2: // LD (C), A
		{			
			_memory.writeByte(0xFF00 + _registers.C, _registers.A);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF0: // LDH A,(n)
		{
			word address = _memory.readByte(_registers.pc++) + 0xFF00;
			_registers.A = _memory.readByte(address);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x22: // LDI (HL), A
		{
			_memory.writeByte((_registers.H << 8) + _registers.L, _registers.A);
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;
			
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x2A: // LDI A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x3A: // LDD A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.L--;
			if (_registers.L == 0xFF)
				_registers.H--;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x02: // LD (BC) A
		{
			_memory.writeByte((_registers.B << 8) + _registers.C, _registers.A);			
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x12: // LD (DE) A
		{
			_memory.writeByte((_registers.D << 8) + _registers.E, _registers.A);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xEA: // LD (nn), A
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_memory.writeByte(address, _registers.A);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x36: // LD (HL), n
		{
			byte nextByte = _memory.readByte(_registers.pc++);
			_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// 16 Bit Loads
		case 0x08: // LD nn,sp
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_memory.writeWord(address, _registers.sp);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;			
		} break;
		case 0xF9: // LD sp, HL
		{
			_registers.sp = (_registers.H << 8) + _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x01: // LD BC, nn
		{
			_registers.C = _memory.readByte(_registers.pc++);
			_registers.B = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x11: // LD DE, nn
		{
			_registers.E = _memory.readByte(_registers.pc++);
			_registers.D = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x21: // LD HL,nn
		{
			_registers.L = _memory.readByte(_registers.pc++);
			_registers.H = _memory.readByte(_registers.pc++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x31: // LD SP,nn
		{
			_registers.sp = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x32: // LDD (HL), A 
		{
			_memory.writeByte((_registers.H << 8) + _registers.L, _registers.A);
			_registers.L--;
			if (_registers.L == 0xFF)
				_registers.H--;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF5: // PUSH AF
		{
			_memory.writeByte(--_registers.sp, _registers.A);
			_memory.writeByte(--_registers.sp, _registers.F);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;

		} break;
		case 0xC5: // PUSH BC
		{
			_memory.writeByte(--_registers.sp, _registers.B);
			_memory.writeByte(--_registers.sp, _registers.C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD5: // PUSH DE
		{
			_memory.writeByte(--_registers.sp, _registers.D);
			_memory.writeByte(--_registers.sp, _registers.E);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE5: // PUSH HL
		{
			_memory.writeByte(--_registers.sp, _registers.H);
			_memory.writeByte(--_registers.sp, _registers.L);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF1: // POP AF
		{
			_registers.F = _memory.readByte(_registers.sp++);
			_registers.A = _memory.readByte(_registers.sp++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xC1: // POP BC
		{
			_registers.C = _memory.readByte(_registers.sp++);
			_registers.B = _memory.readByte(_registers.sp++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD1: // POP DE
		{
			_registers.E = _memory.readByte(_registers.sp++);
			_registers.D = _memory.readByte(_registers.sp++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE1: // POP HL
		{
			_registers.L = _memory.readByte(_registers.sp++);
			_registers.H = _memory.readByte(_registers.sp++);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// 8-bit ALU
		case 0x3C: // INC A
		{
			if ((_registers.A & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else 
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.A++;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x04: // INC B
		{
			if ((_registers.B & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.B++;

			if (_registers.B == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x0C: // INC C
		{
			if ((_registers.C & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.C++;

			if (_registers.C == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x14: // INC D
		{
			if ((_registers.D & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.D++;

			if (_registers.D == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1C: // INC E
		{
			if ((_registers.E & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.E++;

			if (_registers.E == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x24: // INC H
		{
			if ((_registers.H & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.H++;

			if (_registers.H == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x2C: // INC L
		{
			if ((_registers.L & 0x0F) == 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.L++;

			if (_registers.L == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x34: // INC (HL)
		{
			byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L) + 1;
			_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);

			if ((nextByte & 0x0F) == 0x00)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);

			if (nextByte == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x3D: // DEC A
		{
			if ((_registers.A & 0x0F) == 0x00)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.A--;
			if (_registers.A == 0) 
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x05: // DEC B
		{
			if ((_registers.B & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.B--;
			if (_registers.B == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x0D: // DEC C
		{
			if ((_registers.C & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.C--;
			if (_registers.C == 0)
				setFlag(FLAG_Z);
			else 
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x15: // DEC D
		{
			if ((_registers.D & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.D--;
			if (_registers.D == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1D: // DEC E
		{
			if ((_registers.E & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.E--;
			if (_registers.E == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		case 0x25: // DEC H
		{
			if ((_registers.H & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.H--;
			if (_registers.H == 0)
				setFlag(FLAG_Z);
			else 
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x2D: // DEC L
		{
			if ((_registers.L & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.L--;
			if (_registers.L == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x35: // DEC HL
		{
			byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
			
			if ((nextByte & 0x0F) == 0x0)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			
			_memory.writeByte((_registers.H << 8) + _registers.L, --nextByte);

			if (nextByte == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x87: // ADD A, A
		{
			if (0xFF - _registers.A < _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.A += _registers.A;

			resetFlag(FLAG_N);
			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x80: // ADD A, B
		{
			if (0xFF - _registers.A < _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.A += _registers.B;
			
			resetFlag(FLAG_N);

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x81: // ADD A, C
		{
			if (0xFF - _registers.A < _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);
			_registers.A += _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x82: // ADD A, D
		{
			if (0xFF - _registers.A < _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);
			_registers.A += _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x83: // ADD A, E
		{
			if (0xFF - _registers.A < _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);

			_registers.A += _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x84: // ADD A, H
		{
			if (0xFF - _registers.A < _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);
			_registers.A += _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x85: // ADD A, L
		{
			if (0xFF - _registers.A < _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);

			_registers.A += _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x86: // ADD A, HL
		{
			byte val = _memory.readByte((_registers.H << 8) + _registers.L);
			if (0xFF - _registers.A < val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);
			
			_registers.A += val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xC6: // ADD A, #
		{
			byte val = _memory.readByte(_registers.pc++);
			if (0xFF - _registers.A < val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			resetFlag(FLAG_N);

			_registers.A += val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA7: // AND A, A
		{
			_registers.A = _registers.A & _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA0: // AND A, B
		{
			_registers.A = _registers.A & _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA1: // AND A, C
		{
			_registers.A = _registers.A & _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA2: // AND A, D
		{
			_registers.A = _registers.A & _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA3: // AND A, E
		{
			_registers.A = _registers.A & _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA4: // AND A, H
		{
			_registers.A = _registers.A & _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA5: // AND A, L
		{
			_registers.A = _registers.A & _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA6: // AND A, (HL)
		{
			_registers.A = _registers.A & _memory.readByte((_registers.H << 8) + _registers.L);

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE6: // AND A, #
		{
			_registers.A = _registers.A & _memory.readByte(_registers.pc++);
		
			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			setFlag(FLAG_H);
			resetFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x97: // SUB A
		{
			if ((_registers.A & 0x0F) < (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			
			if (_registers.A < _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x90: // SUB B
		{
			if ((_registers.A & 0x0F) < (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x91: // SUB C
		{
			if ((_registers.A & 0x0F) < (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x92: // SUB D
		{
			if ((_registers.A & 0x0F) < (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x93: // SUB E
		{
			if ((_registers.A & 0x0F) < (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x94: // SUB H
		{
			if ((_registers.A & 0x0F) < (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x95: // SUB L
		{
			if ((_registers.A & 0x0F) < (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x96: // SUB (HL)
		{
			byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
			if ((_registers.A & 0x0F) < (nextByte & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < nextByte)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= nextByte;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD6: // SUB #
		{
			byte nextByte = _memory.readByte(_registers.pc++);
			if ((_registers.A & 0x0F) < (nextByte & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A < nextByte)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.A -= nextByte;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			setFlag(FLAG_N);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBF: // CP A, A
		{
			if (_registers.A - _registers.A == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB8: // CP A, B
		{
			if (_registers.A - _registers.B == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB9: //CP A, C
		{
			if (_registers.A - _registers.C == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBA: // CP A, D
		{
			if (_registers.A - _registers.D == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBB: //CP A, E
		{
			if (_registers.A - _registers.E == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBC: //CP A, H
		{
			if (_registers.A - _registers.H == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBD: // CP A, L
		{
			if (_registers.A - _registers.L == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xFE: // CP A, #
		{
			byte val = _memory.readByte(_registers.pc++);

			if (_registers.A - val == 0)
				setFlag(FLAG_Z);
			else 
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xBE: // CP A, HL
		{
			byte val = _memory.readByte((_registers.H << 8) + _registers.L);
			if (_registers.A - val == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			if (_registers.A < val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((_registers.A & 0x0F) < (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB7: // OR A, A
		{
			_registers.A = _registers.A | _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB0: // OR A, B
		{
			_registers.A = _registers.A | _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB1: // OR A, C
		{
			_registers.A = _registers.A | _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB2: // OR A, D
		{
			_registers.A = _registers.A | _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB3: // OR A, E
		{
			_registers.A = _registers.A | _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB4: // OR A, H
		{
			_registers.A = _registers.A | _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB5: // OR A, L
		{
			_registers.A = _registers.A | _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xB6: // OR A, (HL)
		{
			_registers.A = _registers.A | _memory.readByte((_registers.H << 8) + _registers.L);

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF6: // OR A, #
		{
			_registers.A = _registers.A | _memory.readByte(_registers.pc++);

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAF: // XOR A, A
		{
			_registers.A ^= _registers.A; 
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA8: // XOR A, B
		{
			_registers.A ^= _registers.B;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xA9: // XOR A, C
		{
			_registers.A ^= _registers.C;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAA: // XOR A, D
		{
			_registers.A ^= _registers.D;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAB: // XOR A, E
		{
			_registers.A ^= _registers.E;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAC: // XOR A, H
		{
			_registers.A ^= _registers.H; 
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAD: // XOR A, L
		{
			_registers.A ^= _registers.L;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xAE: // XOR A, (HL)
		{
			_registers.A ^= _memory.readByte((_registers.H << 8) + _registers.L);
			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xEE: // XOR A, #
		{
			_registers.A ^= _memory.readByte(_registers.pc++);
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); 
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8F: // ADC A, A
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x88: // ADC A, B
		{			
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x89: // ADC A, C
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8A: // ADC A, D
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8B: // ADC A, E
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8C: // ADC A, H
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8D: // ADC A, L
		{
			byte flagC = getFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) < flagC + (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x8E: // ADC A, (HL)
		{
			byte flagC = getFlag(FLAG_C);
			byte val = _memory.readByte((_registers.H << 8) + _registers.L);

			if (0x0F - (_registers.A & 0x0F) < flagC + (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xCE: // ADC A, #
		{
			byte flagC = getFlag(FLAG_C);
			byte val = _memory.readByte(_registers.pc++);

			if (0x0F - (_registers.A & 0x0F) < flagC + (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A < flagC + val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += flagC + val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9F: // SBC A, A
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.A & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.A;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x98: // SBC A, B
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.B & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.B;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x99: // SBC A, C
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.C & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.C;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9A: // SBC A, D
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.D & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.D;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9B: // SBC A, E
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.E & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.E;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9C: // SBC A, H
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.H & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.H;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9D: // SBC A, L
		{
			byte flagC = getFlag(FLAG_C);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (_registers.L & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + _registers.L;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x9E: // SBC A, (HL)
		{
			byte flagC = getFlag(FLAG_C);
			byte val = _memory.readByte((_registers.H << 8) + _registers.L);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (val & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + val;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xDE: // SBC A, n
		{
			byte flagC = getFlag(FLAG_C);
			byte val = _memory.readByte(_registers.pc++);

			int regALoNibble = _registers.A & 0x0F;
			int otherLoNibble = flagC + (val & 0x0F);
			if (regALoNibble < otherLoNibble)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			int regA = _registers.A;
			int other = flagC + val;

			if (regA < other)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			setFlag(FLAG_N);

			_registers.A -= flagC + val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// 16-bit ALU
		case 0x03: // INC BC
		{
			if (_registers.C == 0xFF)
				_registers.B++;
			_registers.C++;
			
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x13: // INC DE
		{
			if (_registers.E == 0xFF)
				_registers.D++;
			_registers.E++;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x23: // INC HL
		{
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x33: // INC SP
		{
			_registers.sp++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x0B: // DEC BC
		{
			if (_registers.C == 0x00)
				_registers.B--;
			_registers.C--;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1B: // DEC DE
		{
			if (_registers.E == 0x00)
				_registers.D--;
			_registers.E--;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x2B: // DEC HL
		{
			if (_registers.L == 0x00)
				_registers.H--;
			_registers.L--;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x3B: // DEC SP 
		{
			_registers.sp--;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x09: // ADD HL, BC
		{
			int BC = (_registers.B << 8) + _registers.C;
			int HL = (_registers.H << 8) + _registers.L;
			int res = HL + BC;

			if (res & 0xFFFF0000)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0xFFF - (HL & 0xFFF) < (BC & 0xFFF))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			HL += BC;

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;

		} break;
		case 0x19: // ADD HL, DE
		{
			int DE = (_registers.D << 8) + _registers.E;
			int HL = (_registers.H << 8) + _registers.L;
			int res = HL + DE;

			if (res & 0xFFFF0000)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0xFFF - (HL & 0xFFF) < (DE & 0xFFF))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			HL += DE;

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x29: // ADD HL, HL
		{
			int HL2 = (_registers.H << 8) + _registers.L;
			int HL = (_registers.H << 8) + _registers.L;
			int res = HL + HL2;

			if (res & 0xFFFF0000)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0xFFF - (HL & 0xFFF) < (HL2 & 0xFFF))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			HL += HL2;

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x39: // ADD HL, SP
		{
			int SP = _registers.sp;
			int HL = (_registers.H << 8) + _registers.L;
			int res = HL + SP;

			if (res & 0xFFFF0000)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0xFFF - (HL & 0xFFF) < (SP & 0xFFF))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			HL += SP;

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE8: // ADD SP, n
		{
			signed char val = _memory.readByte(_registers.pc++);
			int result = _registers.sp + val;

			// Gearboy saves the day again
			if (((_registers.sp ^ val ^ result) & 0x100) == 0x100)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (((_registers.sp ^ val ^ result) & 0x10) == 0x10)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.sp += val;

			resetFlag(FLAG_Z);
			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// Rotates & Shifts
		case 0x0F: // RRCA
		{
			if ((_registers.A & 0x01) != 0)
			{
				_registers.A >>= 1;
				_registers.A |= 0x80;
				setFlag(FLAG_C);
			}
			else
			{
				_registers.A >>= 1;
				resetFlag(FLAG_C);
			}
			
			resetFlag(FLAG_Z);
			resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x17: // RLA
		{
			byte prevCarry = getFlag(FLAG_C);
			byte prevSeventhBit = _registers.A & 0x80;

			_registers.A <<= 1;

			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			if (prevSeventhBit)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (prevCarry)
				_registers.A |= 0x01;
			else
				_registers.A &= ~0x01;

			if (_registers.A == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x1F: // RRA
		{
			byte prevCarry = isFlagSet(FLAG_C) ? 0x80 : 0x00;
			
			if ((_registers.A & 0x01) != 0)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);				

			_registers.A >>= 1;
			_registers.A |= prevCarry;

			resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x07: // RLCA
		{			
			if ((_registers.A & 0x80) != 0)
			{
				setFlag(FLAG_C);
				_registers.A <<= 1;
				_registers.A |= 0x1;
			}
			else
			{
				resetFlag(FLAG_C);
				resetFlag(FLAG_H);
				resetFlag(FLAG_N);
				resetFlag(FLAG_Z);
				_registers.A <<= 1;
			}
			
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// Calls
		case 0xCD: // Call nn
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc + 2);
			_registers.pc = _memory.readWord(_registers.pc);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2; //TODO: check correct timing manual says 12Cycles
		} break;
		case 0xC4: // CALL NZ, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_Z)) 
			{
				_registers.sp -= 2;
				_memory.writeWord(_registers.sp, _registers.pc + 2);
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M += 2;
				_registers.T += 8; 
			}
			else 
				_registers.pc += 2;

		} break;
		case 0xCC: // CALL Z, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_Z))
			{
				_registers.sp -= 2;
				_memory.writeWord(_registers.sp, _registers.pc + 2);
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M += 2;
				_registers.T += 8;
			}
			else
				_registers.pc += 2;
		} break;
		case 0xD4: // CALL NC, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_C))
			{
				_registers.sp -= 2;
				_memory.writeWord(_registers.sp, _registers.pc + 2);
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M += 2;
				_registers.T += 8;
			}
			else
				_registers.pc += 2;
		} break;
		case 0xDC: // CALL C, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_C))
			{
				_registers.sp -= 2;
				_memory.writeWord(_registers.sp, _registers.pc + 2);
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M += 2;
				_registers.T += 8;
			}
			else
				_registers.pc += 2;
		} break;

		// Jumps
		case 0x10: // DJNZn
		{
			signed char i = _memory.readByte(_registers.pc++);
			_registers.B--;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (_registers.B != 0)
			{
				_registers.pc += i;
				_registers.M ++;
			}
			
		} break;
		case 0xC3: // JP nn
		{
			_registers.pc = _memory.readWord(_registers.pc);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE9: // JP (HL)
		{
			_registers.pc = (_registers.H << 8) + _registers.L;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xC2: // JP NZ, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M++;
				_registers.T += 4;
			}
			else
			{
				_registers.pc += 2;
			}
		} break;
		case 0xCA: // JP Z, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M++;
				_registers.T += 4;
			}
			else
			{
				_registers.pc += 2;
			}
		} break;
		case 0xD2: // JP NC, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M++;
				_registers.T += 4;
			}
			else
			{
				_registers.pc += 2;
			}
		} break;
		case 0xDA: // JP C, nn
		{
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.pc);
				_registers.M++;
				_registers.T += 4;
			}
			else
			{
				_registers.pc += 2;
			}
		} break;

		case 0x18: // JR n
		{
			signed char n = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			_registers.pc += n;
			_registers.M++;
			_registers.T += 4;
		} break;
		case 0x20: // JR NZ,n
		{
			signed char nextByte = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_Z))
			{
				_registers.pc += nextByte;
				_registers.M++; 
				_registers.T += 4;
			}
		} break;
		case 0x28: // JR Z,n
		{
			signed char nextByte = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_Z))
			{
				_registers.pc += nextByte;
				_registers.M++;
				_registers.T += 4;
			}
		} break;
		case 0x30: // JR NC,n
		{
			signed char nextByte = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (!isFlagSet(FLAG_C))
			{
				_registers.pc += nextByte;
				_registers.M++;
				_registers.T += 4;
			}
		} break;
		case 0x38: // JR C,n
		{
			signed char nextByte = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
			if (isFlagSet(FLAG_C))
			{
				_registers.pc += nextByte;
				_registers.M++;
				_registers.T += 4;
			}
		} break;

		// Returns
		case 0xC9: // RET
		{
			_registers.pc = _memory.readWord(_registers.sp);
			_registers.sp += 2;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xC0: // RET NZ
		{
			if (!isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xC8: // RET Z
		{
			if (isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD0: // RET NC
		{
			if (!isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD8: // RET C
		{
			if (isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD9: // RETI
		{
			_registers.ime = 1;

			_registers.pc = _memory.readWord(_registers.sp);
			_registers.sp += 2;

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// Misc
		case 0x76: // HALT
		{
			if ((_memory.getIE() & _memory.getIF()) != 0)
				_halted = true;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		case 0x27: // DAA
		{		
			word s = _registers.A;

			if (isFlagSet(FLAG_N)) 
			{
				if (isFlagSet(FLAG_H)) s = (s - 0x06) & 0xFF;
				if (isFlagSet(FLAG_C)) s -= 0x60;
			}
			else 
			{
				if (isFlagSet(FLAG_H) || (s & 0xF) > 9) s += 0x06;
				if (isFlagSet(FLAG_C) || s > 0x9F) s += 0x60;
			}

			_registers.A = (s & 0xFF);
			resetFlag(FLAG_H);
			
			if (_registers.A == 0x00) 
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			
			if (s >= 0x100) 
				setFlag(FLAG_C);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF3: // DI
		{
			_registers.ime = 0;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xFB: // EI
		{
			_registers.ime = 1;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		// Restarts
		case 0xC7: // RST 0x00
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x00;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xCF: // RST 0x08
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x08;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xD7: // RST 0x10
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x10;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xDF: // RST 0x18
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x18;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xE7: // RST 0x20
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x20;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xEF: // RST 0x28
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x28;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xF7: // RST 0x30
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x30;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0xFF: // RST 0x38
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x38;
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;						
		case 0x2F: // CPL
		{
			_registers.A = ~_registers.A;
			setFlag(FLAG_N);
			setFlag(FLAG_H);
			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x37: // SCF
		{			
			setFlag(FLAG_C);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;
		case 0x3F: // CCF
		{
			if (isFlagSet(FLAG_C))
				resetFlag(FLAG_C);
			else
				setFlag(FLAG_C);

			resetFlag(FLAG_N);
			resetFlag(FLAG_H);

			_registers.M = coreInstructionTicks[_opcode] / 2;
			_registers.T = coreInstructionTicks[_opcode] * 2;
		} break;

		// CB
		case 0xCB:
		{
			_opcode      = _memory.readByte(_registers.pc++);
			_isBitOpcode = true;
			switch (_opcode)
			{
				case 0x47: // BIT 0A
				{
					if ((_registers.A & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4F: // BIT 1A
				{
					if ((_registers.A & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x57: // BIT 2A
				{
					if ((_registers.A & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5F: // BIT 3A
				{
					if ((_registers.A & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x67: // BIT 4A
				{
					if ((_registers.A & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6F: // BIT 5A
				{
					if ((_registers.A & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x77: // BIT 6A
				{
					if ((_registers.A & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7F: // BIT 7A
				{
					if ((_registers.A & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x40: // BIT 0B
				{
					if ((_registers.B & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x48: // BIT 1B
				{
					if ((_registers.B & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x50: // BIT 2B
				{
					if ((_registers.B & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x58: // BIT 3B
				{
					if ((_registers.B & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x60: // BIT 4B
				{
					if ((_registers.B & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x68: // BIT 5B
				{
					if ((_registers.B & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x70: // BIT 6B
				{
					if ((_registers.B & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x78: // BIT 7B
				{
					if ((_registers.B & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x41: // BIT 0C
				{
					if ((_registers.C & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x49: // BIT 1C
				{
					if ((_registers.C & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x51: // BIT 2C
				{
					if ((_registers.C & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x59: // BIT 3C
				{
					if ((_registers.C & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x61: // BIT 4C
				{
					if ((_registers.C & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x69: // BIT 5C
				{
					if ((_registers.C & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x71: // BIT 6C
				{
					if ((_registers.C & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x79: // BIT 7C
				{
					if ((_registers.C & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x42: // BIT 0D
				{
					if ((_registers.D & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4A: // BIT 1D
				{
					if ((_registers.D & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x52: // BIT 2D
				{
					if ((_registers.D & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5A: // BIT 3D
				{
					if ((_registers.D & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x62: // BIT 4D
				{
					if ((_registers.D & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6A: // BIT 5D
				{
					if ((_registers.D & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x72: // BIT 6D
				{
					if ((_registers.D & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7A: // BIT 7D
				{
					if ((_registers.D & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x43: // BIT 0E
				{
					if ((_registers.E & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4B: // BIT 1E
				{
					if ((_registers.E & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x53: // BIT 2E
				{
					if ((_registers.E & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5B: // BIT 3E
				{
					if ((_registers.E & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x63: // BIT 4E
				{
					if ((_registers.E & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6B: // BIT 5E
				{
					if ((_registers.E & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x73: // BIT 6E
				{
					if ((_registers.E & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7B: // BIT 7E
				{
					if ((_registers.E & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x44: // BIT 0H
				{
					if ((_registers.H & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4C: // BIT 1H
				{
					if ((_registers.H & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x54: // BIT 2H
				{
					if ((_registers.H & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5C: // BIT 3H
				{
					if ((_registers.H & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x64: // BIT 4H
				{
					if ((_registers.H & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6C: // BIT 5H
				{
					if ((_registers.H & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x74: // BIT 6H
				{
					if ((_registers.H & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7C: // BIT 7H
				{
					if ((_registers.H & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x45: // BIT 0L
				{
					if ((_registers.L & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4D: // BIT 1L
				{
					if ((_registers.L & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x55: // BIT 2L
				{
					if ((_registers.L & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5D: // BIT 3L
				{
					if ((_registers.L & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x65: // BIT 4L
				{
					if ((_registers.L & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6D: // BIT 5L
				{
					if ((_registers.L & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x75: // BIT 6L
				{
					if ((_registers.L & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7D: // BIT 7L
				{
					if ((_registers.L & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x46: // BIT 0HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x4E: // BIT 1HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x56: // BIT 2HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x5E: // BIT 3HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x66: // BIT 4HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x6E: // BIT 5HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x76: // BIT 6HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x7E: // BIT 7HL
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x87: // RES 0, A
				{
					_registers.A &= ~0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8F: // RES 1, A
				{
					_registers.A &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x97: // RES 2, A
				{
					_registers.A &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9F: // RES 3, A
				{
					_registers.A &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA7: // RES 4, A
				{
					_registers.A &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAF: // RES 5, A
				{
					_registers.A &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB7: // RES 6, A
				{
					_registers.A &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBF: // RES 7, A
				{
					_registers.A &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x80: // RES 0, B
				{
					_registers.B &= ~0x1;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x88: // RES 1, B
				{
					_registers.B &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x90: // RES 2, B
				{
					_registers.B &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x98: // RES 3, B
				{
					_registers.B &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA0: // RES 4, B
				{
					_registers.B &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA8: // RES 5, B
				{
					_registers.B &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB0: // RES 6, B
				{
					_registers.B &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB8: // RES 7, B
				{
					_registers.B &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x81: // RES 0, C
				{
					_registers.C &= ~0x1;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x89: // RES 1, C
				{
					_registers.C &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x91: // RES 2, C
				{
					_registers.C &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x99: // RES 3, C
				{
					_registers.C &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA1: // RES 4, C
				{
					_registers.C &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA9: // RES 5, C
				{
					_registers.C &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB1: // RES 6, C
				{
					_registers.C &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB9: // RES 7, C
				{
					_registers.C &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x82: // RES 0, D
				{
					_registers.D &= ~0x1;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8A: // RES 1, D
				{
					_registers.D &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x92: // RES 2, D
				{
					_registers.D &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9A: // RES 3, D
				{
					_registers.D &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA2: // RES 4, D
				{
					_registers.D &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAA: // RES 5, D
				{
					_registers.D &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB2: // RES 6, D
				{
					_registers.D &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBA: // RES 7, D
				{
					_registers.D &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x83: // RES 0, E
				{
					_registers.E &= ~0x1;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8B: // RES 1, E
				{
					_registers.E &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x93: // RES 2, E
				{
					_registers.E &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9B: // RES 3, E
				{
					_registers.E &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA3: // RES 4, E
				{
					_registers.E &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAB: // RES 5, E
				{
					_registers.E &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB3: // RES 6, E
				{
					_registers.E &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBB: // RES 7, E
				{
					_registers.E &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x84: // RES 0, H
				{
					_registers.H &= ~0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8C: // RES 1, H
				{
					_registers.H &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x94: // RES 2, H
				{
					_registers.H &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9C: // RES 3, H
				{
					_registers.H &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA4: // RES 4, H
				{
					_registers.H &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAC: // RES 5, H
				{
					_registers.H &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB4: // RES 6, H
				{
					_registers.H &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBC: // RES 7, H
				{
					_registers.H &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x85: // RES 0, L
				{
					_registers.L &= ~0x1;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8D: // RES 1, L
				{
					_registers.L &= ~0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x95: // RES 2, L
				{
					_registers.L &= ~0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9D: // RES 3, L
				{
					_registers.L &= ~0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA5: // RES 4, L
				{
					_registers.L &= ~0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAD: // RES 5, L
				{
					_registers.L &= ~0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB5: // RES 6, L
				{
					_registers.L &= ~0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBD: // RES 7, L
				{
					_registers.L &= ~0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;				
				case 0x86: // RES 0, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x1;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);					
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x8E: // RES 1, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x2;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x96: // RES 2, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x4;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x9E: // RES 3, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x8;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xA6: // RES 4, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x10;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xAE: // RES 5, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x20;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xB6: // RES 6, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x40;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xBE: // RES 7, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x80;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x17: // RL A
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.A & 0x80) != 0 ? 0x01 : 0x00;

					_registers.A <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.A |= 0x01;
					else
						_registers.A &= ~0x01;

					if (_registers.A == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x10: // RL B
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.B & 0x80) != 0 ? 0x01 : 0x00;

					_registers.B <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.B |= 0x01;
					else
						_registers.B &= ~0x01;

					if (_registers.B == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x11: // RL C
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.C & 0x80) != 0 ? 0x01 : 0x00;

					_registers.C <<= 1;
					
					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);
					
					if (prevCarry)
						_registers.C |= 0x01;
					else
						_registers.C &= ~0x01;

					if (_registers.C == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x12: // RL D
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.D & 0x80) != 0 ? 0x01 : 0x00;

					_registers.D <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.D |= 0x01;
					else
						_registers.D &= ~0x01;

					if (_registers.D == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x13: // RL E
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.E & 0x80) != 0 ? 0x01 : 0x00;

					_registers.E <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.E |= 0x01;
					else
						_registers.E &= ~0x01;

					if (_registers.E == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x14: // RL H
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.H & 0x80) != 0 ? 0x01 : 0x00;

					_registers.H <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.H |= 0x01;
					else
						_registers.H &= ~0x01;

					if (_registers.H == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x15: // RL L
				{
					byte prevCarry = getFlag(FLAG_C);
					byte prevSeventhBit = (_registers.L & 0x80) != 0 ? 0x01 : 0x00;

					_registers.L <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						_registers.L |= 0x01;
					else
						_registers.L &= ~0x01;

					if (_registers.L == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x16: // RL (HL)
				{
					byte prevCarry = getFlag(FLAG_C);
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					byte prevSeventhBit = (val & 0x80) != 0 ? 0x01 : 0x00;

					val <<= 1;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					if (prevSeventhBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (prevCarry)
						val |= 0x01;
					else
						val &= ~0x01;

					if (val == 0)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1F: // RR A
				{
					byte lsb = _registers.A & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.A >>= 1;

					if (prevCarry)
						_registers.A |= 0x80;
					else
						_registers.A &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x18: // RR B
				{
					byte lsb = _registers.B & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.B >>= 1;

					if (prevCarry)
						_registers.B |= 0x80;
					else
						_registers.B &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x19: // RR C
				{
					byte lsb = _registers.C & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.C >>= 1;

					if (prevCarry)
						_registers.C |= 0x80;
					else
						_registers.C &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1A: // RR D
				{
					byte lsb = _registers.D & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.D >>= 1;

					if (prevCarry)
						_registers.D |= 0x80;
					else
						_registers.D &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1B: // RR E
				{
					byte lsb = _registers.E & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.E >>= 1;

					if (prevCarry)
						_registers.E |= 0x80;
					else
						_registers.E &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1C: // RR H
				{
					byte lsb = _registers.H & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.H >>= 1;

					if (prevCarry)
						_registers.H |= 0x80;
					else
						_registers.H &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1D: // RR L
				{
					byte lsb = _registers.L & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.L >>= 1;

					if (prevCarry)
						_registers.L |= 0x80;
					else
						_registers.L &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x1E: // RR (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					byte lsb = val & 0x01;
					byte prevCarry = getFlag(FLAG_C);

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					val >>= 1;

					if (prevCarry)
						val |= 0x80;
					else
						val &= ~0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0F: // RRCA
				{
					byte oldBit0 = _registers.A & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);
					
					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.A >>= 1;

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x08: // RRC B
				{
					byte oldBit0 = _registers.B & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.B >>= 1;

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x09: // RRC C
				{
					byte oldBit0 = _registers.C & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.C >>= 1;

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0A: // RRC D
				{
					byte oldBit0 = _registers.D & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.D >>= 1;

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0B: // RRC E
				{
					byte oldBit0 = _registers.E & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.E >>= 1;

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0C: // RRC H
				{
					byte oldBit0 = _registers.H & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.H >>= 1;

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0D: // RRC L
				{
					byte oldBit0 = _registers.L & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.L >>= 1;

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x0E: // RRC (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					byte oldBit0 = val & 0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					val >>= 1;

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x07: // RLC A
				{
					byte oldBit7 = _registers.A & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.A <<= 1;

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x00: // RLC B
				{
					byte oldBit7 = _registers.B & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.B <<= 1;

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x01: // RLC C
				{
					byte oldBit7 = _registers.C & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.C <<= 1;

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x02: // RLC D
				{
					byte oldBit7 = _registers.D & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.D <<= 1;

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x03: // RLC E
				{
					byte oldBit7 = _registers.E & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.E <<= 1;

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x04: // RLC H
				{
					byte oldBit7 = _registers.H & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.H <<= 1;

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x05: // RLC L
				{
					byte oldBit7 = _registers.L & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.L <<= 1;

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x06: // RLC (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					byte oldBit7 = val & 0x80;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (oldBit7)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					val <<= 1;

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);
					
					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				// Sets
				case 0xC7: // SET 0, A
				{
					_registers.A |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCF: // SET 1, A
				{
					_registers.A |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD7: // SET 2, A
				{
					_registers.A |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDF: // SET 3, A
				{
					_registers.A |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE7: // SET 4, A
				{
					_registers.A |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xEF: // SET 5, A
				{
					_registers.A |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF7: // SET 6, A
				{
					_registers.A |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xFF: // SET 7, A
				{
					_registers.A |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC0: // SET 0, B
				{
					_registers.B |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC8: // SET 1, B
				{
					_registers.B |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD0: // SET 2, B
				{
					_registers.B |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD8: // SET 3, B
				{
					_registers.B |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE0: // SET 4, B
				{
					_registers.B |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE8: // SET 5, B
				{
					_registers.B |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF0: // SET 6, B
				{
					_registers.B |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF8: // SET 7, B
				{
					_registers.B |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC1: // SET 0, C
				{
					_registers.C |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC9: // SET 1, C
				{
					_registers.C |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD1: // SET 2, C
				{
					_registers.C |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD9: // SET 3, C
				{
					_registers.C |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE1: // SET 4, C
				{
					_registers.C |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE9: // SET 5, C
				{
					_registers.C |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF1: // SET 6, C
				{
					_registers.C |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF9: // SET 7, C
				{
					_registers.C |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC2: // SET 0, D
				{
					_registers.D |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCA: // SET 1, D
				{
					_registers.D |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD2: // SET 2, D
				{
					_registers.D |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDA: // SET 3, D
				{
					_registers.D |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE2: // SET 4, D
				{
					_registers.D |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xEA: // SET 5, D
				{
					_registers.D |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF2: // SET 6, D
				{
					_registers.D |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xFA: // SET 7, D
				{
					_registers.D |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC3: // SET 0, E
				{
					_registers.E |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCB: // SET 1, E
				{
					_registers.E |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD3: // SET 2, E
				{
					_registers.E |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDB: // SET 3, E
				{
					_registers.E |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE3: // SET 4, E
				{
					_registers.E |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xEB: // SET 5, E
				{
					_registers.E |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF3: // SET 6, E
				{
					_registers.E |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xFB: // SET 7, E
				{
					_registers.E |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC4: // SET 0, H
				{
					_registers.H |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCC: // SET 1, H
				{
					_registers.H |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD4: // SET 2, H
				{
					_registers.H |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDC: // SET 3, H
				{
					_registers.H |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE4: // SET 4, H
				{
					_registers.H |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xEC: // SET 5, H
				{
					_registers.H |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF4: // SET 6, H
				{
					_registers.H |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xFC: // SET 7, H
				{
					_registers.H |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC5: // SET 0, L
				{
					_registers.L |= 0x01;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCD: // SET 1, L
				{
					_registers.L |= 0x02;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD5: // SET 2, L
				{
					_registers.L |= 0x04;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDD: // SET 3, L
				{
					_registers.L |= 0x08;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE5: // SET 4, L
				{
					_registers.L |= 0x10;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xED: // SET 5,L
				{
					_registers.L |= 0x20;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF5: // SET 6, L
				{
					_registers.L |= 0x40;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xFD: // SET 7, L
				{
					_registers.L |= 0x80;
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xC6: // SET 0, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x01;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xCE: // SET 1, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x02;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xD6: // SET 2, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x04;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xDE: // SET 3, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x08;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xE6: // SET 4, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x10;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xEE: // SET 5, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x20;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0xF6: // SET 6, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x40;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;				
				case 0xFE: // SET 7, (HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x80;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;

				// Shifts
				case 0x27: // SLA A
				{
					if ((_registers.A & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.A <<= 1;
					_registers.A &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x20: // SLA B
				{
					if ((_registers.B & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.B <<= 1;
					_registers.B &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x21: // SLA C
				{
					if ((_registers.C & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.C <<= 1;
					_registers.C &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x22: // SLA D
				{
					if ((_registers.D & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.D <<= 1;
					_registers.D &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x23: // SLA E
				{
					if ((_registers.E & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.E <<= 1;
					_registers.E &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x24: // SLA H
				{
					if ((_registers.H & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.H <<= 1;
					_registers.H &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x25: // SLA L
				{
					if ((_registers.L & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					_registers.L <<= 1;
					_registers.L &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x26: // SLA (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					if ((val & 0x80) != 0)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					val <<= 1;
					val &= ~0x01;

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2F: // SRA A
				{
					byte msb = _registers.A & 0x80;
					byte lsb = _registers.A & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.A >>= 1;

					if (msb)
						_registers.A |= 0x80;
					else
						_registers.A &= ~0x80;

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x28: // SRA B
				{
					byte msb = _registers.B & 0x80;
					byte lsb = _registers.B & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.B >>= 1;

					if (msb)
						_registers.B |= 0x80;
					else
						_registers.B &= ~0x80;

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x29: // SRA C
				{
					byte msb = _registers.C & 0x80;
					byte lsb = _registers.C & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.C >>= 1;

					if (msb)
						_registers.C |= 0x80;
					else
						_registers.C &= ~0x80;

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2A: // SRA D
				{
					byte msb = _registers.D & 0x80;
					byte lsb = _registers.D & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.D >>= 1;

					if (msb)
						_registers.D |= 0x80;
					else
						_registers.D &= ~0x80;

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2B: // SRA E
				{
					byte msb = _registers.E & 0x80;
					byte lsb = _registers.E & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.E >>= 1;

					if (msb)
						_registers.E |= 0x80;
					else
						_registers.E &= ~0x80;

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2C: // SRA H
				{
					byte msb = _registers.H & 0x80;
					byte lsb = _registers.H & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.H >>= 1;

					if (msb)
						_registers.H |= 0x80;
					else
						_registers.H &= ~0x80;

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2D: // SRA L
				{
					byte msb = _registers.L & 0x80;
					byte lsb = _registers.L & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					_registers.L >>= 1;

					if (msb)
						_registers.L |= 0x80;
					else
						_registers.L &= ~0x80;

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x2E: // SRA (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					byte msb = val & 0x80;
					byte lsb = val & 0x01;

					if (lsb)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					resetFlag(FLAG_H);
					resetFlag(FLAG_N);

					val >>= 1;

					if (msb)
						val |= 0x80;
					else
						val &= ~0x80;

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3F: // SRL A
				{
					byte prevBit = _registers.A & 0x01;

					_registers.A >>= 1;

					_registers.A &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];

				} break;

				case 0x38: // SRL B
				{
					byte prevBit = _registers.B & 0x01;

					_registers.B >>= 1;

					_registers.B &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x39: // SRL C
				{
					byte prevBit = _registers.C & 0x01;

					_registers.C >>= 1;

					_registers.C &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3A: // SRL D
				{
					byte prevBit = _registers.D & 0x01;

					_registers.D >>= 1;

					_registers.D &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3B: // SRL E
				{
					byte prevBit = _registers.E & 0x01;

					_registers.E >>= 1;

					_registers.E &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3C: // SRL H
				{
					byte prevBit = _registers.H & 0x01;

					_registers.H >>= 1;

					_registers.H &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3D: // SRL L
				{
					byte prevBit = _registers.L & 0x01;

					_registers.L >>= 1;

					_registers.L &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x3E: // SRL (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);

					byte prevBit = val & 0x01;

					val >>= 1;
					val &= ~0x80;

					if (prevBit)
						setFlag(FLAG_C);
					else
						resetFlag(FLAG_C);

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_N);
					resetFlag(FLAG_H);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;

				// Swaps
				case 0x37: // SWAP A
				{
					_registers.A = ((_registers.A & 0x0F) << 4 |
						            (_registers.A & 0xF0) >> 4);

					if (_registers.A == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x30: // SWAP B
				{
					_registers.B = ((_registers.B & 0x0F) << 4 |
						            (_registers.B & 0xF0) >> 4);

					if (_registers.B == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x31: // SWAP C
				{
					_registers.C = ((_registers.C & 0x0F) << 4 |
						            (_registers.C & 0xF0) >> 4);

					if (_registers.C == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x32: // SWAP D
				{
					_registers.D = ((_registers.D & 0x0F) << 4 |
						            (_registers.D & 0xF0) >> 4);

					if (_registers.D == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x33: // SWAP E
				{
					_registers.E = ((_registers.E & 0x0F) << 4 |
						            (_registers.E & 0xF0) >> 4);

					if (_registers.E == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x34: // SWAP H
				{
					_registers.H = ((_registers.H & 0x0F) << 4 |
						            (_registers.H & 0xF0) >> 4);

					if (_registers.H == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x35: // SWAP L
				{
					_registers.L = ((_registers.L & 0x0F) << 4 |
						            (_registers.L & 0xF0) >> 4);

					if (_registers.L == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;
				case 0x36: // SWAP (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);

					val = ((val & 0x0F) << 4 |
						   (val & 0xF0) >> 4);

					if (val == 0x00)
						setFlag(FLAG_Z);
					else
						resetFlag(FLAG_Z);

					resetFlag(FLAG_H);
					resetFlag(FLAG_C);
					resetFlag(FLAG_N);

					_memory.writeByte((_registers.H << 8) + _registers.L, val);

					_registers.M = cbInstructionTicks[_opcode] / 4;
					_registers.T = cbInstructionTicks[_opcode];
				} break;

				default:
				{
					std::cout << ">>> at: 0x" << std::hex << _registers.pc << " unimplemented cb instruction: 0x" << std::hex << static_cast<int>(_opcode) << " <<<" << std::endl;
					_errorState = ES_UNIMPLEMENTED_INSTRUCTION;
				} 
			}
		} break;

		default:
		{
			std::cout << ">>> at: 0x" << std::hex << _registers.pc << " unimplemented instruction: 0x" << std::hex << static_cast<int>(_opcode) << " <<<" << std::endl;
			_errorState = ES_UNIMPLEMENTED_INSTRUCTION;
		}
	}

	_internalM += _registers.M;
	_internalT += _registers.T;	
	incrementTimer();
}

void Cpu::handleInterrupts()
{
	if (getIME() && _memory.getIE() && _memory.getIF())
	{
		byte maskedInterrupts = _memory.getIE() & _memory.getIF();

		if ((maskedInterrupts & Memory::INTERRUPT_FLAG_VBLANK) != 0)
		{
			_memory.resetInterrupt(Memory::INTERRUPT_FLAG_VBLANK);
			RST40();
			_halted = false;
			incrementTimer();
		}
		else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_TOGGLELCD) != 0)
		{
			_memory.resetInterrupt(Memory::INTERRUPT_FLAG_TOGGLELCD);
			RST48();
			_halted = false;
			incrementTimer();
		}
		else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_TIMER) != 0)
		{
			_memory.resetInterrupt(Memory::INTERRUPT_FLAG_TIMER);
			RST50();
			_halted = false;
			incrementTimer();
		}
		else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_SERIAL) != 0)
		{
			_memory.resetInterrupt(Memory::INTERRUPT_FLAG_SERIAL);
			RST58();
			_halted = false;
			incrementTimer();
		}
		else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_JOYPAD) != 0)
		{
			_memory.resetInterrupt(Memory::INTERRUPT_FLAG_JOYPAD);
			RST60();
			_halted = false;
			incrementTimer();
		}
	}
}

void Cpu::RST40()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_VBLANK;
	_registers.M = coreInstructionTicks[_opcode] / 2;
	_registers.T = coreInstructionTicks[_opcode] * 2;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST48()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_LCD;
	_registers.M = coreInstructionTicks[_opcode] / 2;
	_registers.T = coreInstructionTicks[_opcode] * 2;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST50()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_TIMER;
	_registers.M = coreInstructionTicks[_opcode] / 2;
	_registers.T = coreInstructionTicks[_opcode] * 2;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST58()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_SLINK;
	_registers.M = coreInstructionTicks[_opcode] / 2;
	_registers.T = coreInstructionTicks[_opcode] * 2;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST60()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_JOYPAD;
	_registers.M = coreInstructionTicks[_opcode] / 2;
	_registers.T = coreInstructionTicks[_opcode] * 2;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::resetCpu()
{
	_registers = {};
	_registers.ime = 1;
	_internalM = 0;
	_internalT = 0;
	_clockSub = 0;
	_clockDiv = 0;
	_clockMain = 0;

	_registers.pc = 0x0000;

	_opcode      = NO_OPCODE;
	_isBitOpcode = false;
	_halted      = false;
}

void Cpu::printRegisters()
{
	//if (_errorState == ES_UNIMPLEMENTED_INSTRUCTION) return;
	
	auto opcodeDisassembly = _isBitOpcode ? s_bitOpcodeDisassembly.at(_opcode) : s_instrDisassembly.at(_opcode);
	
	std::cout << "---------- Registers for: " << opcodeDisassembly << " [0x" << std::hex << static_cast<int>(_opcode) << "] ----------" << std::endl;	


	std::cout << "PC: 0x" << std::hex << _registers.pc << std::endl;
	std::cout << "SP: 0x" << std::hex << _registers.sp << std::endl;
	std::cout << "A:  0x" << std::hex << static_cast<int>(_registers.A)<< std::endl;
	std::cout << "B:  0x" << std::hex << static_cast<int>(_registers.B) << std::endl;
	std::cout << "C:  0x" << std::hex << static_cast<int>(_registers.C) << std::endl;
	std::cout << "D:  0x" << std::hex << static_cast<int>(_registers.D) << std::endl;
	std::cout << "E:  0x" << std::hex << static_cast<int>(_registers.E) << std::endl;
	std::cout << "H:  0x" << std::hex << static_cast<int>(_registers.H) << std::endl;
	std::cout << "L:  0x" << std::hex << static_cast<int>(_registers.L) << std::endl;
	std::cout << "F:  0x" << std::hex << static_cast<int>(_registers.F) << std::endl;
	std::cout << "Flags: Z = 0x" << static_cast<int>(getFlag(FLAG_Z)) << "    N = 0x" << static_cast<int>(getFlag(FLAG_N)) <<
		         "    H = 0x" << static_cast<int>(getFlag(FLAG_H))  << "    C = 0x" << static_cast<int>(getFlag(FLAG_C)) << std::endl;
	std::cout << "Clock: M = " << _registers.M << "    T = " << _registers.T << std::endl;
	std::cout << "Internal Clock: M = " << _internalM << "    T = " << _internalT << std::endl;
	std::cout << "IME: 0x" << std::hex << (int)_registers.ime << " IE: 0x" << std::hex << (int)_memory.getIE() << " IF: 0x" << std::hex << (int)_memory.getIF() << std::endl;
	std::cout << "ROM Bank: " << std::dec << (int)_memory.getCurrentRomBank() << std::endl;
	std::cout << "------------------------------------------" << std::endl;
}

const word* Cpu::getPC() const { return &_registers.pc; }
const timer_t* Cpu::getT() const { return &_registers.T; }

bool Cpu::isFlagSet(const byte flag) const { return (_registers.F & flag) != 0; }
byte Cpu::getFlag(const byte flag) const { return (_registers.F & flag) != 0 ? 0x1 : 0x0; }
byte Cpu::getLastExecutedOpcode() const { return _opcode; }
byte Cpu::getIME() const { return _registers.ime; }

void Cpu::setFlag(const byte flag)   { _registers.F |= flag; }
void Cpu::resetFlag(const byte flag) { _registers.F &= ~flag; }

void Cpu::incrementTimer()
{
	_clockSub += (byte)_registers.M;

	if (_clockSub > 3)
	{
		_clockMain++;
		_clockSub -= 4;

		if (++_clockDiv == 16)
		{
			_memory.incrementDiv();
			_clockDiv = 0;
		}
	}

	byte tac = _memory.readByte(0xFF07);
	byte inputClockSel = tac & 0x3;

	switch (inputClockSel)
	{
		case 0x00: if (_clockMain >= 64) stepTimer(); break; // 4K
		case 0x01: if (_clockMain >= 1)  stepTimer(); break; // 256K
		case 0x02: if (_clockMain >= 4)  stepTimer(); break; // 64K
		case 0x03: if (_clockMain >= 16) stepTimer(); break; // 16K
	}
}

void Cpu::stepTimer()
{
	_clockMain = 0;

	byte tima = _memory.readByte(0xFF05);

	if (tima == 0xFF)
	{
		byte tma = _memory.readByte(0xFF06);
		_memory.writeByte(0xFF05, tma);
		*(_memory.getIFPtr()) |= Memory::INTERRUPT_FLAG_TIMER;
	}
	else
	{
		_memory.writeByte(0xFF05, tima + 1);
	}
}