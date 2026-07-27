#pragma once
#include <string>

/* Cobalt - facing SDL3 wrapper.

 Layout (add a new .cpp for a new feature -- the bundle picks up every
 .cpp in this folder automatically, nothing else needs to change):
   sdl.hpp     - this file: the public API used from .cb scripts
   internal.hpp/state.cpp - shared SDL_Window* / SDL_Renderer* state
   window.cpp  - __Window__: create/poll/clear/present/quit
   draw.cpp    - __Draw__:   shape drawing
   input.cpp   - __Input__:  keyboard/mouse
   link.txt    - extra linker flags this bundle needs ("-lSDL3")

 Cobalt usage:
   @import <sdl>
   Window.Init("Title", 800, 600);
   while (Window.PollEvents()) {
       Window.Clear(20, 20, 30);
       Draw.Rect(100, 100, 50, 50, 220, 80, 80);
       Window.Present();
   }
   Window.Quit(); */

class __Window__ {
public:
	// Creates the window + renderer. Call once before the loop. Returns
	// false (and prints why to stderr) if SDL couldn't create a window --
	// e.g. no display available.
	bool Init(std::string title, int width, int height);

	// Pumps the OS event queue and returns false once the window has been
	// asked to close (X button, Alt+F4, etc). Meant to drive the loop:
	//     while (Window.PollEvents()) { ... }
	bool PollEvents();

	// Clears the backbuffer to a solid color (0-255 per channel). Call at
	// the start of a frame, before any Draw.* calls.
	void Clear(int r, int g, int b);

	// Presents everything drawn since Clear(). Call at the end of a frame.
	void Present();

	// Tears down the renderer/window/SDL. Safe to call even if Init()
	// failed or was never called.
	void Quit();
};

class __Draw__ {
public:
	void Rect(int x, int y, int w, int h, int r, int g, int b);
	void RectOutline(int x, int y, int w, int h, int r, int g, int b);
	void Line(int x1, int y1, int x2, int y2, int r, int g, int b);
	void Point(int x, int y, int r, int g, int b);
};

class __Input__ {
public:
	// Key names match SDL's scancode names, e.g. "A", "SPACE", "LEFT",
	// "ESCAPE", "RETURN". Returns false for an unrecognized name.
	bool KeyDown(std::string key);

	int MouseX();
	int MouseY();

	// 1 = left, 2 = middle, 3 = right (SDL's own button numbering).
	bool MouseDown(int button);
};