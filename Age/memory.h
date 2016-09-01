#pragma once
#include "common.h"

#include <vector>

class Memory
{
public:
	Memory();

	byte readByte(const word addr);
	word readWord(const word addr);

	void writeByte(const word addr, const byte val);
	void writeWord(const word addr, const word val);

	void fillRom(const std::vector<char>& romData);

	void setPcRef(const word* pcref);

	void resetMemory();

private:
	byte _inbios;
	byte _bios[256];
	byte _rom[32768];
	byte _vram[8192];
	byte _eram[8192];
	byte _wram[8192];
	byte _oam[160];
	byte _iomem[128];
	byte _zram[128];

	const word* _pcref;
};