#include "cpu.h"
#include "memory.h"

#include <iostream>
#include <unordered_map>
#include <string>

static const byte NO_OPCODE = 0xDD;

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
	{ 0x0A, "LD A, (BC)" },
	{ 0x1A, "LD A, (DE)" },
	{ 0x3E, "LD A, #" },
	{ 0x7F, "LD A, A" },
	{ 0x47, "LD B, A" },
	{ 0x4F, "LD C, A" },
	{ 0x57, "LD D, A" },
	{ 0x5F, "LD E, A" },
	{ 0x66, "LD H, (HL)" },
	{ 0x67, "LD H, A" },
	{ 0x6F, "LD L, A" },
	{ 0x77, "LD (HL), A" },
	{ 0xE0, "LDH (n), A" },
	{ 0xE2, "LD (C), A" },
	{ 0xF0, "LDH A, (n)"},
	{ 0x22, "LDI (HL), A" },
	{ 0x2A, "LDI A, (HL)" },
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
	{ 0xAF, "XOR A, A" },
	{ 0xA8, "XOR A, B" },
	{ 0xA9, "XOR A, C" },
	{ 0xAA, "XOR A, D" },
	{ 0xAB, "XOR A, E" },
	{ 0xAC, "XOR A, H" },
	{ 0xAD, "XOR A, L" },
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

	// Rotates & Shifts
	{ 0x0F, "RRCA" },
	{ 0x17, "RL A" },

	// Calls
	{ 0xCD, "CALL nn" },
	{ 0xC4, "CALL NZ, nn" },
	{ 0xCC, "CALL Z, nn" },
	{ 0xD4, "CALL NC, nn" },
	{ 0xDC, "CALL C, nn" },

	// Jumps
	{ 0xC3, "JP nn(16 bits)" },
	{ 0x18, "JR n" },
	{ 0x20, "JR NZ,n" },
	{ 0x28, "JR Z,n" },
	{ 0x30, "JR NC,n" },
	{ 0x38, "JR C,n" },


	// Returns
	{ 0xC9, "RET" },

	// Misc
	{ 0xF3, "DI" },
	{ 0xFB, "EI" },
	{ 0xFF, "RST 0x38" },
	{ 0x2F, "CPL" },
	{ 0x3F, "SCF" }
};

static const std::unordered_map<byte, std::string> s_bitOpcodeDisassembly =
{
	{ 0x7C, "BIT 7H"},
	{ 0x4F, "BIT 1A" },
	{ 0x11, "RL C" },
	
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
		case 0x47: // LD B, A
		{
			_registers.B = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x4F: // LD C, A
		{
			_registers.C = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x57: // LD D, A
		{
			_registers.D = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x5F: // LD E, A
		{
			_registers.E = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
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
		case 0x6F: // LD L, A
		{
			_registers.L = _registers.A;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0x77: // LD (HL), A
		{
			_memory.writeByte(((_registers.H << 8) + _registers.L), _registers.A);
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
		case 0x73: // LD (HL), E
		{
			_memory.writeByte((_registers.H << 8) + _registers.L, _registers.E);
			_registers.M = 2;
			_registers.T = 8;
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
		case 0x3D: // DEC A
		{
			if (_registers.A != 0x10)
				setFlag(FLAG_H);

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
			std::cout << "Not implemented" << std::endl;
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
			if ((_registers.A & 0x0F) >= (_registers.A & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);
			
			if (_registers.A >= _registers.A)
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
			if ((_registers.A & 0x0F) >= (_registers.B & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.B)
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
			if ((_registers.A & 0x0F) >= (_registers.C & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.C)
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
			if ((_registers.A & 0x0F) >= (_registers.D & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.D)
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
			if ((_registers.A & 0x0F) >= (_registers.E & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.E)
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
			if ((_registers.A & 0x0F) >= (_registers.H & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.H)
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
			if ((_registers.A & 0x0F) >= (_registers.L & 0x0F))
				setFlag(FLAG_H);
			else
				resetFlag(FLAG_H);

			if (_registers.A >= _registers.L)
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

			if ((_registers.A & 0x0F) > (_registers.A & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.B & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.C & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.D & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.E & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.H & 0x0F))
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

			if ((_registers.A & 0x0F) > (_registers.L & 0x0F))
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

			if ((_registers.A & 0x0F) > (val & 0x0F))
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

			if ((_registers.A & 0x0F) > (val & 0x0F))
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
		case 0xAF: // XOR A, A
		{
			_registers.A ^= _registers.A; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.B; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.C; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.D; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.E; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.H; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
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
			_registers.A ^= _registers.L; // (A xor A = 0)
			if (_registers.A == 0x00)
				setFlag(FLAG_Z); // we know A xor A is 0
			else
				resetFlag(FLAG_Z);
			resetFlag(FLAG_N);
			resetFlag(FLAG_H);
			resetFlag(FLAG_C);
			_registers.M = 1;
			_registers.T = 4;
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

		// Misc
		case 0xF3: // DI
		{
			std::cout << std::hex << _registers.pc << " Unimplemented DI" << std::endl;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		case 0xFB: // EI
		{
			std::cout << std::hex << _registers.pc << " Unimplemented EI" << std::endl;
			_registers.M = 1;
			_registers.T = 4;
		} break;
		// Restarts
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

				case 0x4F: // BIT 1A
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
					std::cout << ">>> Unimplemented instruction: 0x" << std::hex << static_cast<int>(_opcode) << " <<<" << std::endl;
					_errorState = ES_UNIMPLEMENTED_INSTRUCTION;
				} 
			}
		} break;

		default:
		{
			std::cout << ">>> Unimplemented instruction: 0x" << std::hex << static_cast<int>(_opcode) << " <<<"<< std::endl;		
			_errorState = ES_UNIMPLEMENTED_INSTRUCTION;
		}
	}

	_internalM += _registers.M;
	_internalT += _registers.T;	
}

void Cpu::resetCpu()
{
	_registers = {};
	_flags     = 0;
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
	std::cout << "------------------------------------------" << std::endl;
}

const word* Cpu::getPC() const { return &_registers.pc; }
const clock_t* Cpu::getT() const { return &_registers.T; }

bool Cpu::isFlagSet(const byte flag) const { return (_flags & flag) != 0; }
byte Cpu::getFlag(const byte flag) const { return (_flags & flag) != 0 ? 0x1 : 0x0; }

void Cpu::setFlag(const byte flag)   { _flags |= flag; }
void Cpu::resetFlag(const byte flag) { _flags &= ~flag; }