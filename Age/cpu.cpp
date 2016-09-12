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
	{ 0x12, "LD (DE), A" },
	{ 0xEA, "LD (nn), A" },
	{ 0x36, "LD (HL), n" },

	// 16 bit Loads
	{ 0x08, "LD nn, sp" },
	{ 0x01, "LD BC, nn(16 bits)" },
	{ 0x11, "LD DE, nn(16 bits)" },
	{ 0x21, "LD HL, nn(16 bits)" },
	{ 0x31, "LD SP, nn(16 bits)" },
	{ 0x73, "LD (HL), E" },
	{ 0x32, "LDD (HL), A"},
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
	{ 0xF6, "OR A, #" },
	{ 0xAF, "XOR A, A" },
	{ 0xA8, "XOR A, B" },
	{ 0xA9, "XOR A, C" },
	{ 0xAA, "XOR A, D" },
	{ 0xAB, "XOR A, E" },
	{ 0xAC, "XOR A, H" },
	{ 0xAD, "XOR A, L" },
	{ 0xEE, "XOR A, #" },
	{ 0x8F, "ADC A, A" },
	{ 0x88, "ADC A, B" },
	{ 0x89, "ADC A, C" },
	{ 0x8A, "ADC A, D" },
	{ 0x8B, "ADC A, E" },
	{ 0x8C, "ADC A, H" },
	{ 0x8D, "ADC A, L" },

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

	// Rotates & Shifts
	{ 0x0F, "RRCA" },
	{ 0x17, "RL A" },
	{ 0x07, "RLCA" },

	// Calls
	{ 0xCD, "CALL nn" },
	{ 0xC4, "CALL NZ, nn" },
	{ 0xCC, "CALL Z, nn" },
	{ 0xD4, "CALL NC, nn" },
	{ 0xDC, "CALL C, nn" },

	// Jumps
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
	{ 0x3F, "SCF" }
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
	{ 0x7C, "BIT 7H" },
	{ 0x41, "BIT 0C" },
	{ 0x49, "BIT 1C" },
	{ 0x51, "BIT 2C" },
	{ 0x59, "BIT 3C" },
	{ 0x61, "BIT 4C" },
	{ 0x69, "BIT 5C" },
	{ 0x71, "BIT 6C" },
	{ 0x79, "BIT 7C" },
	{ 0x46, "BIT 0HL" },
	{ 0x4E, "BIT 1HL" },
	{ 0x56, "BIT 2HL" },
	{ 0x5E, "BIT 3HL" },
	{ 0x66, "BIT 4HL" },
	{ 0x6E, "BIT 5HL" },
	{ 0x76, "BIT 6HL" },
	{ 0x7E, "BIT 7HL" },
	{ 0x4F, "BIT 1A" },
	{ 0x87, "RES 0, A" },
	{ 0x80, "RES 0, B" },
	{ 0x81, "RES 0, C" },
	{ 0x82, "RES 0, D" },
	{ 0x83, "RES 0, E" },
	{ 0x84, "RES 0, H" },
	{ 0x85, "RES 0, L" },
	{ 0x86, "RES 0, (HL) "},
	{ 0x11, "RL C" },
	
	// Sets
	{ 0xFE, "SET 7,(HL)" },
	
	// Shifts
	{ 0x27, "SLA A" },
	{ 0x20, "SLA B" },
	{ 0x21, "SLA C" },
	{ 0x22, "SLA D" },
	{ 0x23, "SLA E" },
	{ 0x24, "SLA H" },
	{ 0x25, "SLA L" },
	{ 0x3F, "SRL A" },
	{ 0x38, "SRL B" },
	{ 0x39, "SRL C" },
	{ 0x3A, "SRL D" },
	{ 0x3B, "SRL E" },
	{ 0x3C, "SRL H" },
	{ 0x3D, "SRL L" },

	// Swap
	{ 0x37, "SWAP A" },
	{ 0x30, "SWAP B" },
	{ 0x31, "SWAP C" },
	{ 0x32, "SWAP D" },
	{ 0x33, "SWAP E" },
	{ 0x34, "SWAP H" },
	{ 0x35, "SWAP L" }
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
	_opcode      = _memory.readByte(_registers.pc++);
	_isBitOpcode = false;

	switch (_opcode)
	{
		
		case 0x00: // NOP
		{
			_registers.M = 1;
			_registers.T = 4;
		} break;

		// 8 Bit Loads
		case 0x06: // LD B,n
		{
			_registers.B = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x0E: // LD C,n
		{
			_registers.C = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x16: // LD D,n
		{
			_registers.D = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x1E: // LD E,n
		{
			_registers.E = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x26: // LD H,n
		{
			_registers.H = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x2E: // LD L,n
		{
			_registers.L = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x7F: // LD A, A
		{
			_registers.A = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x78: // LD A, B
		{
			_registers.A = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x79: // LD A, C
		{
			_registers.A = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x7A: // LD A, D
		{
			_registers.A = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x7B: // LD A, E
		{
			_registers.A = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x7C: //LD A, H
		{
			_registers.A = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x7D: // LD A, L
		{
			_registers.A = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0xF2: // LD A, (C)
		{
			_registers.A = _memory.readByte(0xFF00 + _registers.C);
			_registers.M = 2;
			_registers.T = 4;
		} break;
		case 0x7E: // LD A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x0A: // LD A, (BC)
		{
			_registers.A = _memory.readByte((_registers.B << 8) + _registers.C);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x1A: // LD A, (DE)
		{
			_registers.A = _memory.readByte((_registers.D << 8) + _registers.E);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xFA: // LD A, (nn)
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_registers.A = _memory.readByte(address);
			_registers.M = 4;
			_registers.T = 16;
		} break;
		case 0x47: // LD B, A
		{
			_registers.B = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x40: // LD B, B
		{
			_registers.B = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x41: // LD B, C
		{
			_registers.B = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x42: // LD B, D
		{
			_registers.B = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x43: // LD B, E
		{
			_registers.B = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x44: // LD B, H
		{
			_registers.B = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x45: // LD B, L
		{
			_registers.B = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x46: // LD B, (HL)
		{
			_registers.B = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x4F: // LD C, A
		{
			_registers.C = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x4E: // LD C, (HL)
		{
			_registers.C = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		}
		case 0x57: // LD D, A
		{
			_registers.D = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x50: // LD D, B
		{
			_registers.D = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x51: // LD D, C
		{
			_registers.D = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x52: // LD D, D
		{
			_registers.D = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x53: // LD D, E
		{
			_registers.D = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x54: // LD D, H
		{
			_registers.D = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x55: // LD D, L
		{
			_registers.D = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x56: // LD D, (HL)
		{
			_registers.D = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x5F: // LD E, A
		{
			_registers.E = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x58: // LD E, B
		{
			_registers.E = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x59: // LD E, C
		{
			_registers.E = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5A: // LD E, D
		{
			_registers.E = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5B: // LD E, E
		{
			_registers.E = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5C: // LD E, H
		{
			_registers.E = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5D: // LD E, L
		{
			_registers.E = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5E: // LD E, (HL)
		{
			_registers.E = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x66: // LD H, (HL)
		{
			_registers.H = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x67: // LD H, A
		{
			_registers.H = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x60: // LD H, B
		{
			_registers.H = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x61: // LD H, C
		{
			_registers.H = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x62: // LD H, D
		{
			_registers.H = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x63: // LD H, E
		{
			_registers.H = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x64: // LD H, H
		{
			_registers.H = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x65: // LD H, L
		{
			_registers.H = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x6F: // LD L, A
		{
			_registers.L = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x68: // LD L, B
		{
			_registers.L = _registers.B;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x69: // LD L, C
		{
			_registers.L = _registers.C;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x6A: // LD L, D
		{
			_registers.L = _registers.D;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x6B: // LD L, E
		{
			_registers.L = _registers.E;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x6C: // LD L, H
		{
			_registers.L = _registers.H;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x6D: // LD L, L"
		{
			_registers.L = _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x77: // LD (HL), A
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.A);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x70: // LD (HL), B
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.B);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x71: // LD (HL), C
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.C);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x72: // LD (HL), D
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.D);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x73: // LD (HL), E
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.E);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x74: // LD (HL), H
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.H);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x75: // LD (HL), L
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.L);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x3E: // LD A, #
		{
			_registers.A = _memory.readByte(_registers.pc++);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xE0: // LDH (n), A
		{
			byte address = _memory.readByte(_registers.pc++);
			_memory.writeByte(0xFF00 + address, _registers.A);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0xE2: // LD (C), A
		{			
			_memory.writeByte(0xFF00 + _registers.C, _registers.A);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xF0: // LDH A,(n)
		{
			word address = _memory.readByte(_registers.pc++) + 0xFF00;
			_registers.A = _memory.readByte(address);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x22: // LDI (HL), A
		{
			_memory.writeByte((_registers.H << 8) + _registers.L, _registers.A);
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;
			
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x2A: // LDI A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x3A: // LDD A, (HL)
		{
			_registers.A = _memory.readByte((_registers.H << 8) + _registers.L);
			_registers.L--;
			if (_registers.L == 0xFF)
				_registers.H--;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x12: // LD (DE) A
		{
			_memory.writeByte((_registers.D << 8) + _registers.E, _registers.A);
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xEA: // LD (nn), A
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_memory.writeByte(address, _registers.A);
			_registers.M = 4;
			_registers.T = 16;
		} break;
		case 0x36: // LD (HL), n
		{
			byte nextByte = _memory.readByte(_registers.pc++);
			_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
			_registers.M = 3;
			_registers.T = 12;
		} break;

		// 16 Bit Loads
		case 0x08: // LD nn,sp
		{
			word address = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_memory.writeWord(address, _registers.sp);
			_registers.M = 5;
			_registers.T = 20;			
		} break;
		case 0x01: // LD BC, nn
		{
			_registers.C = _memory.readByte(_registers.pc++);
			_registers.B = _memory.readByte(_registers.pc++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x11: // LD DE, nn
		{
			_registers.E = _memory.readByte(_registers.pc++);
			_registers.D = _memory.readByte(_registers.pc++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x21: // LD HL,nn
		{
			_registers.L = _memory.readByte(_registers.pc++);
			_registers.H = _memory.readByte(_registers.pc++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x31: // LD SP,nn
		{
			_registers.sp = _memory.readWord(_registers.pc);
			_registers.pc += 2;
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x32: // LDD (HL), A 
		{
			_memory.writeByte((_registers.H << 8) + _registers.L, _registers.A);
			_registers.L--;
			if (_registers.L == 0xFF)
				_registers.H--;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xF5: // PUSH AF
		{
			_memory.writeByte(--_registers.sp, _registers.A);
			_memory.writeByte(--_registers.sp, _registers.F);
			_registers.M = 4;
			_registers.T = 16;

		} break;
		case 0xC5: // PUSH BC
		{
			_memory.writeByte(--_registers.sp, _registers.B);
			_memory.writeByte(--_registers.sp, _registers.C);
			_registers.M = 4;
			_registers.T = 16;
		} break;
		case 0xD5: // PUSH DE
		{
			_memory.writeByte(--_registers.sp, _registers.D);
			_memory.writeByte(--_registers.sp, _registers.E);
			_registers.M = 4;
			_registers.T = 16;
		} break;
		case 0xE5: // PUSH HL
		{
			_memory.writeByte(--_registers.sp, _registers.H);
			_memory.writeByte(--_registers.sp, _registers.L);
			_registers.M = 4;
			_registers.T = 16;
		} break;
		case 0xF1: // POP AF
		{
			_registers.F = _memory.readByte(_registers.sp++);
			_registers.A = _memory.readByte(_registers.sp++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0xC1: // POP BC
		{
			_registers.C = _memory.readByte(_registers.sp++);
			_registers.B = _memory.readByte(_registers.sp++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0xD1: // POP DE
		{
			_registers.E = _memory.readByte(_registers.sp++);
			_registers.D = _memory.readByte(_registers.sp++);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0xE1: // POP HL
		{
			_registers.L = _memory.readByte(_registers.sp++);
			_registers.H = _memory.readByte(_registers.sp++);
			_registers.M = 3;
			_registers.T = 12;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x3D: // DEC A
		{
			if (_registers.A != 0x10)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.A--;
			if (_registers.A == 0) 
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x35: // DEC HL
		{
			byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
			
			
			if (nextByte != 0x10)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			
			_memory.writeByte((_registers.H << 8) + _registers.L, --nextByte);

			if (nextByte == 0)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			setFlag(FLAG_N);

			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0x87: // ADD A, A
		{
			if (0xFF - _registers.A <= _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.A & 0x0F))
			resetFlag(FLAG_N);

			_registers.A += _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x80: // ADD A, B
		{
			if (0xFF - _registers.A <= _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.B & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x81: // ADD A, C
		{
			if (0xFF - _registers.A <= _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.C & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x82: // ADD A, D
		{
			if (0xFF - _registers.A <= _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.D & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x83: // ADD A, E
		{
			if (0xFF - _registers.A <= _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.E & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x84: // ADD A, H
		{
			if (0xFF - _registers.A <= _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.H & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x85: // ADD A, L
		{
			if (0xFF - _registers.A <= _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (_registers.L & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x86: // ADD A, HL
		{
			byte val = _memory.readByte((_registers.H << 8) + _registers.L);
			if (0xFF - _registers.A <= val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (val & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xC6: // AND A, #
		{
			byte val = _memory.readByte(_registers.pc++);
			if (0xFF - _registers.A <= val)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if (0x0F - (_registers.A & 0x0F) <= (val & 0x0F))
				resetFlag(FLAG_N);

			_registers.A += val;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 2;
			_registers.T = 8;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 2;
			_registers.T = 8;
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

			_registers.M = 2;
			_registers.T = 8;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 2;
			_registers.T = 8;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 2;
			_registers.T = 8;
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

			if ((_registers.A & 0x0F) >= (val & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.M = 2;
			_registers.T = 8;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 1;
			_registers.T = 4;
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
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x8F: // ADC A, A
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.A)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.A;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x88: // ADC A, B
		{			
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.B)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.B;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x89: // ADC A, C
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.C)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.C;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x8A: // ADC A, D
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.D)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.D;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x8B: // ADC A, E
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.E)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.E;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x8C: // ADC A, H
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.H)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.H;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x8D: // ADC A, L
		{
			if (0x0F - (_registers.A & 0x0F) <= getFlag(FLAG_C) + (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (0xFF - _registers.A <= getFlag(FLAG_C) + _registers.L)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			resetFlag(FLAG_N);

			_registers.A += getFlag(FLAG_C) + _registers.L;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			_registers.M = 1;
			_registers.T = 4;
		} break;

		// 16-bit ALU
		case 0x03: // INC BC
		{
			if (_registers.C == 0xFF)
				_registers.B++;
			_registers.C++;
			
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x13: // INC DE
		{
			if (_registers.E == 0xFF)
				_registers.D++;
			_registers.E++;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x23: // INC HL
		{
			if (_registers.L == 0xFF)
				_registers.H++;
			_registers.L++;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x33: // INC SP
		{
			_registers.sp++;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x0B: // DEC BC
		{
			if (_registers.C == 0x00)
				_registers.B--;
			_registers.C--;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x1B: // DEC DE
		{
			if (_registers.E == 0x00)
				_registers.D--;
			_registers.E--;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x2B: // DEC HL
		{
			if (_registers.L == 0x00)
				_registers.H--;
			_registers.L--;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x3B: // DEC SP 
		{
			_registers.sp--;
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x09: // ADD HL, BC
		{
			int BC = (_registers.B << 8) + _registers.C;
			int HL = (_registers.H << 8) + _registers.L;
			HL += BC;

			if (HL > 0xFFFF) 
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((((HL & 0xFFFF) & 0x0F) + (BC & 0x0F)) > 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;
			
			_registers.M = 2;
			_registers.T = 8;

		} break;
		case 0x19: // ADD HL, DE
		{
			int DE = (_registers.D << 8) + _registers.E;
			int HL = (_registers.H << 8) + _registers.L;
			HL += DE;

			if (HL > 0xFFFF)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((((HL & 0xFFFF) & 0x0F) + (DE & 0x0F)) > 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x29: // ADD HL, HL
		{
			int HL2 = (_registers.H << 8) + _registers.L;
			int HL = (_registers.H << 8) + _registers.L;
			HL += HL2;

			if (HL > 0xFFFF)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((((HL & 0xFFFF) & 0x0F) + (HL2 & 0x0F)) > 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0x39: // ADD HL, SP
		{
			int SP = _registers.sp;
			int HL = (_registers.H << 8) + _registers.L;
			HL += SP;

			if (HL > 0xFFFF)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			if ((((HL & 0xFFFF) & 0x0F) + (SP & 0x0F)) > 0x0F)
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			_registers.H = (HL >> 8) & 0xFF;
			_registers.L = HL & 0xFF;

			_registers.M = 2;
			_registers.T = 8;
		} break;

		// Rotates & Shifts
		case 0x0F: // RRCA
		{
			if ((_registers.A & 0x01) != 0)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);
			
			_registers.A >>= 1;
			if (_registers.A == 0x00)
				setFlag(FLAG_Z);

			resetFlag(FLAG_H);
			resetFlag(FLAG_N);

			_registers.M = 1;
			_registers.T = 4;
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

			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x07: // RLCA
		{
			byte prevBit = (_registers.A & 0x80) != 0 ? 1 : 0;

			_registers.A <<= 1;

			if (_registers.A == 0x00)
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);

			resetFlag(FLAG_N);
			resetFlag(FLAG_H);

			if (prevBit)
				setFlag(FLAG_C);
			else
				resetFlag(FLAG_C);

			_registers.M = 1;
			_registers.T = 4;
		} break;

		// Calls
		case 0xCD: // Call nn
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc + 2);
			_registers.pc = _memory.readWord(_registers.pc);
			_registers.M = 5;
			_registers.T = 20; //TODO: check correct timing manual says 12Cycles
		} break;
		case 0xC4: // CALL NZ, nn
		{
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
		case 0xC3: // JP nn
		{
			_registers.pc = _memory.readWord(_registers.pc);
			_registers.M = 3;
			_registers.T = 12;
		} break;
		case 0xE9: // JP (HL)
		{
			_registers.pc = (_registers.H << 8) + _registers.L;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0xC2: // JP NZ, nn
		{
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 3;
			_registers.T = 12;
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
			_registers.M = 2;
			_registers.T = 8;
			_registers.pc += n;
			_registers.M++;
			_registers.T += 4;
		} break;
		case 0x20: // JR NZ,n
		{
			signed char nextByte = _memory.readByte(_registers.pc);
			_registers.pc++;
			_registers.M = 2;
			_registers.T = 8;
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
			_registers.M = 2;
			_registers.T = 8;
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
			_registers.M = 2;
			_registers.T = 8;
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
			_registers.M = 2;
			_registers.T = 8;
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

			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xC0: // RET NZ
		{
			if (!isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xC8: // RET Z
		{
			if (isFlagSet(FLAG_Z))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xD0: // RET NC
		{
			if (!isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xD8: // RET C
		{
			if (isFlagSet(FLAG_C))
			{
				_registers.pc = _memory.readWord(_registers.sp);
				_registers.sp += 2;
			}
			_registers.M = 2;
			_registers.T = 8;
		} break;
		case 0xD9: // RETI
		{
			_registers.ime = 1;

			_registers.pc = _memory.readWord(_registers.sp);
			_registers.sp += 2;

			_registers.M = 3;
			_registers.T = 12;
		} break;

		// Misc
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

			_registers.A = s;
			resetFlag(FLAG_H);
			
			if (_registers.A == 0x00) 
				setFlag(FLAG_Z);
			else
				resetFlag(FLAG_Z);
			
			if (s >= 0x100) 
				setFlag(FLAG_C);

		} break;
		case 0xF3: // DI
		{
			_registers.ime = 0;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0xFB: // EI
		{
			_registers.ime = 1;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		// Restarts
		case 0xC7: // RST 0x00
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x00;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xCF: // RST 0x08
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x08;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xD7: // RST 0x10
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x10;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xDF: // RST 0x18
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x18;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xE7: // RST 0x20
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x20;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xEF: // RST 0x28
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x28;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xF7: // RST 0x30
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x30;
			_registers.M = 8;
			_registers.T = 32;
		} break;
		case 0xFF: // RST 0x38
		{
			_registers.sp -= 2;
			_memory.writeWord(_registers.sp, _registers.pc);
			_registers.pc = 0x38;
			_registers.M = 8;
			_registers.T = 32;
		} break;						
		case 0x2F: // CPL
		{
			_registers.A = ~_registers.A;
			setFlag(FLAG_N);
			setFlag(FLAG_H);
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x3F: // SCF
		{
			setFlag(FLAG_C);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);

			_registers.M = 1;
			_registers.T = 4;
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

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x4F: // BIT 1A
				{
					if ((_registers.A & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x57: // BIT 2A
				{
					if ((_registers.A & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x5F: // BIT 3A
				{
					if ((_registers.A & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x67: // BIT 4A
				{
					if ((_registers.A & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x6F: // BIT 5A
				{
					if ((_registers.A & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x77: // BIT 6A
				{
					if ((_registers.A & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x7F: // BIT 7A
				{
					if ((_registers.A & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x40: // BIT 0B
				{
					if ((_registers.B & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x48: // BIT 1B
				{
					if ((_registers.B & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x50: // BIT 2B
				{
					if ((_registers.B & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x58: // BIT 3B
				{
					if ((_registers.B & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x60: // BIT 4B
				{
					if ((_registers.B & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x68: // BIT 5B
				{
					if ((_registers.B & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x70: // BIT 6B
				{
					if ((_registers.B & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x78: // BIT 7B
				{
					if ((_registers.B & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x41: // BIT 0C
				{
					if ((_registers.C & 0x01) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x49: // BIT 1C
				{
					if ((_registers.C & 0x02) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x51: // BIT 2C
				{
					if ((_registers.C & 0x04) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x59: // BIT 3C
				{
					if ((_registers.C & 0x08) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x61: // BIT 4C
				{
					if ((_registers.C & 0x10) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x69: // BIT 5C
				{
					if ((_registers.C & 0x20) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x71: // BIT 6C
				{
					if ((_registers.C & 0x40) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x79: // BIT 7C
				{
					if ((_registers.C & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x7C: // BIT 7H
				{
					if ((_registers.H & 0x80) != 0)
						resetFlag(FLAG_Z);
					else
						setFlag(FLAG_Z);
					resetFlag(FLAG_N);
					setFlag(FLAG_H);

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
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

					_registers.M = 3;
					_registers.T = 12;
				} break;
				case 0x87: // RES 0, A
				{
					_registers.A &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x80: // RES 0, B
				{
					_registers.B &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x81: // RES 0, C
				{
					_registers.C &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x82: // RES 0, D
				{
					_registers.D &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x83: // RES 0, E
				{
					_registers.E &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x84: // RES 0, H
				{
					_registers.H &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x85: // RES 0, L
				{
					_registers.L &= ~0x1;
					_registers.M = 2;
					_registers.T = 8;
				} break;
				case 0x86: // RES 0, (HL)
				{
					byte val = _memory.readByte((_registers.H << 8) + _registers.L);
					val &= ~0x1;
					_memory.writeByte((_registers.H << 8) + _registers.L, val);
					_registers.M = 4;
					_registers.T = 16;
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

					_registers.M = 2;
					_registers.T = 8;
				} break;
				
				// Sets
				case 0xFE: // SET 7,(HL)
				{
					byte nextByte = _memory.readByte((_registers.H << 8) + _registers.L);
					nextByte |= 0x80;
					_memory.writeByte((_registers.H << 8) + _registers.L, nextByte);
					_registers.M = 1;
					_registers.T = 4;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;

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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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

					_registers.M = 2;
					_registers.T = 8;
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
}

void Cpu::RST40()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_VBLANK;
	_registers.M = 3;
	_registers.T = 12;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST48()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_LCD;
	_registers.M = 3;
	_registers.T = 12;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST50()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_TIMER;
	_registers.M = 3;
	_registers.T = 12;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST58()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_SLINK;
	_registers.M = 3;
	_registers.T = 12;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::RST60()
{
	_registers.ime = 0;
	_registers.sp -= 2;
	_memory.writeWord(_registers.sp, _registers.pc);

	_registers.pc = INTERRUPT_HANDLER_JOYPAD;
	_registers.M = 3;
	_registers.T = 12;

	_internalM += _registers.M;
	_internalT += _registers.T;
}

void Cpu::resetCpu()
{
	_registers = {};
	_registers.ime = 1;
	_internalM = 0;
	_internalT = 0;

	_registers.pc = 0x0000;

	_opcode      = NO_OPCODE;
	_isBitOpcode = false;
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
	std::cout << "------------------------------------------" << std::endl;
}

const word* Cpu::getPC() const { return &_registers.pc; }
const timer_t* Cpu::getT() const { return &_registers.T; }

bool Cpu::isFlagSet(const byte flag) const { return (_registers.F & flag) != 0; }
byte Cpu::getFlag(const byte flag) const { return (_registers.F & flag) != 0 ? 0x1 : 0x0; }

byte Cpu::getIME() const { return _registers.ime; }

void Cpu::setFlag(const byte flag)   { _registers.F |= flag; }
void Cpu::resetFlag(const byte flag) { _registers.F &= ~flag; }