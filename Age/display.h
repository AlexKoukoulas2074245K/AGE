#pragma once

#include "common.h"

#include <functional>

class Display final
{
public:
	static const word DISPLAY_COLS      = 160;
	static const word DISPLAY_ROWS      = 144;
	static const byte DISPLAY_DEPTH     = 4;
	static const word DISPLAY_TILES     = 384;
	static const word DISPLAY_TILE_COLS = 8;
	static const word DISPLAY_TILE_ROWS = 8;

	using pixel_t = struct pixel
	{
		byte r, g, b, a;
	};

	using fill_display_callback_t = std::function<void(byte*)>;

public:
	Display(fill_display_callback_t);

	byte readByte(const word addr);
	void writeByte(const word addr, const byte val);

	void resetDisplay();
	void setZ80TimeRegister(const clock_t* T);
	void setVramRef(byte* vram);
	void emulateGameboyDisplay();
	void updateTile(const word tile, const word x, const word y, const byte color);

private:
	void renderScanline();
	bool isControlFlagSet(const byte flag) const;
	void setControlFlag(const byte flag);
	void setPixel(const byte x, const byte y, const pixel_t& pixel);
	pixel_t getPixel(const byte x, const byte y) const;

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
	pixel_t _palette[4];

	const clock_t* _z80Time;
	byte*          _vramRef;

	display_mode   _displayMode;
	clock_t        _displayClock;
	byte           _displayLine;
	byte           _displayScrollX;
	byte           _displayScrollY;
	byte           _displayControlRegister;

	fill_display_callback_t _fillDisplayCallback;
};
