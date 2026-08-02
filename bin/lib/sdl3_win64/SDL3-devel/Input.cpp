#include "sdl.hpp"
#include "internal.hpp"
#include <SDL3/SDL.h>

using namespace sdlint;

bool __Input__::KeyDown(std::string key) {
	SDL_Scancode code = SDL_GetScancodeFromName(key.c_str());
	if (code == SDL_SCANCODE_UNKNOWN) return false;
	const bool* state = SDL_GetKeyboardState(nullptr);
	return state[code];
}

int __Input__::MouseX() {
	float x, y;
	SDL_GetMouseState(&x, &y);
	return (int)x;
}

int __Input__::MouseY() {
	float x, y;
	SDL_GetMouseState(&x, &y);
	return (int)y;
}

bool __Input__::MouseDown(int button) {
	float x, y;
	SDL_MouseButtonFlags flags = SDL_GetMouseState(&x, &y);
	return (flags & SDL_BUTTON_MASK(button)) != 0;
}