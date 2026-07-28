#pragma once

// Shared SDL state used across window.cpp / draw.cpp / input.cpp.
// Defined once in state.cpp. Not part of the public API -- .cb scripts
// never see this, only sdl.hpp.
struct SDL_Window;
struct SDL_Renderer;

namespace sdlint {
	extern SDL_Window* window;
	extern SDL_Renderer* renderer;
	extern bool running;
}