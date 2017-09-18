#include "display.h"

#include "cpu.h"
#include "memory.h"
#include "window.h"

#include <memory.h>
#include <iostream>
#include <unordered_set>
#include <unordered_map>

#define PALETTE_SHIFT_ENABLED
//#define SHOW_SELECTED_TILES

static const timer_t OAM_ACCESS_TIME  = 80;
static const timer_t VRAM_ACCESS_TIME = 172;
static const timer_t HBLANK_TIME      = 204;
static const timer_t VBLANK_TIME      = 4560;
static const timer_t FULL_FRAME_TIME  = 70224;

static const byte DISPLAY_CONTROL_FLAG_BKG    = 0x01;
static const byte DISPLAY_CONTROL_FLAG_SPR    = 0x02;
static const byte DISPLAY_CONTROL_FLAG_SSIZE  = 0x04;
static const byte DISPLAY_CONTROL_FLAG_BKGTM  = 0x08;
static const byte DISPLAY_CONTROL_FLAG_TS  = 0x10;
static const byte DISPLAY_CONTROL_FLAG_WINDOW = 0x20;
static const byte DISPLAY_CONTROL_FLAG_WINTM  = 0x40;
static const byte DISPLAY_CONTROL_FLAG_DISPL  = 0x80;

static const byte SPRITE_FLAG_PALETTE = 0x10;
static const byte SPRITE_FLAG_X_FLIP  = 0x20;
static const byte SPRITE_FLAG_Y_FLIP  = 0x40;
static const byte SPRITE_FLAG_PRIO    = 0x80;

static const dword COLOR_0 = 0x00D0F8E0;
static const dword COLOR_1 = 0xFF70C088;
static const dword COLOR_2 = 0xFF566834;
static const dword COLOR_3 = 0xFF201808;

static const std::unordered_map<dword, byte> s_colorToVal
{
	{COLOR_0, 0},
	{COLOR_1, 1},
	{COLOR_2, 2},
	{COLOR_3, 3}
};

Display::Display(fill_displays_callback_t fillDisplayCallback)
	: _fillDisplayCallback(fillDisplayCallback)
{
	resetDisplay();
}

byte Display::readByte(const word addr)
{
	switch (addr)
	{		
		case 0xFF40: return _displayControlRegister; break;
		case 0xFF41: return _statRegister; break;
		case 0xFF42: return _displayScrollY; break;
		case 0xFF43: return _displayScrollX; break;
		case 0xFF44: return _displayLine; break;
		case 0xFF45: return _displayLYC; break;	
		case 0xFF47: return s_colorToVal.at(_bkgPalette[0]) |
			                s_colorToVal.at(_bkgPalette[1]) << 8 |
							s_colorToVal.at(_bkgPalette[2]) << 16 |
							s_colorToVal.at(_bkgPalette[3]) << 24; break;
		case 0xFF48: return s_colorToVal.at(_spr0Palette[0]) |
			                s_colorToVal.at(_spr0Palette[1]) << 8 |
							s_colorToVal.at(_spr0Palette[2]) << 16 |
							s_colorToVal.at(_spr0Palette[3]) << 24; break;
		case 0xFF49: return s_colorToVal.at(_spr1Palette[0]) |
			                s_colorToVal.at(_spr1Palette[1]) << 8 |
							s_colorToVal.at(_spr1Palette[2]) << 16 |
							s_colorToVal.at(_spr1Palette[3]) << 24; break;
		case 0xFF4A: return _displayWindowY; break;
		case 0xFF4B: return _displayWindowX; break;

		default:
			std::cout << " unimplemented display read at 0x" << std::hex << addr << std::endl;
	}
	return 0;
}

void Display::writeByte(const word addr, const byte val)
{
	bool prevWindow = (_displayControlRegister & DISPLAY_CONTROL_FLAG_WINDOW) != 0;

	switch (addr)
	{
		case 0xFF40: _displayControlRegister = val; break;
		case 0xFF41: _statRegister = val; break;
		case 0xFF42: _displayScrollY = val; break;
		case 0xFF43: _displayScrollX = val; break;
		case 0xFF45: _displayLYC = val; break;
		case 0xFF47: 
		{
#ifdef PALETTE_SHIFT_ENABLED
			for (size_t i = 0; i < 4; ++i)
			{
				switch ((val >> (i * 2)) & 3)
				{
					case 0: _bkgPalette[i] = COLOR_0; break;
					case 1: _bkgPalette[i] = COLOR_1; break;
					case 2: _bkgPalette[i] = COLOR_2; break;
					case 3: _bkgPalette[i] = COLOR_3; break;
				}
			}
#endif
		} break;

		case 0xFF48:
		{
#ifdef PALETTE_SHIFT_ENABLED
			for (size_t i = 0; i < 4; ++i)
			{
				switch ((val >> (i * 2)) & 3)
				{
					case 0: _spr0Palette[i] = COLOR_0; break;
					case 1: _spr0Palette[i] = COLOR_1; break;
					case 2: _spr0Palette[i] = COLOR_2; break;
					case 3: _spr0Palette[i] = COLOR_3; break;
				}
			}
#endif
		} break;

		case 0xFF49:
		{
#ifdef PALETTE_SHIFT_ENABLED
			for (size_t i = 0; i < 4; ++i)
			{
				switch ((val >> (i * 2)) & 3)
				{
					case 0: _spr1Palette[i] = COLOR_0; break;
					case 1: _spr1Palette[i] = COLOR_1; break;
					case 2: _spr1Palette[i] = COLOR_2; break;
					case 3: _spr1Palette[i] = COLOR_3; break;
				}
			}
#endif
		} break;
		case 0xFF4A: _displayWindowX = val; break;
		case 0xFF4B: _displayWindowY = val; break;

		default:
			std::cout << " unimplemented display write at 0x" << std::hex << addr << std::endl;
	}

	if (((_displayControlRegister & DISPLAY_CONTROL_FLAG_WINDOW) != 0) != prevWindow)
	{
		if (prevWindow)
			std::cout << "Window turned off" << std::endl;
		else
			std::cout << "Window turned on" << std::endl;
	}
}

void Display::resetDisplay()
{
	// Clear Graphics
	memset(_gfx, 0x77, sizeof(_gfx));
	memset(_tileset, 0x00, sizeof(_tileset));
	memset(_tileGfx, 0x00, sizeof(_tileGfx));
	memset(_spriteGfx, 0x00, sizeof(_spriteGfx));

	_bkgPalette[0] = COLOR_0;
	_bkgPalette[1] = COLOR_1;
	_bkgPalette[2] = COLOR_2;
	_bkgPalette[3] = COLOR_3;

	_spr0Palette[0] = COLOR_0;
	_spr0Palette[1] = COLOR_1;
	_spr0Palette[2] = COLOR_2;
	_spr0Palette[3] = COLOR_3;

	_spr1Palette[0] = COLOR_0;
	_spr1Palette[1] = COLOR_1;
	_spr1Palette[2] = COLOR_2;
	_spr1Palette[3] = COLOR_3;
	
	_displayClock           = 0;
	_displayLine            = 0;
	_displayControlRegister = 0;
	_statRegister           = 0;
	_displayScrollX         = 0;
	_displayScrollY         = 0;
	_displayLYC             = 0;
	_displayWindowX         = 0;
	_displayWindowY         = 0;

	for (byte i = 0; i < 40; ++i)
	{
		_spriteData[i].y     = 0;
		_spriteData[i].x     = 0;
		_spriteData[i].tile  = 0;
		_spriteData[i].flags = 0;
	}
}

void Display::setZ80TimeRegister(const timer_t* T)
{
	_z80Time = T;
}

void Display::setMemory(Memory* const memory)
{
	_memory = memory;
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
				_displayClock -= HBLANK_TIME;
				++_displayLine;
				
				if (_displayLine == DISPLAY_ROWS)
				{
					_displayMode = DISPLAY_MODE_VBLANK;
					fillTileViewGfx();
					fillSpriteViewGfx();
					_fillDisplayCallback(_gfx, _tileGfx, _spriteGfx);

					if (_statRegister & 0x10)
						*(_memory->getIFPtr()) |= Memory::INTERRUPT_FLAG_TOGGLELCD;
						
					*(_memory->getIFPtr()) |= Memory::INTERRUPT_FLAG_VBLANK;
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
				_displayClock -= HBLANK_TIME + OAM_ACCESS_TIME + VRAM_ACCESS_TIME;
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
				_displayClock -= OAM_ACCESS_TIME;
				_displayMode = DISPLAY_MODE_VRAM_READ;
			}
		} break;

		case DISPLAY_MODE_VRAM_READ:
		{
			if (_displayClock >= VRAM_ACCESS_TIME)
			{
				_displayClock -= VRAM_ACCESS_TIME;
				_displayMode = DISPLAY_MODE_HBLANK;

				renderScanline();
			}
		} break;
	} 
}

void Display::changeSpriteData(const word addr, const byte val)
{
	byte spriteIndex = addr >> 2;

	if (spriteIndex >= 40)
		return;

	switch (addr & 3)
	{
		// Y-coord
		case 0: _spriteData[spriteIndex].y = val - 16; break;

		// X-coord
		case 1: _spriteData[spriteIndex].x = val - 8; break;
		// Tile num
		case 2: _spriteData[spriteIndex].tile = val; break;

		// Flags
		case 3: _spriteData[spriteIndex].flags = val; break;
	}
}

void Display::changeTileData(const word tile, const word x, const word y, const byte color)
{
	_tileset[tile][y][x] = color;
}

void Display::printSpriteData(const int mouseX, const int mouseY)
{
	int col = (mouseX >> 3) / 8;
	int row = (mouseY >> 3) / 8;
	int tileIndex = row * 8 + col;

	std::cout << std::dec << _spriteData[tileIndex].x << ", " << _spriteData[tileIndex].y << " flags: " << (int)_spriteData[tileIndex].flags << std::endl;
}

void Display::renderScanline()
{	
	/*
	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_WINDOW))
	{
		if (_displayWindowX != 0 || _displayWindowY != 0)
			std::cout << std::dec << _displayWindowX << ", " << std::dec << _displayWindowY << std::endl;
		//std::cout << std::dec << (int)_displayWindowX << ", " << std::dec << (int)_displayWindowY << std::endl;
		int mapOffset = isControlFlagSet(DISPLAY_CONTROL_FLAG_WINTM) ? 0x1C00 : 0x1800;

		mapOffset += (((_displayLine + 0) & 0xFF) >> 3) << 5;

		int lineOffset = 0 >> 3;

		int y = (_displayLine + 0) & 7;
		int x = 0 & 7;

		int displayOffset = _displayLine * DISPLAY_COLS * DISPLAY_DEPTH;
		int tileIndex = _memory->retrieveFromVram(mapOffset + lineOffset);

		if (!isControlFlagSet(DISPLAY_CONTROL_FLAG_TS) && tileIndex < 128)
			tileIndex += 256;

		for (size_t i = 0; i < DISPLAY_COLS; ++i)
		{
			byte col = (_tileset[tileIndex][y][x] & 0x3);

			dword emucol = _bkgPalette[col];
			
			_gfx[displayOffset] = (emucol & 0x000000FF) >> 0;
			_gfx[displayOffset + 1] = (emucol & 0x0000FF00) >> 8;
			_gfx[displayOffset + 2] = (emucol & 0x00FF0000) >> 16;
			_gfx[displayOffset + 3] = (emucol & 0xFF000000) >> 24;
			displayOffset += 4;

			if (++x == 8)
			{
				x = 0;
				lineOffset = (lineOffset + 1) & 31;
				tileIndex = _memory->retrieveFromVram(mapOffset + lineOffset);
				if (!isControlFlagSet(DISPLAY_CONTROL_FLAG_TS) && tileIndex < 128)
					tileIndex += 256;
			}
		}
	}

	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKG))
	{
		int mapOffset = isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTM) ? 0x1C00 : 0x1800;

		mapOffset += (((_displayLine + _displayScrollY) & 0xFF) >> 3) << 5;

		int lineOffset = _displayScrollX >> 3;

		int y = (_displayLine + _displayScrollY) & 7;
		int x = _displayScrollX & 7;

		int displayOffset = _displayLine * DISPLAY_COLS * DISPLAY_DEPTH;
		int tileIndex = _memory->retrieveFromVram(mapOffset + lineOffset);

		if (!isControlFlagSet(DISPLAY_CONTROL_FLAG_TS) && tileIndex < 128)
			tileIndex += 256;

		for (size_t i = 0; i < DISPLAY_COLS; ++i)
		{
			byte col = (_tileset[tileIndex][y][x] & 0x3);

			dword emucol = _bkgPalette[col];

			if (emucol != COLOR_0)
			{
				_gfx[displayOffset] = (emucol & 0x000000FF) >> 0;
				_gfx[displayOffset + 1] = (emucol & 0x0000FF00) >> 8;
				_gfx[displayOffset + 2] = (emucol & 0x00FF0000) >> 16;
				_gfx[displayOffset + 3] = (emucol & 0xFF000000) >> 24;			
			}

			displayOffset += 4;

			if (++x == 8)
			{
				x = 0;
				lineOffset = (lineOffset + 1) & 31;
				tileIndex = _memory->retrieveFromVram(mapOffset + lineOffset);
				if (!isControlFlagSet(DISPLAY_CONTROL_FLAG_TS) && tileIndex < 128)
					tileIndex += 256;
			}
		}
	}
	*/

	word tileData = 0;
	word backgroundMemory = 0;
	bool unsign= true;

	const byte scrollX = _displayScrollX;
	const byte scrollY = _displayScrollY;
	const byte windowY = _displayWindowY;
	const byte windowX = _displayWindowX - 7;

	bool usingWindow = false;

	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_WINDOW))
	{
		usingWindow = windowY <= _displayLine;
	}

	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_TS))
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
		unsign = false;
	}

	if (usingWindow == false)
	{
		if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTM))
			backgroundMemory = 0x9C00;
		else
			backgroundMemory = 0x9800;
	}
	else
	{
		if (isControlFlagSet(DISPLAY_CONTROL_FLAG_WINTM))
			backgroundMemory = 0x9C00;
		else
			backgroundMemory = 0x9800;
	}

	byte yPos = 0;

	if (!usingWindow)
		yPos = scrollY + _displayLine;
	else
		yPos = _displayLine - windowY;

	// Which of the 8 vertical pixels of the current tile is the scanline on?
	word tileRow = (((byte)(yPos / 8)) * 32);

	for (int pixel = 0; pixel < 160; pixel++)
	{
		byte xPos = pixel + scrollX;

		if (usingWindow)
			if (pixel >= windowX)
				xPos = pixel - windowX;

		// Which of the 32 horizontal tiles does this xPos fall within
		word tileCol = xPos / 8;
		signed short tileNum = 0;

		word tileAddress = backgroundMemory + tileRow + tileCol;

		if (unsign)
			tileNum = (byte) _memory->readByte(tileAddress);
		else
			tileNum = (signed char) _memory->readByte(tileAddress);

		word tileLocation = tileData;

		if (unsign)
			tileLocation += (tileNum * 16);
		else
			tileLocation += ((tileNum + 128) * 16);

		byte line = yPos % 8;
		line *= 2;
		byte data1 = _memory->readByte(tileLocation + line);
		byte data2 = _memory->readByte(tileLocation + line + 1);

		int colorBit = xPos % 8;
		colorBit -= 7;
		colorBit *= -1;

		int colorNum = 0;
		if ((data2 & (1 << colorBit)) != 0x00)
			colorNum += 2;
		if ((data1 & (1 << colorBit)) != 0x00)
			colorNum += 1;

		dword emucol = _bkgPalette[colorNum];

		int arrayIndex = (_displayLine * 160 * DISPLAY_DEPTH) + pixel * DISPLAY_DEPTH;

		_gfx[arrayIndex] = (emucol & 0x000000FF) >> 0;
		_gfx[arrayIndex + 1] = (emucol & 0x0000FF00) >> 8;
		_gfx[arrayIndex + 2] = (emucol & 0x00FF0000) >> 16;
		_gfx[arrayIndex + 3] = (emucol & 0xFF000000) >> 24;
	}

	/* ------------------ */
	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_SPR))
	{
		for (byte i = 0; i < 40; ++i)
		{
			sprite_data sd = _spriteData[i];
			int sx = sd.x;
			int sy = sd.y;
			byte spriteHeight = isControlFlagSet(DISPLAY_CONTROL_FLAG_SSIZE) ? 16 : 8;

			// Check if the sprite falls on this scanline
			if (sy <= _displayLine && (sy + spriteHeight) > _displayLine)
			{
				dword* spritePalette = isSpriteFlagSet(i, SPRITE_FLAG_PALETTE) ? _spr1Palette : _spr0Palette;
				spritePalette = _bkgPalette;
				int displayOffset = (_displayLine * 160 + sx) * 4;

				byte* tileRow;

				if (isSpriteFlagSet(i, SPRITE_FLAG_Y_FLIP))
					tileRow = _tileset[sd.tile][7 - (_displayLine - sy)];
				else
					tileRow = _tileset[sd.tile][_displayLine - sy];

				for (size_t x = 0; x < 8; ++x)
				{
					// If pixel is still on screen then render it
					if ((sx + x) >= 0 && (sx + x) < 160)
					{
						// If X flip is set reverse pixel write						
						dword emucol = 0;
						if (!isSpriteFlagSet(i, SPRITE_FLAG_X_FLIP))
							emucol = spritePalette[tileRow[x]];
						else
							emucol = spritePalette[tileRow[7 - x]];

						// If transparent pixel ignore
						if (emucol == COLOR_0)
						{
							displayOffset += 4;
							continue;
						}

						if (isSpriteFlagSet(i, SPRITE_FLAG_PRIO))
						{
							dword prevCol = _gfx[displayOffset]           |
								            _gfx[displayOffset + 1] << 8  |
										    _gfx[displayOffset + 2] << 16 |
											_gfx[displayOffset + 3] << 24;

							if (prevCol != COLOR_0)
							{
								displayOffset += 4;
								continue;
							}
						}

						_gfx[displayOffset] = (emucol & 0x000000FF) >> 0;
						_gfx[displayOffset + 1] = (emucol & 0x0000FF00) >> 8;
						_gfx[displayOffset + 2] = (emucol & 0x00FF0000) >> 16;
						_gfx[displayOffset + 3] = (emucol & 0xFF000000) >> 24;

						displayOffset += 4;
					}
				}
			}

			if (isControlFlagSet(DISPLAY_CONTROL_FLAG_SSIZE) && 
				(sy <= _displayLine && (sy + spriteHeight) > _displayLine))
			{
				if (_displayLine + 8 >= DISPLAY_ROWS || sd.tile + 1 > DISPLAY_TILES - 1)
					continue;

				dword* spritePalette = isSpriteFlagSet(i, SPRITE_FLAG_PALETTE) ? _spr1Palette : _spr0Palette;
				spritePalette = _bkgPalette;
				int displayOffset = ((_displayLine + 8) * 160 + sx) * 4;

				byte* tileRow;

				if (isSpriteFlagSet(i, SPRITE_FLAG_Y_FLIP))
					tileRow = _tileset[sd.tile + 1][7 - (_displayLine - sy)];
				else
					tileRow = _tileset[sd.tile + 1][_displayLine - sy];

				for (size_t x = 0; x < 8; ++x)
				{
					// If X flip is set reverse pixel write	
					if ((sx + x) >= 0 && (sx + x) < 160)
					{
						// If transparent pixel ignore
						dword emucol = 0;
						if (!isSpriteFlagSet(i, SPRITE_FLAG_X_FLIP))
							emucol = spritePalette[tileRow[x]];
						else
							emucol = spritePalette[tileRow[7 - x]];						

						if (emucol == COLOR_0)
						{
							displayOffset += 4;
							continue;
						}

						if (isSpriteFlagSet(i, SPRITE_FLAG_PRIO))
						{
							dword prevCol = _gfx[displayOffset] |
								_gfx[displayOffset + 1] << 8 |
								_gfx[displayOffset + 2] << 16 |
								_gfx[displayOffset + 3] << 24;

							if (prevCol != COLOR_0)
							{
								displayOffset += 4;
								continue;
							}
						}

						_gfx[displayOffset] = (emucol & 0x000000FF) >> 0;
						_gfx[displayOffset + 1] = (emucol & 0x0000FF00) >> 8;
						_gfx[displayOffset + 2] = (emucol & 0x00FF0000) >> 16;
						_gfx[displayOffset + 3] = (emucol & 0xFF000000) >> 24;

						displayOffset += 4;
					}
				}
			}
		}
	}
}

void Display::fillTileViewGfx()
{
	std::unordered_set<int> selectedTiles;

#ifdef SHOW_SELECTED_TILES
	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTM))
	{
		for (int i = 0x9C00; i <= 0x9FFF; ++i)
		{
			selectedTiles.insert(_memory->retrieveFromVram(i - 0x8000));
		}
	}
	else
	{
		for (int i = 0x9800; i <= 0x9BFF; ++i)
		{
			selectedTiles.insert(_memory->retrieveFromVram(i - 0x8000));
		}
	}
#endif

	for (int y = 0; y < DISPLAY_TILE_VIEW_BASE_HEIGHT; ++y)
	{
		for (int x = 0; x < DISPLAY_TILE_VIEW_BASE_WIDTH; ++x)
		{
			int tileIndex = (y / DISPLAY_TILE_ROWS) * DISPLAY_TILE_VIEW_TILES_PER_ROW + x / DISPLAY_TILE_COLS;

			int arrayIndex = (y * DISPLAY_TILE_VIEW_BASE_WIDTH * DISPLAY_DEPTH) + x * DISPLAY_DEPTH;
			byte color = (_tileset[tileIndex][y][x] & 0x03);
			dword emucol = _bkgPalette[_tileset[tileIndex][y % DISPLAY_TILE_ROWS][x % DISPLAY_TILE_COLS]];
	
#ifdef SHOW_SELECTED_TILES
			if (emucol == COLOR_0 && selectedTiles.count(tileIndex))
			{
				emucol = 0xFF00FF00;
			}
#endif

			_tileGfx[arrayIndex]     = (emucol & 0x000000FF) >> 0;
			_tileGfx[arrayIndex + 1] = (emucol & 0x0000FF00) >> 8;
			_tileGfx[arrayIndex + 2] = (emucol & 0x00FF0000) >> 16;
			_tileGfx[arrayIndex + 3] = (emucol & 0xFF000000) >> 24;
		}
	}
}

void Display::fillSpriteViewGfx()
{	
	int xOffset = 0;
	int yOffset = 0;
	
	for (int i = 0; i < DISPLAY_SPRITES; ++i)
	{
		const auto& spriteData = _spriteData[i];

		for (int y = 0; y < DISPLAY_TILE_ROWS; ++y)
		{
			for (int x = 0; x < DISPLAY_TILE_COLS; ++x)
			{
				dword* selPalette = isSpriteFlagSet(i, SPRITE_FLAG_PALETTE) ? _spr1Palette : _spr0Palette;
				dword emucol = selPalette[_tileset[spriteData.tile][y][x] & 0x3];

				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH]     = (emucol & 0x000000FF) >> 0;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 1] = (emucol & 0x0000FF00) >> 8;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 2] = (emucol & 0x00FF0000) >> 16;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 3] = (emucol & 0xFF000000) >> 24;
			}
		}

		xOffset += DISPLAY_TILE_COLS;

		if (xOffset > DISPLAY_SPRITE_AREA - DISPLAY_TILE_COLS)
		{
			xOffset = 0;
			yOffset += DISPLAY_TILE_ROWS;
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

bool Display::isSpriteFlagSet(const byte spriteIndex, const byte flag) const
{
	return (_spriteData[spriteIndex].flags & flag) != 0;
}

void Display::setSpriteFlag(const byte spriteIndex, const byte flag)
{
	_spriteData[spriteIndex].flags |= flag;
}