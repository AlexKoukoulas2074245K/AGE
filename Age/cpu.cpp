#include "cpu.h"
#include "memory.h"

const byte ZFLAG = 0x01;
const byte NFLAG = 0x02;
const byte HFLAG = 0x04;
const byte CFLAG = 0x08;

Cpu::Cpu(Memory& memory)
	: _memory(memory)
{
	resetCpu();
}

void Cpu::resetCpu()
{
	_registers = {};
	_flags     = 0;
	_m         = 0;
	_t         = 0;

	_registers.pc = 0x100;
}

const word* Cpu::getPC() const { return &_registers.pc; }

void Cpu::setFlag(const byte flag)   { _flags |= flag; }
void Cpu::resetFlag(const byte flag) { _flags ^= flag; }