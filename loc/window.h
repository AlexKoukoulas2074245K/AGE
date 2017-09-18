#pragma once

#include "common.h"

#include <string>

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Surface;
struct SDL_Texture;
union SDL_Event;
class Window
{
public:

	Window(
		const word width,
		const word height, 
		const word x,
		const word y,
		const std::string& title);
	~Window();

	bool isDestroyed() const;
	bool isFocused() const;
	word getWidth() const;
	word getHeight() const;
	int getID() const;
	SDL_Renderer* getRendererHandle() const;

	void handleEvent(const SDL_Event& e);
	void render();
	void setTextureFromSurface(SDL_Surface* surface);

private:

	int _id;
	bool _destroyed;
	bool _focused;
	const word _originalWidth;
	const word _originalHeight;
	SDL_Window* _windowHandle;
	SDL_Renderer* _rendererHandle;
	SDL_Texture* _currTexture;
};
