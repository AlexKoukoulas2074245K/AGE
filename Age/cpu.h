#pragma once
#include "common.h"

class Memory;
class Cpu final
{
public:
	Cpu(Memory&);

	void emulateCycle();

	void resetCpu();
	void printRegisters();

	const word* getPC() const;
	const clock_t* getT() const;

	bool isFlagSet(const byte flag) const;
	byte getFlag(const byte flag) const;

public:

	static const byte FLAG_Z = 0x01;
	static const byte FLAG_N = 0x02;
	static const byte FLAG_H = 0x04;
	static const byte FLAG_C = 0x08;

private:

	void resetFlag(const byte flag);
	void setFlag(const byte flag);

private:
	enum error_state
	{
		ES_OK,
		ES_UNIMPLEMENTED_INSTRUCTION
	};

	struct registers
	{
		byte A, B, C, D, E, H, L, F;
		word pc, sp;
		clock_t M, T;
	};

private:
	registers   _registers;
	byte        _flags;
	clock_t     _internalM, _internalT;
	byte        _opcode;
	byte        _isBitOpcode;
	error_state _errorState;
	Memory&	    _memory;
};