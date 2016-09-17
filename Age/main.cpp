#include <vld.h>

#include "common.h"
#include "memory.h"
#include "display.h"
#include "cpu.h"
#include "input.h"
#include "window.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <SDL.h>

#include <memory>

//#define CURR_ADDRESS_TO_BREAK 0x1FFA
//#define CURR_ADDRESS_TO_BREAK 0x02C6
#define CURR_ADDRESS_TO_BREAK 0x20BD

static const char* DEBUG_FLAG = "-d";

static SDL_Surface*  mainViewSurface;
static SDL_Surface*  tileViewSurface;
static SDL_Surface*  spriteViewSurface;

static std::unique_ptr<Window> tileView;
static std::unique_ptr<Window> spriteView;
static std::unique_ptr<Window> mainView;

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

void fillDisplay(byte* gfxData, byte* tileGfx, byte* spriteGfx)
{
	// Fill graphics and render main view
	SDL_FreeSurface(mainViewSurface);
	mainViewSurface = SDL_CreateRGBSurfaceFrom(
		static_cast<void*>(gfxData),
		160, 144,
		8 * Display::DISPLAY_DEPTH,
		Display::DISPLAY_COLS * Display::DISPLAY_DEPTH,
		0x000000FF,
		0x0000FF00,
		0x00FF0000,
		0xFF000000
	);
	
	mainView->setTextureFromSurface(mainViewSurface);
	mainView->render();

	// Fill graphics and render tile view
	if (tileView)
	{
		SDL_FreeSurface(tileViewSurface);
		tileViewSurface = SDL_CreateRGBSurfaceFrom(
			static_cast<void*>(tileGfx),
			Display::DISPLAY_TILE_VIEW_BASE_WIDTH, Display::DISPLAY_TILE_VIEW_BASE_HEIGHT,
			8 * Display::DISPLAY_DEPTH,
			Display::DISPLAY_TILE_VIEW_BASE_WIDTH * Display::DISPLAY_DEPTH,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000);

		tileView->setTextureFromSurface(tileViewSurface);
		tileView->render();
	}
	
	// Fill graphics and render sprite view
	if (spriteView)
	{
		SDL_FreeSurface(spriteViewSurface);
		spriteViewSurface = SDL_CreateRGBSurfaceFrom(
			static_cast<void*>(spriteGfx),
			Display::DISPLAY_SPRITE_VIEW_BASE_WIDTH, Display::DISPLAY_SPRITE_VIEW_BASE_HEIGHT,
			8 * Display::DISPLAY_DEPTH,
			Display::DISPLAY_SPRITE_VIEW_BASE_WIDTH * Display::DISPLAY_DEPTH,
			0x000000FF,
			0x0000FF00,
			0x00FF0000,
			0xFF000000);

		spriteView->setTextureFromSurface(spriteViewSurface);
		spriteView->render();
	}
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

	// Initialize SDL
	// TODO: Handle Errors
	SDL_Init(SDL_INIT_EVERYTHING);

	mainView   = std::make_unique<Window>(Display::DISPLAY_COLS * 4, Display::DISPLAY_ROWS * 4, 894, 30, "A.G.E");

	if (debugFlag)
	{
		tileView   = std::make_unique<Window>(Display::DISPLAY_TILE_VIEW_BASE_WIDTH * 2, Display::DISPLAY_TILE_VIEW_BASE_HEIGHT* 2, 638, 30, "Tile View");
		spriteView = std::make_unique<Window>(Display::DISPLAY_SPRITE_VIEW_BASE_WIDTH * 8,  Display::DISPLAY_SPRITE_VIEW_BASE_HEIGHT * 8, 382, 445, "Sprite View");
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

	// Load Rom
	loadRom(argv[1], memory);
	
	SDL_Event sdlEvent;
	bool running = true;
	bool shouldPrint = false;
	bool spacePressed = false;
	bool spacePressed0 = false;
	
	word pcHistory[1000];
	word pcHistoryIndex = 0;
	

	int i = 0;
	while(running)
	{
		while (SDL_PollEvent(&sdlEvent))
		{
			switch (sdlEvent.type)
			{
				case SDL_QUIT: running = false; break;
				case SDL_WINDOWEVENT: 
				{
					if (mainView)
						mainView->handleEvent(sdlEvent);
					if (tileView)
						tileView->handleEvent(sdlEvent);
					if (spriteView)
						spriteView->handleEvent(sdlEvent);
				} break;
				case SDL_KEYDOWN:
				{
					switch (sdlEvent.key.keysym.sym)
					{
						case SDLK_SPACE: spacePressed = true; break;
						case SDLK_ESCAPE: running = false; break;
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

				case SDL_MOUSEBUTTONDOWN:
				{
					if (sdlEvent.window.windowID == spriteView->getID())
					{
						display.printSpriteData(sdlEvent.button.x, sdlEvent.button.y);
					}
				} break;
			}
		}
		
		static int yes = false;
		if (*cpu.getPC() == CURR_ADDRESS_TO_BREAK)
		{			
			yes = true;
			shouldPrint = true;
		}

		if (spacePressed && !spacePressed0) 
			shouldPrint = !shouldPrint;

		cpu.emulateCycle();
		pcHistory[pcHistoryIndex] = *cpu.getPC();
		if (pcHistory[pcHistoryIndex] == pcHistory[pcHistoryIndex-1])
		{
			const auto b = false;
		}
		pcHistoryIndex = (pcHistoryIndex + 1) % 1000;
		cpu.handleInterrupts();
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
		display.emulateGameboyDisplay();
	

#if defined (_DEBUG) || defined (DEBUG)
		if (shouldPrint)
//			cpu.printRegisters();
#endif

		spacePressed0 = spacePressed;

		if (mainView && mainView->isDestroyed())
		{
			mainView = nullptr;
			running  = false;
		}
		if (tileView && tileView->isDestroyed()) 
			tileView = nullptr;
		if (spriteView && spriteView->isDestroyed()) 
			spriteView = nullptr;
	}

	return 0;
}