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
#include <SDL_image.h>

#include <memory>

//#define CURR_ADDRESS_TO_BREAK 0x1FFA
//#define CURR_ADDRESS_TO_BREAK 0x02C6
#define CURR_ADDRESS_TO_BREAK 0xC2A6

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

#if defined(DEBUG) || defined(_DEBUG)
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
#endif
}

int main(int argc, char* argv[])
{	
	// Initialize SDL
	// TODO: Handle Errors
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_EventState(SDL_DROPFILE, SDL_ENABLE);
	
	mainView   = std::make_unique<Window>(592, 540, 894, 30, "A.G.E");
	
#ifdef _DEBUG
	tileView   = std::make_unique<Window>(Display::DISPLAY_TILE_VIEW_BASE_WIDTH * 2, Display::DISPLAY_TILE_VIEW_BASE_HEIGHT* 2, 638, 30, "Tile View");
	spriteView = std::make_unique<Window>(Display::DISPLAY_SPRITE_VIEW_BASE_WIDTH * 8,  Display::DISPLAY_SPRITE_VIEW_BASE_HEIGHT * 8, 382, 445, "Sprite View");	
#endif

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
	//loadRom(argv[1], memory);
	
	
	SDL_Event sdlEvent;
	bool running = true;
	bool hasRomBeenLoaded = false;
	bool shouldPrint      = false;
	bool spacePressed     = false;
	bool spacePressed0    = false;
	bool aPressed         = false;
	bool aPressed0        = false;
	bool sPressed         = false;
	bool sPressed0        = false;
	
	word pcHistory[10000];
	word pcHistoryIndex = 0;
	
	SDL_SetWindowTitle(mainView->getWindowHandle(), "Drag n' Drop a ROM file inside this window!");

	int i = 0;
	while (running)
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
						case SDLK_a: aPressed = true; break;
						case SDLK_s: sPressed = true; break;
						case SDLK_ESCAPE: running = false; break;
						default: input.keyDown(sdlEvent.key.keysym.sym);
					}
					
				} break;
				case SDL_KEYUP:
				{
					switch (sdlEvent.key.keysym.sym)
					{
						case SDLK_SPACE: spacePressed = false; break;
						case SDLK_a: aPressed = false; break;
						case SDLK_s: sPressed = false; break;
						default: input.keyUp(sdlEvent.key.keysym.sym);
					}
				} break;

				case SDL_MOUSEBUTTONDOWN:
				{
					//if (sdlEvent.window.windowID == spriteView->getID())
					//{
						//display.printSpriteData(sdlEvent.button.x, sdlEvent.button.y);
					//}
				} break;

				case SDL_DROPFILE:
				{
					char* droppedRomPath = sdlEvent.drop.file;					
					memory.resetMemory();
					cpu.resetCpu();
					input.resetInput();
					display.resetDisplay();
					memory.setPcRef(cpu.getPC());
					display.setZ80TimeRegister(cpu.getT());
					input.setIFRef(memory.getIFPtr());
					loadRom(droppedRomPath, memory);
					hasRomBeenLoaded = true;					
					SDL_free(droppedRomPath);

					SDL_SetWindowTitle(mainView->getWindowHandle(), ("Emulating: " + memory.getCartName()).c_str());
				} break;
			}
		}

#if defined (DEBUG) || defined(_DEBUG)
		if (*cpu.getPC() == CURR_ADDRESS_TO_BREAK)
		{			
			shouldPrint = true;
		}
		
		
		if (aPressed && !aPressed0)
		{
			byte lcdc = memory.readByte(0xFF40);
			if (lcdc & 0x01)
				memory.writeByte(0xFF40, lcdc & ~0x01);
			else
				memory.writeByte(0xFF40, lcdc | 0x01);
		}

		if (sPressed & !sPressed0)
		{
			byte lcdc = memory.readByte(0xFF40);
			if (lcdc & 0x20)
				memory.writeByte(0xFF40, lcdc & ~0x20);
			else
				memory.writeByte(0xFF40, lcdc | 0x20);
		}
#endif
		if (hasRomBeenLoaded)
		{
			cpu.emulateCycle();
			pcHistory[pcHistoryIndex] = *cpu.getPC();
			if (pcHistory[pcHistoryIndex] == pcHistory[pcHistoryIndex - 1])
			{
				const auto b = false;
			}
			pcHistoryIndex = (pcHistoryIndex + 1) % 10000;
			cpu.handleInterrupts();
			display.emulateGameboyDisplay();
		}		
			
#if defined (_DEBUG) || defined (DEBUG)
		if (shouldPrint)
			cpu.printRegisters();

		spacePressed0 = spacePressed;
		aPressed0 = aPressed;
		sPressed0 = sPressed;

		if (mainView && mainView->isDestroyed())
		{
			mainView = nullptr;
			running  = false;
		}
		if (tileView && tileView->isDestroyed()) 
		{
			tileView = nullptr;
		}
		if (spriteView && spriteView->isDestroyed()) 
		{
			spriteView = nullptr;
		}
#endif
	}
	return 0;
}