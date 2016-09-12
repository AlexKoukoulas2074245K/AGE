#pragma once

#include "common.h"

class Input
{
public:
	Input();

	void resetInput();

	void setIFRef(byte* intFlag);

	void keyDown(const int key);
	void keyUp(const int key);

	byte readByte(const word addr);
	void writeByte(const word addr, const byte val);

private:

	byte  _keys[2];
	byte  _column;
	byte* _intFlag;
};
