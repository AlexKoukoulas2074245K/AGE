#pragma once
#include "common.h"

class Memory;
class Cpu final
{
public:
	Cpu(Memory&);

	void emulateCycle();
	void handleInterrupts();

	void resetCpu();
	void printRegisters();

	const word* getPC() const;
	const timer_t* getT() const;

	bool isFlagSet(const byte flag) const;
	byte getFlag(const byte flag) const;
	byte getLastExecutedOpcode() const;
	byte getIME() const;

	void RST40();
	void RST48();
	void RST50();
	void RST58();
	void RST60();

public:

	static const byte FLAG_Z = 0x80;
	static const byte FLAG_N = 0x40;
	static const byte FLAG_H = 0x20;
	static const byte FLAG_C = 0x10;

private:

	void resetFlag(const byte flag);
	void setFlag(const byte flag);
	void incrementTimer();
	void stepTimer();

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
		timer_t M, T;
		byte ime;
	};

private:
	registers   _registers;
	bool        _halted;
	timer_t     _internalM, _internalT;
	byte        _clockSub;
	byte        _clockDiv;
	byte        _clockMain;
	byte        _opcode;
	byte        _isBitOpcode;
	error_state _errorState;
	Memory&	    _memory;
};