#include "display.h"

#include "cpu.h"
#include "memory.h"
#include "window.h"

#include <memory.h>
#include <iostream>
#include <unordered_set>

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
static const byte DISPLAY_CONTROL_FLAG_BKGTS  = 0x10;
static const byte DISPLAY_CONTROL_FLAG_WINDOW = 0x20;
static const byte DISPLAY_CONTROL_FLAG_WINTM  = 0x40;
static const byte DISPLAY_CONTROL_FLAG_DISPL  = 0x80;

static const byte SPRITE_FLAG_PALETTE = 0x10;
static const byte SPRITE_FLAG_X_FLIP  = 0x20;
static const byte SPRITE_FLAG_Y_FLIP  = 0x40;
static const byte SPRITE_FLAG_PRIO    = 0x80;

Display::Display(fill_displays_callback_t fillDisplayCallback)

	: _displayMode(DISPLAY_MODE_HBLANK)
	, _displayClock(0)
	, _displayLine(0)
	, _displayControlRegister(0)
	, _statRegister(0)
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
		case 0xFF41: return _statRegister; break;
		case 0xFF42: return _displayScrollY; break;
		case 0xFF43: return _displayScrollX; break;
		case 0xFF44: return _displayLine; break;
		default:
			std::cout << " unimplemented display read at 0x" << std::hex << addr << std::endl;
	}
	return 0;
}

void Display::writeByte(const word addr, const byte val)
{
	switch (addr)
	{
		case 0xFF40: _displayControlRegister = val; break;
		case 0xFF41: _statRegister = val; break;
		case 0xFF42: _displayScrollY = val; break;
		case 0xFF43: _displayScrollX = val; break;
		case 0xFF47: 
		{
#ifdef PALETTE_SHIFT_ENABLED
			for (size_t i = 0; i < 4; ++i)
			{
				switch ((val >> (i * 2)) & 3)
				{
					case 0: _bkgPalette[i] = { 0xF8, 0xF8, 0xF8, 0x00 }; break;
					case 1: _bkgPalette[i] = { 0xA8, 0xA8, 0xA8, 0xFF }; break;
					case 2: _bkgPalette[i] = { 0x60, 0x60, 0x60, 0xFF }; break;
					case 3: _bkgPalette[i] = { 0x00, 0x00, 0x00, 0xFF }; break;
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
					case 0: _spr0Palette[i] = { 0xF8, 0xF8, 0xF8, 0x00 }; break;
					case 1: _spr0Palette[i] = { 0xA8, 0xA8, 0xA8, 0xFF }; break;
					case 2: _spr0Palette[i] = { 0x60, 0x60, 0x60, 0xFF }; break;
					case 3: _spr0Palette[i] = { 0x00, 0x00, 0x00, 0xFF }; break;
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
					case 0: _spr1Palette[i] = { 0xF8, 0xF8, 0xF8, 0x00 }; break;
					case 1: _spr1Palette[i] = { 0xA8, 0xA8, 0xA8, 0xFF }; break;
					case 2: _spr1Palette[i] = { 0x60, 0x60, 0x60, 0xFF }; break;
					case 3: _spr1Palette[i] = { 0x00, 0x00, 0x00, 0xFF }; break;
				}
			}
#endif
		} break;

		default:
			std::cout << " unimplemented display write at 0x" << std::hex << addr << std::endl;
	}
}

void Display::resetDisplay()
{
	// Clear Graphics
	memset(_gfx, 0x77, sizeof(_gfx));
	memset(_tileset, 0x00, sizeof(_tileset));
	memset(_tileGfx, 0x00, sizeof(_tileGfx));
	memset(_spriteGfx, 0x00, sizeof(_spriteGfx));

	_bkgPalette[0] = { 0xF8, 0xF8, 0xF8, 0x00 };
	_bkgPalette[1] = { 0xA8, 0xA8, 0xA8, 0xFF };
	_bkgPalette[2] = { 0x60, 0x60, 0x60, 0xFF };
	_bkgPalette[3] = { 0x00, 0x00, 0x00, 0xFF };

	_spr0Palette[0] = { 0xF8, 0xF8, 0xF8, 0x00 };
	_spr0Palette[1] = { 0xA8, 0xA8, 0xA8, 0xFF };
	_spr0Palette[2] = { 0x60, 0x60, 0x60, 0xFF };
	_spr0Palette[3] = { 0x00, 0x00, 0x00, 0xFF };

	_spr1Palette[0] = { 0xF8, 0xF8, 0xF8, 0x00 };
	_spr1Palette[1] = { 0xA8, 0xA8, 0xA8, 0xFF };
	_spr1Palette[2] = { 0x60, 0x60, 0x60, 0xFF };
	_spr1Palette[3] = { 0x00, 0x00, 0x00, 0xFF };

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

void Display::setIFRef(byte* intFlag)
{
	_intFlag = intFlag;
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
					fillTileViewGfx();
					fillSpriteViewGfx();
					_fillDisplayCallback(_gfx, _tileGfx, _spriteGfx);
					*_intFlag |= Memory::INTERRUPT_FLAG_VBLANK;
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

void Display::changeSpriteData(const word addr, const byte val)
{
	byte spriteIndex = addr >> 2;

	if (spriteIndex >= 40)
		return;

	switch (addr & 3)
	{
		// Y-coord
		case 0: 
		{
			_spriteData[spriteIndex].y = val - 16; 
		} break;

		// X-coord
		case 1: 
		{
			_spriteData[spriteIndex].x = val - 8;
		} break;
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
	byte scanRow[DISPLAY_COLS];

	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKG))
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
			byte col = (_tileset[tileIndex][y][x] & 0x3);
			pixel_t emulatedColor = _bkgPalette[col];

			_gfx[displayOffset]     = emulatedColor.r;
			_gfx[displayOffset + 1] = emulatedColor.g;
			_gfx[displayOffset + 2] = emulatedColor.b;
			_gfx[displayOffset + 3] = emulatedColor.a;
			displayOffset += 4;
			
			scanRow[i] = _tileset[tileIndex][y][x];

			if (++x == 8)
			{
				x = 0;
				lineOffset = (lineOffset + 1) & 31;
				tileIndex  = _vramRef[mapOffset + lineOffset]; 
				//if (isControlFlagSet(DISPLAY_CONTROL_FLAG_BKGTS) && tileIndex < 128) tileIndex += 256;
			}
		}	
	}

	if (isControlFlagSet(DISPLAY_CONTROL_FLAG_SPR))
	{
		for (byte i = 0; i < 40; ++i)
		{
			sprite_data sd = _spriteData[i];
			int sx = sd.x;
			int sy = sd.y;

			// Check if the sprite falls on this scanline
			if (sy <= _displayLine && (sy + 8) > _displayLine)
			{
				pixel_t* spritePalette = isSpriteFlagSet(i, SPRITE_FLAG_PALETTE) ? _spr1Palette : _spr0Palette;
				spritePalette = _bkgPalette;
				int displayOffset = (_displayLine * 160 + sx) * 4;

				byte* tileRow;

				if (isSpriteFlagSet(i, SPRITE_FLAG_Y_FLIP))
					tileRow = _tileset[sd.tile][7 - (_displayLine - sy)];
				else
					tileRow = _tileset[sd.tile][_displayLine - sy];

				for (size_t x = 0; x < 8; ++x)
				{
					// If pixel is still on screen &&
					// its not transparent (color 0) &&
					// (if the sprite has prio || shows under bg)
					// then render it
					if ((sx + x) >= 0 && (sx + x) < 160 && (!isSpriteFlagSet(i, SPRITE_FLAG_PRIO) || scanRow[sx + x] == 0))
					{
						// If X flip is set reverse pixel write						
						pixel_t color;
						if (!isSpriteFlagSet(i, SPRITE_FLAG_X_FLIP))
							color = spritePalette[tileRow[x]];
						else
							color = spritePalette[tileRow[7 - x]];
						

						_gfx[displayOffset]     = color.r;
						_gfx[displayOffset + 1] = color.g;
						_gfx[displayOffset + 2] = color.b;
						_gfx[displayOffset + 3] = color.a;

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
	for (int i = 0x9800; i <= 0x9BFF; ++i)
	{
		selectedTiles.insert(_vramRef[i - 0x8000]);
	}
#endif

	for (int y = 0; y < DISPLAY_TILE_VIEW_BASE_HEIGHT; ++y)
	{
		for (int x = 0; x < DISPLAY_TILE_VIEW_BASE_WIDTH; ++x)
		{
			int tileIndex = (y / DISPLAY_TILE_ROWS) * DISPLAY_TILE_VIEW_TILES_PER_ROW + x / DISPLAY_TILE_COLS;

			int arrayIndex = (y * DISPLAY_TILE_VIEW_BASE_WIDTH * DISPLAY_DEPTH) + x * DISPLAY_DEPTH;
			byte color = (_tileset[tileIndex][y][x] & 0x03);
			pixel_t emulatedColor = _bkgPalette[_tileset[tileIndex][y % DISPLAY_TILE_ROWS][x % DISPLAY_TILE_COLS]];
	
#ifdef SHOW_SELECTED_TILES
			if ((emulatedColor.r == 0xF8 ||
				emulatedColor.g == 0xA8) &&
				selectedTiles.count(tileIndex))
			{
				emulatedColor = {0x00, 0xFF, 0x00, 0xFF};
			}
#endif

			_tileGfx[arrayIndex]     = emulatedColor.r;
			_tileGfx[arrayIndex + 1] = emulatedColor.g;
			_tileGfx[arrayIndex + 2] = emulatedColor.b;
			_tileGfx[arrayIndex + 3] = emulatedColor.a;
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
				pixel_t* selPalette = isSpriteFlagSet(i, SPRITE_FLAG_PALETTE) ? _spr1Palette : _spr0Palette;

				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH]     = selPalette[_tileset[spriteData.tile][y][x] & 0x3].r;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 1] = selPalette[_tileset[spriteData.tile][y][x] & 0x3].g;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 2] = selPalette[_tileset[spriteData.tile][y][x] & 0x3].b;
				_spriteGfx[(yOffset + y) * DISPLAY_SPRITE_AREA * DISPLAY_DEPTH + (xOffset + x) * DISPLAY_DEPTH + 3] = selPalette[_tileset[spriteData.tile][y][x] & 0x3].a;
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