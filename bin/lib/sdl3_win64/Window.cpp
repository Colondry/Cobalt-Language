#include "sdl3_win64.hpp"
#include "internal.hpp"
#include <SDL3/SDL.h>
#include <cstdio>

using namespace sdlint;

RGB hexStringToRgb(const std::string& hexStr) {
	RGB color{ 0, 0, 0 };

	// Check if the string starts with '#' and offset the pointer accordingly
	const char* cStr = (hexStr[0] == '#') ? hexStr.c_str() + 1 : hexStr.c_str();

	// %02x reads exactly 2 hexadecimal characters
	std::sscanf(cStr, "%02x%02x%02x", &color.r, &color.g, &color.b);

	return color;
}

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

bool __Window__::UpdateEvents() {
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

void __Window__::BackColor(std::string hexColor) {
	if (!renderer) return;
	RGB rgb = hexStringToRgb(hexColor);
	SDL_SetRenderDrawColor(renderer, rgb.r, rgb.g, rgb.b, 255);
	SDL_RenderClear(renderer);
}

void __Window__::Show() {
	if (!renderer) return;
	SDL_RenderPresent(renderer);
}

void __Window__::Close() {
	if (renderer) { SDL_DestroyRenderer(renderer); renderer = nullptr; }
	if (window) { SDL_DestroyWindow(window); window = nullptr; }
	SDL_Quit();
	running = false;
}

std::uint32_t GetMouseState(int& mouseX, int& mouseY) {
	float mx = static_cast<float>(mouseX);
	float my = static_cast<float>(mouseY);
	std::uint32_t state = SDL_GetMouseState(&mx, &my);
	mouseX = static_cast<int>(mx);
	mouseY = static_cast<int>(my);
	return state;
}