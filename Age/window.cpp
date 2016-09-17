#include "window.h"

#include <SDL.h>

Window::Window(
	const word width,
	const word height,
	const word x,
	const word y,
	const std::string& title)

	: _destroyed(false)
	, _focused(true)
	, _originalWidth(width)
	, _originalHeight(height)
	, _currTexture(nullptr)
{
	_windowHandle   = SDL_CreateWindow(title.c_str(), x, y, width, height, 0);
	_rendererHandle = SDL_CreateRenderer(_windowHandle, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	_id             = SDL_GetWindowID(_windowHandle);
}


Window::~Window()
{
	SDL_DestroyRenderer(_rendererHandle);
	SDL_DestroyWindow(_windowHandle);
}

void Window::handleEvent(const SDL_Event& e)
{
	if (e.type == SDL_WINDOWEVENT && e.window.windowID == _id)
	{
		switch (e.window.event)
		{
			case SDL_WINDOWEVENT_CLOSE:        _destroyed = true; break;
			case SDL_WINDOWEVENT_SIZE_CHANGED: render(); break;
			case SDL_WINDOWEVENT_FOCUS_GAINED: _focused = true; break;
			case SDL_WINDOWEVENT_FOCUS_LOST:   _focused = false; break;
		}
	}
}

bool Window::isDestroyed() const { return _destroyed; }
bool Window::isFocused() const { return _focused; }
word Window::getWidth() const { return _originalWidth; }
word Window::getHeight() const { return _originalHeight; }
int Window::getID() const { return _id; }

void Window::setTextureFromSurface(SDL_Surface* surface)
{
	SDL_DestroyTexture(_currTexture);
	_currTexture = SDL_CreateTextureFromSurface(_rendererHandle, surface);
}

void Window::render()
{
	SDL_SetRenderDrawColor(_rendererHandle, 0xE0, 0xF8, 0xD0, 0xFF);
	SDL_RenderClear(_rendererHandle);

	if (_currTexture)
		SDL_RenderCopy(_rendererHandle, _currTexture, nullptr, nullptr);

	SDL_RenderPresent(_rendererHandle);
}