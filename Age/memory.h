#pragma once
#include "common.h"

#include <vector>
#include <functional>

class Input;
class Display;
class Memory final
{
public:
	Memory(Display&, Input&);

	byte readByte(const word addr);
	word readWord(const word addr);

	byte getIE() const;
	byte getIF() const;
	byte* getIFPtr();
	void resetInterrupt(const byte interrupt);

	void writeByte(const word addr, const byte val);
	void writeWord(const word addr, const word val);

	void fillRom(const std::vector<char>& romData);

	void setPcRef(const word* pcref);

	void resetMemory();

public:
	static const byte INTERRUPT_FLAG_VBLANK    = 0x01;
	static const byte INTERRUPT_FLAG_TOGGLELCD = 0x02;
	static const byte INTERRUPT_FLAG_TIMER     = 0x04;
	static const byte INTERRUPT_FLAG_SERIAL    = 0x08;
	static const byte INTERRUPT_FLAG_JOYPAD    = 0x10;

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
	byte _ie;
	byte _if;

	const word* _pcref;
	Display& _displayRef;
	Input& _inputRef;
};