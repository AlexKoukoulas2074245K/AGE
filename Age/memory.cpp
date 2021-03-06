#include "memory.h"
#include "display.h"
#include "input.h"

#include <ctime>
#include <random>
#include <iostream>
#include <memory>

static const byte i_bios[256] = 
{
	0x31, 0xFE, 0xFF, 0xAF, 0x21, 0xFF, 0x9F, 0x32, 0xCB, 0x7C, 0x20, 0xFB, 0x21, 0x26, 0xFF, 0x0E,
	0x11, 0x3E, 0x80, 0x32, 0xE2, 0x0C, 0x3E, 0xF3, 0xE2, 0x32, 0x3E, 0x77, 0x77, 0x3E, 0xFC, 0xE0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1A, 0xCD, 0x95, 0x00, 0xCD, 0x96, 0x00, 0x13, 0x7B,
	0xFE, 0x34, 0x20, 0xF3, 0x11, 0xD8, 0x00, 0x06, 0x08, 0x1A, 0x13, 0x22, 0x23, 0x05, 0x20, 0xF9,
	0x3E, 0x19, 0xEA, 0x10, 0x99, 0x21, 0x2F, 0x99, 0x0E, 0x0C, 0x3D, 0x28, 0x08, 0x32, 0x0D, 0x20,
	0xF9, 0x2E, 0x0F, 0x18, 0xF3, 0x67, 0x3E, 0x64, 0x57, 0xE0, 0x42, 0x3E, 0x91, 0xE0, 0x40, 0x04,
	0x1E, 0x02, 0x0E, 0x0C, 0xF0, 0x44, 0xFE, 0x90, 0x20, 0xFA, 0x0D, 0x20, 0xF7, 0x1D, 0x20, 0xF2,
	0x0E, 0x13, 0x24, 0x7C, 0x1E, 0x83, 0xFE, 0x62, 0x28, 0x06, 0x1E, 0xC1, 0xFE, 0x64, 0x20, 0x06,
	0x7B, 0xE2, 0x0C, 0x3E, 0x87, 0xF2, 0xF0, 0x42, 0x90, 0xE0, 0x42, 0x15, 0x20, 0xD2, 0x05, 0x20,
	0x4F, 0x16, 0x20, 0x18, 0xCB, 0x4F, 0x06, 0x04, 0xC5, 0xCB, 0x11, 0x17, 0xC1, 0xCB, 0x11, 0x17,
	0x05, 0x20, 0xF5, 0x22, 0x23, 0x22, 0x23, 0xC9, 0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D, 0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E,
	0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99, 0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC,
	0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E, 0x3c, 0x42, 0xB9, 0xA5, 0xB9, 0xA5, 0x42, 0x3C,
	0x21, 0x04, 0x01, 0x11, 0xA8, 0x00, 0x1A, 0x13, 0xBE, 0x20, 0xFE, 0x23, 0x7D, 0xFE, 0x34, 0x20,
	0xF5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xFB, 0x86, 0x20, 0xFE, 0x3E, 0x01, 0xE0, 0x50
};

Memory::Memory(Display& displayRef, Input& inputRef)
	: _pcref(nullptr)
	, _displayRef(displayRef)
	, _inputRef(inputRef)
	, _ie(0)
	, _if(0)
{
	resetMemory();
	_displayRef.setMemory(this);
}

Memory::~Memory()
{
	const auto b = false;
	free(_rom);
}

byte Memory::readByte(const word addr)
{	
	switch (addr & 0xF000)
	{
		// BIOS 256 (ROM0)
		case 0x0000:
		{
			if (_inbios == 1)
			{
				if (addr < 0x0100)
					return _bios[addr];
				if (*_pcref >= 0x0100)
					_inbios = 0;				
			}

			return _rom[addr];
		} break;

		// ROM 0 (16k)
		case 0x1000:
		case 0x2000:
		case 0x3000:
		{
			return _rom[addr];
		} break;

		// ROM 1 (16k)
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000:
		{			
			return _rom[_mbcState.ROMOffset + (addr & 0x3FFF)];
		} break;

		// VRAM (8k)
		case 0x8000:
		case 0x9000:
		{
			return _vram[addr & 0x1FFF];
		} break;

		// ERAM (8k)
		case 0xA000:
		case 0xB000:
		{
			return _eram[_mbcState.RAMOffset + (addr & 0x1FFF)];
		} break;

		// WRAM (8k)
		case 0xC000:
		case 0xD000:
		{			
			return _wram[addr & 0x1FFF];
		} break;

		// WRAM (shadow)
		case 0xE000:
		{
			return _wram[addr & 0x1FFF];
		} break;

		// WRAM (Shadow), OAM, IORAM, ZRAM
		case 0xF000:
		{
			if (addr < 0xFE00)
				return _wram[addr & 0x1FFF];
			else if (addr < 0xFEFF)
				return _oam[addr & 0xFF];
			else if (addr < 0xFF80)
			{
				switch (addr & 0x00F0)
				{
					case 0x00:
					{
						if (addr == 0xFF00)
							return _inputRef.readByte(addr);
						else if (addr == 0xFF01 || addr == 0xFF02)
						{
							//std::cout << "At: 0x" << std::hex << *_pcref << " unimplemented serial transfer read " << std::hex << addr << " reading from iom instead" << std::endl;
							return _iomem[addr & 0x7F];
						}
						else if (addr == 0xFF04)
						{							
							return _iomem[addr & 0x7F];
						}
						else if (addr == 0xFF05 || addr == 0xFF06 || addr == 0xFF07)
						{							
							return _iomem[addr & 0x7F];
						}
						else if (addr == 0xFF0F)
							return _if;
					} break;
					case 0x40: 
					{
						if (addr == 0xFF46 || addr == 0xFF4a || addr == 0xFF4b)
							return _iomem[addr & 0x7F];
						else
							return _displayRef.readByte(addr); 
					} break;
					default:						
						return _iomem[addr & 0x7F];
				}
				return 0;
			}
			else if (addr < 0xFFFF)
				return _zram[addr & 0x7F];
			else
				return _ie;
				
		} break;
	}

	return 0x00;
}

word Memory::readWord(const word addr)
{
	return readByte(addr) + (readByte(addr + 1) << 8);
}

byte Memory::retrieveFromVram(const word addr)
{
	return _vram[addr];
}

void Memory::writeByte(const word addr, const byte val)
{
	if (addr == 0x9106 && val == 0x06)
		const auto b = false;

	if (_cartType != 0)
	{
		switch (addr & 0xF000)
		{
			// ERAM Switch
			case 0x0000:
			case 0x1000:
			{
				if (_cartType == 0x02 ||
					_cartType == 0x03 ||
					_cartType == 0X13)
				{
					_mbcState.RAMEnabled = val == 0x0A;
				}
			} break;
			
			// MBC1: ROM Bank
			case 0x2000:
			case 0x3000:
			{
				switch (_cartType)
				{
					case 0x1:
					case 0x2:
					case 0x3:
					{
						byte lo5 = val & 0x1F;

						if (!lo5)
							lo5 = 1;

						_mbcState.ROMBank = (_mbcState.ROMBank & 0x60) + lo5;
						word prevOffset = _mbcState.ROMOffset;
						_mbcState.ROMOffset = _mbcState.ROMBank * 0x4000;										

					} break;

					case 0x13:
					{
						byte lo7 = val & 0x7F;

						if (!lo7)
							lo7 = 1;

						_mbcState.ROMBank = lo7;

						//_mbcState.ROMBank &= 39;
						word prevOffset = _mbcState.ROMOffset;
						_mbcState.ROMOffset = _mbcState.ROMBank * 0x4000;
						
					} break;
				}
			} break;

			// MBC1: RAM Bank
			case 0x4000:
			case 0x5000:
			{
				switch (_cartType)
				{
					case 0x1:
					case 0x2:
					case 0x3:
					{
						if (_mbcState.mode)
						{
							// RAM mode: Set Bank (0-3)
							_mbcState.RAMBank = val & 0x3;
							_mbcState.RAMOffset = _mbcState.RAMBank * 0x2000;							
						}
						else
						{
							// ROM mode: Set High bits of bank
							_mbcState.ROMBank = (_mbcState.ROMBank & 0x1F) + ((val & 0x3) << 5);
							_mbcState.ROMOffset = _mbcState.ROMBank * 0x4000;							
						}
					} break;

					case 0x13:
					{
						if (val <= 0x03)
						{
							_mbcState.RAMBank = val;
							_mbcState.RAMBank &= 3;
							_mbcState.RAMOffset = _mbcState.RAMBank * 0x2000;
						}
					} break;
				} 
			} break;

			// MBC1: Mode switch
			case 0x6000:
			case 0x7000:
			{
				switch (_cartType)
				{
					case 2:
					case 3:
					{
						_mbcState.mode = (val & 0x1) == 1;
					} break;
				}
			} break;

			// External 
			case 0xA000:
			case 0xB000:
			{				
				_eram[_mbcState.RAMOffset + (addr & 0x1FFF)] = val;
			} break;

			default: normalWriteByte(addr, val);
		}
	}
	else
	{
		normalWriteByte(addr, val);
	}
}

void Memory::normalWriteByte(const word addr, const byte val)
{
	if (addr == 0xC000 && val == 0x4C)
		const auto b = false;

	switch (addr & 0xF000)
	{
		// VRAM (8k)
		case 0x8000:
		case 0x9000:
		{
			_vram[addr & 0x1FFF] = val;

			if (addr > 0x97FF) return;

			// Update tileset
			const word baseAddress = addr & 0x1FFE;
			const word tile        = (addr >> 4) & 0x1FF;
			const word y           = (addr >> 1) & 7;

			byte bitIndex = 0;

			for (byte x = 0; x < 8; ++x)
			{
				bitIndex = 1 << (7 - x);
				byte color = ((_vram[baseAddress] & bitIndex) != 0 ? 1 : 0) +
					         ((_vram[baseAddress + 1] & bitIndex) != 0 ? 2 : 0);

				_displayRef.changeTileData(tile, x, y, color);
			}
			
		} break;

		// WRAM (8k)
		case 0xC000:
		case 0xD000:
		{
			if (val == 0x1F)
			{
				const auto b = false;
			}
			_wram[addr & 0x1FFF] = val;
		} break;

		// WRAM (shadow)
		case 0xE000:
		{
			_wram[addr & 0x1FFF] = val;
		} break;

		// WRAM (Shadow), OAM, IORAM, ZRAM
		case 0xF000:
		{
			if (addr < 0xFE00)
				_wram[addr & 0x1FFF] = val;
			else if (addr < 0xFEFF)
			{
				_oam[addr & 0x9F] = val;
				_displayRef.changeSpriteData(addr - 0xFE00, val);
			}
			else if (addr < 0xFF80)
			{
				switch (addr & 0x00F0)
				{
					case 0x00:
					{						
						if (addr == 0xFF00) 
							_inputRef.writeByte(addr, val);
						else if (addr == 0xFF01 || addr == 0xFF02)
						{
							std::cout << "At: 0x" << std::hex << *_pcref << " unimplemented serial transfer write " << std::hex << addr << " dumping to iom instead" << std::endl;
							_iomem[addr & 0x7F] = val;
						}
						else if (addr == 0xFF04)
						{							
							_iomem[addr & 0x7F] = 0;
						}
						else if (addr == 0xFF05 || addr == 0xFF06 || addr == 0xFF07)
						{							
							_iomem[addr & 0x7F] = val;
						}
						else if (addr == 0xFF0F)
							_if = val; 
					} break;
					case 0x40:
					{
						if (addr == 0xFF4a || addr == 0xFF4b)
							_iomem[addr & 0x7F] = val;						
						else if (addr == 0xFF46)
							for (byte i = 0; i < 160; ++i)
							{
								byte nextByte = readByte((val << 8) + i);
								_oam[i] = nextByte;								
								_displayRef.changeSpriteData(i, nextByte);
							}
						else
							_displayRef.writeByte(addr, val);
					} break;

					default:
						//std::cout << "At: 0x" << std::hex << *_pcref << " unhandled write to: 0x" << std::hex << addr << " dumping to iom" << std::endl;
						_iomem[addr & 0x7F] = val;
				}				
			}
			else if (addr < 0xFFFF)
				_zram[addr & 0x7F] = val;
			else
			{
				_ie = val;
			}
		} break;
	}
}

void Memory::writeWord(const word addr, const word val)
{
	writeByte(addr,     val & 0x00FF);
	writeByte(addr + 1, val >> 8);
}

byte Memory::getCurrentRomBank() const
{
	return _mbcState.ROMBank;
}

void Memory::incrementDiv()
{
	_iomem[0x04]++;
}

bool Memory::inBios() const { return _inbios != 0; }
byte Memory::getIE() const { return _ie; }
byte Memory::getIF() const { return _if; }
byte* Memory::getIEPtr() { return &_ie; }
byte* Memory::getIFPtr() { return &_if; }

const std::string& Memory::getCartName() const
{
	return _cartName;
}

void Memory::resetInterrupt(const byte interrupt) { _if &= ~interrupt; }

void Memory::fillRom(const std::vector<char>& romData)
{
	_rom = (byte*)malloc(sizeof(byte) * romData.size());
	memcpy(_rom, &romData[0], romData.size());
	_cartName.clear();
	for (int i = 0x0134; _rom[i] != '\0'; i++)
	{
		_cartName += _rom[i];
	}

	_cartType = _rom[0x0147];
	initMBC();
}

void Memory::setPcRef(const word* pcref) { _pcref = pcref; }

void Memory::resetMemory()
{
	free(_rom);
	_inbios = 1;
	
	memcpy(_bios, i_bios, sizeof(_bios));	
	memset(_vram,  0, sizeof(_vram));	
	memset(_wram,  0, sizeof(_wram));
	memset(_oam,   0, sizeof(_oam));
	memset(_eram,  0, sizeof(_eram));
	memset(_iomem, 0, sizeof(_iomem));
	memset(_zram,  0, sizeof(_zram));

	_ie = 0;
	_if = 0;

	std::srand((unsigned int)std::time(NULL));
}

void Memory::initMBC()
{
	switch (_cartType)
	{
		case 0x0:
		case 0x1: 
		{
			_mbcState.RAMBank    = 0;
			_mbcState.ROMBank    = 0;
			_mbcState.RAMEnabled = false;
			_mbcState.mode       = false;
			_mbcState.ROMOffset  = 0x4000;
			_mbcState.RAMOffset  = 0x0000;
		} break;
	}
}