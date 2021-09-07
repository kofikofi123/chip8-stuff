#include "App.h"
#include <cstring>
#include <iostream>

/**
0 1 2 
**/
App::App():
	sdl_error(false),
	app_quit(false),
	c8(std::make_unique<Chip8>()),
	time_cap((1.0 / 600.0) * 1000.0),
	freq((double)SDL_GetPerformanceFrequency()),
	disp_factor(RES_FACTOR),
	keyboardMap({
		{SDL_SCANCODE_1, 0x01},
		{SDL_SCANCODE_2, 0x02},
		{SDL_SCANCODE_3, 0x03},
		{SDL_SCANCODE_4, 0x0C},
		{SDL_SCANCODE_Q, 0x04},
		{SDL_SCANCODE_W, 0x05},
		{SDL_SCANCODE_E, 0x06},
		{SDL_SCANCODE_R, 0x0D},
		{SDL_SCANCODE_A, 0x07},
		{SDL_SCANCODE_S, 0x08},
		{SDL_SCANCODE_D, 0x09},
		{SDL_SCANCODE_F, 0x0E},
		{SDL_SCANCODE_Z, 0x0A},
		{SDL_SCANCODE_X, 0x00},
		{SDL_SCANCODE_C, 0x0B},
		{SDL_SCANCODE_V, 0x0F}
	})
{
	uint16_t res_x = 64 * disp_factor;
	uint16_t res_y = 32 * disp_factor;

	SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Chip-8",
							  SDL_WINDOWPOS_CENTERED,
							  SDL_WINDOWPOS_CENTERED,
							  res_x,
							  res_y,
							  0);

	renderer = SDL_CreateRenderer(window,
								  -1,
								  SDL_RENDERER_TARGETTEXTURE | SDL_RENDERER_ACCELERATED);

	texture = SDL_CreateTexture(renderer,
								SDL_PIXELFORMAT_RGB332,
								SDL_TEXTUREACCESS_STREAMING,
								64,
								32);



	framebuffer_raw = std::shared_ptr<uint8_t[]>(new uint8_t[64 * 32]);

	c8->setupFramebuffer(framebuffer_raw);
	resetEmulator();
}
/*
void App::updateFramebuffer(uint8_t* pixels, int pitch) {
	
	uint8_t* chip_framebuffer = framebuffer_raw.get();
	uint8_t* p = pixels;
	uint8_t width = disp_factor * 4;
	for (uint8_t y = 0; y < 32; y++) {
		for (uint8_t x = 0; x < 64; x++) {
			uint8_t val = (*chip_framebuffer++) * 0xFF;

			uint8_t* temp = p;
			uint8_t depth = disp_factor;

			while (depth-- > 0) {
				std::memset(p, val, width);
				p = p + (pitch);
			}

			p = temp + width;
		}
		p = pixels + (y * pitch * disp_factor);
	}

	std::memcpy(pixels, framebuffer_raw.get(), pitch * 32);
	//std::cout << pitch << std::endl;
}*/

void App::handleEvents() {
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				app_quit = true;
				break;
			case SDL_KEYDOWN: {
				SDL_KeyboardEvent* key_event = &event.key;
				auto find = keyboardMap.find(key_event->keysym.scancode);

				if (find != keyboardMap.end()) {
					c8->registerKeydown(find->second);
				}
				break;
			}
			case SDL_KEYUP: {
				SDL_KeyboardEvent* key_event = &event.key;
				auto find = keyboardMap.find(key_event->keysym.scancode);

				if (find != keyboardMap.end()) {
					c8->registerKeyup(find->second);
				}
				break;
			}
		}
	}
}

void App::loop() {
	//last_time = 0.0;
	/*
	while (!appQuit() && !error()) {

	}*/
}

void App::resetEmulator(const std::string& filename) {
	c8->reset();
	c8->loadFileIntoMemory("Chip8Font.bin", 0);
	c8->loadFileIntoMemory(filename, 0x200);
}

void App::stepEmulator() {
	uint8_t cycles = 10;
	while (!c8->error() && cycles-- > 0) {
		c8->step( );
	}

	c8->clockTimers();
}


void App::renderWindow() {
	SDL_RenderClear(renderer);

	uint8_t* pixels = nullptr;
	int pitch = 0;

	SDL_LockTexture(texture,
					NULL,
					reinterpret_cast<void**>(&pixels),
					&pitch);

	std::memcpy(pixels, framebuffer_raw.get(), pitch * 32);

	SDL_UnlockTexture(texture);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

App::~App() {
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}