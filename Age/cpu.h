#pragma once
#include "common.h"

class Memory;
class Cpu
{
public:
	Cpu(Memory&);

	void resetCpu();
	
	const word* getPC() const;

private:

	void resetFlag(const byte flag);
	void setFlag(const byte flag);

	struct registers
	{
		byte A, B, C, D, E, H, L, F;
		word pc, sp;
	};

	registers _registers;
	byte      _flags;
	clock_t   _m, _t;

	Memory&   _memory;
};