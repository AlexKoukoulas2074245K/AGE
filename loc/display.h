#pragma once

#include "common.h"

#include <functional>
#include <memory>

class Window;
class Memory;
class Display final
{
public:
	static const word DISPLAY_COLS        = 160;
	static const word DISPLAY_ROWS        = 144;
	static const byte DISPLAY_DEPTH       = 4;
	static const word DISPLAY_TILES       = 384;
	static const word DISPLAY_SPRITES     = 40;
	static const word DISPLAY_TILE_COLS   = 8;
	static const word DISPLAY_TILE_ROWS   = 8;
	static const word DISPLAY_SPRITE_AREA = 64;

	static const word DISPLAY_TILE_VIEW_BASE_WIDTH     = 128;
	static const word DISPLAY_TILE_VIEW_BASE_HEIGHT    = 192;
	static const word DISPLAY_SPRITE_VIEW_BASE_WIDTH   = 64;
	static const word DISPLAY_SPRITE_VIEW_BASE_HEIGHT  = 40;
	static const word DISPLAY_TILE_VIEW_TILES_PER_ROW = 16;

	struct sprite_data
	{
		int x, y;
		byte tile;
		byte flags;
	};

	using fill_displays_callback_t  = std::function<void(byte*, byte*, byte*)>;

public:
	Display(fill_displays_callback_t);

	byte readByte(const word addr);
	void writeByte(const word addr, const byte val);

	void resetDisplay();
	
	void setZ80TimeRegister(const timer_t* T);
	void setMemory(Memory* const memory);

	void emulateGameboyDisplay();
	void changeSpriteData(const word addr, const byte val);
	void changeTileData(const word tile, const word x, const word y, const byte color);
	void printSpriteData(const int mouseX, const int mouseY);

private:

	void renderScanline();
	void fillTileViewGfx();
	void fillSpriteViewGfx();
	bool isControlFlagSet(const byte flag) const;
	void setControlFlag(const byte flag);
	bool isSpriteFlagSet(const byte spriteIndex, const byte flag) const;
	void setSpriteFlag(const byte spriteIndex, const byte flag);
	
private:
	enum display_mode
	{
		DISPLAY_MODE_HBLANK,
		DISPLAY_MODE_VBLANK,
		DISPLAY_MODE_OAM_READ,
		DISPLAY_MODE_VRAM_READ
	};

private:
	byte _gfx[DISPLAY_COLS * DISPLAY_ROWS * DISPLAY_DEPTH];
	byte _tileset[DISPLAY_TILES][DISPLAY_TILE_ROWS][DISPLAY_TILE_COLS];
	byte _tileGfx[DISPLAY_TILE_VIEW_BASE_WIDTH * DISPLAY_TILE_VIEW_BASE_HEIGHT* DISPLAY_DEPTH];
	byte _spriteGfx[DISPLAY_SPRITE_VIEW_BASE_WIDTH * DISPLAY_SPRITE_VIEW_BASE_HEIGHT * DISPLAY_DEPTH];

	sprite_data _spriteData[DISPLAY_SPRITES];
	
	dword _bkgPalette[4];
	dword _spr0Palette[4];
	dword _spr1Palette[4];

	const timer_t* _z80Time;
	Memory* _memory;

	display_mode   _displayMode;
	timer_t        _displayClock;
	byte           _displayLine;
	byte           _displayScrollX;
	byte           _displayScrollY;
	byte           _displayWindowX;
	byte           _displayWindowY;
	byte           _displayLYC;	
	byte           _displayControlRegister;
	byte           _statRegister;

	fill_displays_callback_t  _fillDisplayCallback;
};
