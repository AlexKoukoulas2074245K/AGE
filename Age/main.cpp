#include <vld.h>

#include "common.h"
#include "memory.h"
#include "display.h"
#include "cpu.h"
#include "input.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <SDL.h>
// 2800
// 371
#define CURR_ADDRESS_TO_BREAK 0x02800

static const char* DEBUG_FLAG = "-d";

static SDL_Window*   sdlWindow;
static SDL_Renderer* sdlRenderer;
static SDL_Surface*  sdlDisplaySurface;
static SDL_Texture*  sdlDisplayTexture;

void loadRom(const char* const arg, Memory& memory)
{
	std::ifstream file;
	file.open(arg, std::ios::binary|std::ios::ate);
	if (file.is_open())
	{
		std::ifstream::pos_type pos = file.tellg();
		std::vector<char> programData(static_cast<int>(pos));

		file.seekg(0, std::ios::beg);
		file.read(&programData[0], pos);
		
		memory.fillRom(programData);
	}
}

void fillDisplay(byte* gfxData)
{
	SDL_SetRenderDrawColor(sdlRenderer, 248, 248, 248, 255);
	SDL_RenderClear(sdlRenderer);
	SDL_FreeSurface(sdlDisplaySurface);

	sdlDisplaySurface = SDL_CreateRGBSurfaceFrom(
		static_cast<void*>(gfxData),
		160, 144,
		8 * Display::DISPLAY_DEPTH,
		Display::DISPLAY_COLS * 4,
		0x000000FF,
		0x0000FF00,
		0x00FF0000,
		0xFF000000
	);
	
	SDL_DestroyTexture(sdlDisplayTexture);
	sdlDisplayTexture = SDL_CreateTextureFromSurface(sdlRenderer, sdlDisplaySurface);
	SDL_RenderCopy(sdlRenderer, sdlDisplayTexture, nullptr, nullptr);
	SDL_RenderPresent(sdlRenderer);
}

int main(int argc, char* argv[])
{
	// Parse Arguments
	if (argc < 2)
	{
		std::cout << "Usage: <rom_file_path> [-d] (debug mode)" << std::endl;
		return -1;
	}

	bool debugFlag = false;
	if (argc > 2)
	{
		for (size_t i = 2; i < static_cast<size_t>(argc); ++i)
		{	
			debugFlag = strcmp(argv[i], DEBUG_FLAG) == 0;
		}
	}

	// Initialize Core Systems
	Input input;
	Display display(fillDisplay);
	Memory memory(display, input);
	Cpu cpu(memory);

	// Set additional dependencies in core systems
	memory.setPcRef(cpu.getPC());
	display.setZ80TimeRegister(cpu.getT());
	input.setIFRef(memory.getIFPtr());

	// Initialize SDL
	// TODO: Handle Errors
	SDL_Init(SDL_INIT_EVERYTHING);
	sdlWindow = SDL_CreateWindow(
		"A.G.E",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		Display::DISPLAY_COLS * 4,
		Display::DISPLAY_ROWS * 4,
		0);

	sdlRenderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
	

	// Load Rom
	loadRom(argv[1], memory);
	
	SDL_Event sdlEvent;
	bool running = true;
	bool shouldPrint = false;
	bool spacePressed = false;
	bool spacePressed0 = false;
	int i = 0;
	while(running)
	{
		while (SDL_PollEvent(&sdlEvent))
		{
			switch (sdlEvent.type)
			{
				case SDL_QUIT: running = false; break;
				case SDL_KEYDOWN:
				{
					switch (sdlEvent.key.keysym.sym)
					{
						case SDLK_SPACE: spacePressed = true; break;
						default: input.keyDown(sdlEvent.key.keysym.sym);
					}
					
				} break;
				case SDL_KEYUP:
				{
					switch (sdlEvent.key.keysym.sym)
					{
						case SDLK_SPACE: spacePressed = false; break;
						default: input.keyUp(sdlEvent.key.keysym.sym);
					}
				} break;
			}
		}
		
		if (*cpu.getPC() == CURR_ADDRESS_TO_BREAK &&
			(*cpu.getPC() < 0x0095 || *cpu.getPC() > 0x0343))
		{			
			i++;
		}
		if (i == 2 && *cpu.getPC() == 0x0463)
		{
			shouldPrint = true;
		}
		if (i == 2 && *cpu.getPC() == 0xFFB6)
		{
			shouldPrint = true;
		}
		if (spacePressed && !spacePressed0) 
			shouldPrint = !shouldPrint;

		cpu.emulateCycle();
		display.emulateGameboyDisplay();
		
		if (cpu.getIME() && memory.getIE() && memory.getIF())
		{
			byte maskedInterrupts = memory.getIE() & memory.getIF();

			if ((maskedInterrupts & Memory::INTERRUPT_FLAG_VBLANK) != 0)
			{
				memory.resetInterrupt(Memory::INTERRUPT_FLAG_VBLANK);
				cpu.RST40();
			}
			else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_TOGGLELCD) != 0)
			{
				memory.resetInterrupt(Memory::INTERRUPT_FLAG_TOGGLELCD);
				cpu.RST48();
			}
			else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_TIMER) != 0)
			{
				memory.resetInterrupt(Memory::INTERRUPT_FLAG_TIMER);
				cpu.RST50();
			}
			else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_SERIAL) != 0)
			{
				memory.resetInterrupt(Memory::INTERRUPT_FLAG_SERIAL);
				cpu.RST58();
			}
			else if ((maskedInterrupts & Memory::INTERRUPT_FLAG_JOYPAD) != 0)
			{
				memory.resetInterrupt(Memory::INTERRUPT_FLAG_JOYPAD);
				cpu.RST60();
			}
		}

#if defined (_DEBUG) || defined (DEBUG)
		if (shouldPrint)
			//cpu.printRegisters();
#endif

		spacePressed0 = spacePressed;
	}
	
	SDL_DestroyRenderer(sdlRenderer);
	SDL_DestroyWindow(sdlWindow);

	return 0;
}