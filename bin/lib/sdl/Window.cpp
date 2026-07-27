#include "sdl.hpp"
#include "internal.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace sdlint;

bool __Window__::Init(std::string title, int width, int height) {
	if (!SDL_Init(SDL_INIT_VIDEO)) {
		std::fprintf(stderr, "sdl: SDL_Init() failed: %s\n", SDL_GetError());
		return false;
	}

	window = SDL_CreateWindow(title.c_str(), width, height, SDL_WINDOW_RESIZABLE);
	if (!window) {
		std::fprintf(stderr, "sdl: SDL_CreateWindow() failed: %s\n", SDL_GetError());
		SDL_Quit();
		return false;
	}

	renderer = SDL_CreateRenderer(window, nullptr);
	if (!renderer) {
		std::fprintf(stderr, "sdl: SDL_CreateRenderer() failed: %s\n", SDL_GetError());
		SDL_DestroyWindow(window);
		window = nullptr;
		SDL_Quit();
		return false;
	}
	SDL_SetRenderVSync(renderer, 1);

	running = true;
	return true;
}

bool __Window__::PollEvents() {
	if (!running || !window) return false;

	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) running = false;
		if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
			event.window.windowID == SDL_GetWindowID(window)) {
			running = false;
		}
	}
	return running;
}

void __Window__::Clear(int r, int g, int b) {
	if (!renderer) return;
	SDL_SetRenderDrawColor(renderer, (Uint8)r, (Uint8)g, (Uint8)b, 255);
	SDL_RenderClear(renderer);
}

void __Window__::Present() {
	if (!renderer) return;
	SDL_RenderPresent(renderer);
}

void __Window__::Quit() {
	if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
	if (window) { SDL_DestroyWindow(window); window = nullptr; }
	SDL_Quit();
	running = false;
}