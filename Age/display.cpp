#include "display.h"

#include "cpu.h"
#include "memory.h"
#include <memory.h>
#include <iostream>

static const clock_t OAM_ACCESS_TIME  = 80;
static const clock_t VRAM_ACCESS_TIME = 172;
static const clock_t HBLANK_TIME      = 204;
static const clock_t VBLANK_TIME      = 4560;
static const clock_t FULL_FRAME_TIME  = 70224;

static const byte DISPLAY_CONTROL_FLAG_BKG    = 0x01;
static const byte DISPLAY_CONTROL_FLAG_SPR    = 0x02;
static const byte DISPLAY_CONTROL_FLAG_SSIZE  = 0x04;
static const byte DISPLAY_CONTROL_FLAG_BKGTM  = 0x08;
static const byte DISPLAY_CONTROL_FLAG_BKGTS  = 0x10;
static const byte DISPLAY_CONTROL_FLAG_WINDOW = 0x20;
static const byte DISPLAY_CONTROL_FLAG_WINTM  = 0x40;
static const byte DISPLAY_CONTROL_FLAG_DISPL  = 0x80;

Display::Display(fill_display_callback_t fillDisplayCallback)
	: _displayMode(DISPLAY_MODE_HBLANK)
	, _displayClock(0)
	, _displayLine(0)
	, _displayControlRegister(0)
	, _displayScrollX(0)
	, _displayScrollY(0)
	, _fillDisplayCallback(fillDisplayCallback)
{
	resetDisplay();
}

byte Display::readByte(const word addr)
{
	switch (addr)
	{		
		case 0xFF40: return _displayControlRegister; break;
		case 0xFF41: 
		{
			const auto b = false;
		}
		case 0xFF42: return _displayScrollY; break;
		case 0xFF43: return _displayScrollX; break;
		case 0xFF44: return _displayLine; break;
	}
	return 0;
}

void Display::writeByte(const word addr, const byte val)
{
	switch (addr)
	{
		case 0xFF40: 
		{
			if (val & DISPLAY_CONTROL_FLAG_BKG)    setControlFlag(DISPLAY_CONTROL_FLAG_BKG);
			if (val & DISPLAY_CONTROL_FLAG_BKGTM)  setControlFlag(DISPLAY_CONTROL_FLAG_BKGTM);
			if (val & DISPLAY_CONTROL_FLAG_BKGTS)  setControlFlag(DISPLAY_CONTROL_FLAG_BKGTS);
			if (val & DISPLAY_CONTROL_FLAG_DISPL)  setControlFlag(DISPLAY_CONTROL_FLAG_DISPL);
			if (val & DISPLAY_CONTROL_FLAG_SPR)    setControlFlag(DISPLAY_CONTROL_FLAG_SPR);
			if (val & DISPLAY_CONTROL_FLAG_SSIZE)  setControlFlag(DISPLAY_CONTROL_FLAG_SSIZE);
			if (val & DISPLAY_CONTROL_FLAG_WINDOW) setControlFlag(DISPLAY_CONTROL_FLAG_WINDOW);
			if (val & DISPLAY_CONTROL_FLAG_WINTM)  setControlFlag(DISPLAY_CONTROL_FLAG_WINTM);
		} break;
		case 0xFF41:
		{
			const auto b = false;
		}
		case 0xFF42: 
			_displayScrollY = val; 
			break;
		case 0xFF43: 
			_displayScrollX = val;
			break;
		case 0xFF47: 
		{
			for (size_t i = 0; i < 4; ++i)
			{
				switch ((val >> (i * 2)) & 3)
				{
					case 0: _palette[i] = { 0xF8, 0xF8, 0xF8, 0xFF }; break;
					case 1: _palette[i] = { 0xA8, 0xA8, 0xA8, 0xFF }; break;
					case 2: _palette[i] = { 0x60, 0x60, 0x60, 0xFF }; break;
					case 3: _palette[i] = { 0x00, 0x00, 0x00, 0xFF }; break;
				}
			}
		} break;
	}
}

void Display::resetDisplay()
{
	// Clear Graphics
	memset(_gfx, 0x77, sizeof(_gfx));
	memset(_tileset, 0x00, sizeof(_tileset));
	_palette[0] = { 0xF8, 0xF8, 0xF8, 0xFF };
	_palette[1] = { 0xA8, 0xA8, 0xA8, 0xFF };
	_palette[2] = { 0x60, 0x60, 0x60, 0xFF };
	_palette[3] = { 0x00, 0x00, 0x00, 0xFF };
}

void Display::setZ80TimeRegister(const clock_t* T)
{
	_z80Time = T;
}

void Display::setVramRef(byte* vram)
{
	_vramRef = vram;
}

void Display::emulateGameboyDisplay()
{
	_displayClock += *_z80Time;

	switch (_displayMode)
	{
		case DISPLAY_MODE_HBLANK:
		{
			if (_displayClock >= HBLANK_TIME)
			{
				_displayClock = 0;
				++_displayLine;

				if (_displayLine == DISPLAY_ROWS)
				{
					_displayMode = DISPLAY_MODE_VBLANK;
					_fillDisplayCallback(_gfx);
				}
				else
				{
					_displayMode = DISPLAY_MODE_OAM_READ;
				}
			}
		} break;

		case DISPLAY_MODE_VBLANK:
		{
			if (_displayClock >= HBLANK_TIME + OAM_ACCESS_TIME + VRAM_ACCESS_TIME)
			{
				_displayClock = 0;
				_displayLine++;

				if (_displayLine > 153)
				{
					_displayMode = DISPLAY_MODE_OAM_READ;
					_displayLine = 0;
				}
			}
		} break;

		case DISPLAY_MODE_OAM_READ:
		{
			if (_displayClock >= OAM_ACCESS_TIME)
			{
				_displayClock = 0;
				_displayMode = DISPLAY_MODE_VRAM_READ;
			}
		} break;

		case DISPLAY_MODE_VRAM_READ:
		{
			if (_displayClock >= VRAM_ACCESS_TIME)
			{
				_displayClock = 0;
				_displayMode = DISPLAY_MODE_HBLANK;

				renderScanline();
			}
		} break;
	}
}

void Display::updateTile(const word tile, const word x, const word y, const byte color)
{
	_tileset[tile][y][x] = color;
}

void Display::renderScanline()
{
	int mapOffset = isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTM) ? 0x1C00 : 0x1800;

	mapOffset += (((_displayLine + _displayScrollY) & 0xFF) >> 3) << 5;

	int lineOffset = _displayScrollX >> 3;

	int y = (_displayLine + _displayScrollY) & 7;
	int x = _displayScrollX & 7;

	int displayOffset = _displayLine * DISPLAY_COLS * DISPLAY_DEPTH;
	int tileIndex = _vramRef[mapOffset + lineOffset];
	
	//if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTS) && tileIndex < 128) tileIndex += 256;

	for (size_t i = 0; i < DISPLAY_COLS; ++i)
	{
		byte col = _tileset[tileIndex][y][x];
		pixel_t emulatedColor = _palette[col];

		_gfx[displayOffset]     = emulatedColor.r;
		_gfx[displayOffset + 1] = emulatedColor.g;
		_gfx[displayOffset + 2] = emulatedColor.b;
		_gfx[displayOffset + 3] = emulatedColor.a;
		displayOffset += 4;
		
		if (++x == 8)
		{
			x = 0;
			lineOffset = (lineOffset + 1) & 31;
			tileIndex  = _vramRef[mapOffset + lineOffset]; 
			//if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTS) && tileIndex < 128) tileIndex += 256;
		}
	}	

}

bool Display::isControlFlagSet(const byte flag) const
{
	return (_displayControlRegister & flag) != 0;
}

void Display::setControlFlag(const byte flag)
{
	_displayControlRegister |= flag;
}

void Display::setPixel(const byte x, const byte y, const pixel_t& pixel)
{
	_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH]     = pixel.r;
	_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 1] = pixel.g;
	_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 2] = pixel.b;
	_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 3] = pixel.a;
}

Display::pixel_t Display::getPixel(const byte x, const byte y) const
{
	return 
	{
		_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH],
		_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 1],
		_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 2],
		_gfx[y * DISPLAY_COLS + x * DISPLAY_DEPTH + 3]
	};
}