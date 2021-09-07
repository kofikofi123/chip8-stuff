#ifndef APP_H
#define APP_H

#include "SDL.h"
#include "Chip8.h"
#include <map>

#define RES_FACTOR 10

class App {
	bool sdl_error;
	bool app_quit;

	std::unique_ptr<Chip8> c8;
	std::shared_ptr<uint8_t[]> framebuffer_raw;

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture;
	SDL_Event event;

	double time_cap;
	double freq;
	uint64_t clock;
	uint32_t framebuffer_size;
	uint8_t disp_factor;

	std::map<SDL_Scancode, uint8_t> keyboardMap;

	void updateFramebuffer(uint8_t*, int);
public:
	App();
	~App();

	void handleEvents();
	void stepEmulator();
	void renderWindow();
	void resetEmulator(const std::string& = "roms/Test.ch8");
	void loop();
	

	bool appQuit() { return app_quit; }
	bool sdlError() { return sdl_error; }

	bool error() { return c8->error(); }
	std::string getError() { return c8->getError(); }
};


#endif