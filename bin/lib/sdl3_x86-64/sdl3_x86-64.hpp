#pragma once
#include <string>

struct RGB {
	int r, g, b;
};

RGB hexStringToRgb(const std::string& hexStr);

class __Window__ {
public:
	// Creates the window + renderer. Call once before the loop. Returns
	// false (and prints why to stderr) if SDL couldn't create a window --
	// e.g. no display available.
	bool Init(std::string title, int width, int height);

	// Pumps the OS event queue and returns false once the window has been
	// asked to close (X button, Alt+F4, etc). Meant to drive the loop:
	//     while (Window.PollEvents()) { ... }
	bool UpdateEvents();

	// Clears the backbuffer to a solid color (0-255 per channel). Call at
	// the start of a frame, before any Draw.* calls.
	void BackColor(std::string hexColor);

	// Presents everything drawn since BackColor(). Call at the end of a frame.
	void Show();

	// Tears down the renderer/window/SDL. Safe to call even if Init()
	// failed or was never called.
	void Close();
};

class __Draw__ {
public:
	void Rect(int x, int y, int w, int h, std::string hexColor);
	void RectOutline(int x, int y, int w, int h, std::string hexColor);
	void Line(int x1, int y1, int x2, int y2, std::string hexColor);
	void Point(int x, int y, std::string hexColor);
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