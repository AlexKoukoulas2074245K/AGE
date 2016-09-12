#include "input.h"
#include "memory.h"

#include <SDL_keyboard.h>

Input::Input()
	: _keys{0x0F, 0x0F}
	, _column(0)
{
}

void Input::resetInput()
{
	_keys[0] = 0x0F;
	_keys[1] = 0x0F;
	_column  = 0;
}

void Input::setIFRef(byte* intFlag)
{
	_intFlag = intFlag;
}

byte Input::readByte(const word addr)
{
	switch (_column)
	{
		case 0x10: return _keys[0]; break;
		case 0x20: return _keys[1]; break;
	}

	return 0;
}

void Input::writeByte(const word addr, const byte val)
{
	_column = val & 0x30;
}

void Input::keyDown(const int key)
{
	switch (key)
	{
		case SDLK_LEFT:      _keys[1] &= 0xD; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break; 
		case SDLK_RIGHT:     _keys[1] &= 0xE; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_UP:        _keys[1] &= 0xB; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_DOWN:      _keys[1] &= 0x7; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_x:         _keys[0] &= 0xD; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_z:         _keys[0] &= 0xE; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_RETURN:    _keys[0] &= 0x7; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_BACKSPACE: _keys[0] &= 0xB; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
	}
}

void Input::keyUp(const int key)
{
	switch (key)
	{
		case SDLK_LEFT:      _keys[1] |= 0xD; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_RIGHT:     _keys[1] |= 0xE; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_UP:        _keys[1] |= 0xB; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_DOWN:      _keys[1] |= 0x7; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_x:         _keys[0] |= 0xD; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_z:         _keys[0] |= 0xE; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_RETURN:    _keys[0] |= 0x7; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
		case SDLK_BACKSPACE: _keys[0] |= 0xB; *_intFlag |= Memory::INTERRUPT_FLAG_JOYPAD; break;
	}
}