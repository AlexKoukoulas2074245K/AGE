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
	~Memory();

	byte readByte(const word addr);
	byte retrieveFromVram(const word addr);
	word readWord(const word addr);

	void normalWriteByte(const word addr, const byte val);
	void writeByte(const word addr, const byte val);
	void writeWord(const word addr, const word val);
	void incrementDiv();

	bool inBios() const;
	byte getIE() const;
	byte getIF() const;
	byte* getIEPtr();
	byte* getIFPtr();

	void resetInterrupt(const byte interrupt);

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

	struct mbc_state_t
	{
		byte ROMBank;
		byte RAMBank;
		bool RAMEnabled;
		bool mode;

		word RAMOffset;
		word ROMOffset;
	};

private:

	void initMBC();

private:
	byte _inbios;
	byte _bios[256];
	byte _rom[65536];
	byte _vram[8192];
	byte _eram[8192];
	byte _wram[8192];
	byte _oam[160];
	byte _iomem[128];
	byte _zram[128];	
	byte _ie;
	byte _if;
	byte _cartType;

	mbc_state_t _mbcState;
	const word* _pcref;
	Display& _displayRef;
	Input& _inputRef;
};