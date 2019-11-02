#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>
#include <math.h>
#include "Chip8.h"

static char* readFile(const char*, uint32_t*);
static uint8_t getKeyCode(SDL_Scancode);


const SDL_Scancode CHIP8_KEYBOARD_MAP[16] = {
	SDL_SCANCODE_Q, SDL_SCANCODE_W, SDL_SCANCODE_E, SDL_SCANCODE_R,
	SDL_SCANCODE_A, SDL_SCANCODE_S, SDL_SCANCODE_D, SDL_SCANCODE_F,
	SDL_SCANCODE_Z, SDL_SCANCODE_X, SDL_SCANCODE_C, SDL_SCANCODE_V,
	SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, 
};

int main(int argc, char* argv[]){
	if (argc < 2) return 1;
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	atexit(SDL_Quit);

	uint8_t width = 64;
	uint8_t height = 32;
	uint8_t scale = 10;

	SDL_Window* window = SDL_CreateWindow("Chip-8",
										  SDL_WINDOWPOS_CENTERED,
										  SDL_WINDOWPOS_CENTERED,
										  width * scale,
										  height * scale,
										  0);

	SDL_Renderer* renderer = SDL_CreateRenderer(window, 
											  -1,
											  SDL_RENDERER_ACCELERATED);

	SDL_Texture* texture = SDL_CreateTexture(renderer,
											 SDL_PIXELFORMAT_RGBA32,
											 SDL_TEXTUREACCESS_STREAMING,
											 width * scale,
											 height * scale);

	/////////////////////////////////////////////////////////
	struct Chip8* chip = newEmulator(width, height, scale);

	uint32_t bytes = 0;
	uint32_t fontBytes = 0;
	char* font = readFile("Chip8Font.bin", &fontBytes);
	char* stuff = readFile(argv[1], &bytes);

	if (stuff == NULL || font == NULL){
		free(stuff);
		free(font);

		releaseEmulator(chip);
		SDL_DestroyTexture(texture);
		SDL_DestroyRenderer(renderer);
		SDL_DestroyWindow(window);
		return 0;
	}

	loadIntoEmulator(chip, font, fontBytes, 0);
	loadIntoEmulator(chip, stuff, bytes, 0x200);

	free(stuff);
	free(font);

	chip->pc = 0x200;

	uint8_t* keyboard = chip->keyboard;

	//////////////////////////////////////////////////////////


	uint8_t running = 1;
	SDL_Event event;


	uint64_t start = 0;
	uint64_t end = 0;

	float fps = 60.0f * 10.0f;

	float freq = SDL_GetPerformanceFrequency() * 1000.0f;
	float cap = (1.0f / fps) * 1000.0f;
	while (running){
		start = SDL_GetPerformanceCounter();
		while (SDL_PollEvent(&event)){
			if (event.type == SDL_QUIT){
				running = 0;
			}else if (event.type == SDL_KEYDOWN){
				chip->isKeyWait = 0;
				uint8_t key = getKeyCode(event.key.keysym.scancode);


				if (key != 0xFF){
					keyboard[key] = 1;
					chip->currentKey = key;
				}


			}else if (event.type == SDL_KEYUP){
				uint8_t key = getKeyCode(event.key.keysym.scancode);

				if (key != 0xFF){
					keyboard[key] = 0;
					chip->currentKey = 0xFF;
				}
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
		SDL_RenderClear(renderer);

		

		if (!chip->isKeyWait)
			stepEmulator(chip);

		uint8_t delayTimer = chip->delayRegister;

		if (delayTimer > 0){
			chip->delayRegister--;
		}

		SDL_UpdateTexture(texture,
						  NULL,
						  chip->videoBuffer,
						  1);


		SDL_RenderCopy(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);

		end = SDL_GetPerformanceCounter();
		float elapse = (end - start) / freq;

		SDL_Delay(cap - elapse);
	}
	

	


	releaseEmulator(chip);
	SDL_DestroyTexture(texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 1;
}


static char* readFile(const char* filename, uint32_t* bytes){
	FILE* pFile = fopen(filename, "rb");

	if (!pFile)
		return NULL;


	char* buffer = NULL;
	uint32_t length = 0;


	fseek(pFile, 0, SEEK_END);
	length = ftell(pFile);
	rewind(pFile);


	buffer = malloc(sizeof(char) * length);
	*bytes = fread(buffer, sizeof(char), length, pFile);

	fclose(pFile);

	return buffer;
}


static uint8_t getKeyCode(SDL_Scancode scancode){
	uint8_t key = 0xFF;
	
	if (scancode == CHIP8_KEYBOARD_MAP[0]) 
		key = 0x01;
	else if (scancode == CHIP8_KEYBOARD_MAP[1]) 
		key = 0x02;
	else if (scancode == CHIP8_KEYBOARD_MAP[2])
		key = 0x03;
	else if (scancode == CHIP8_KEYBOARD_MAP[3])
		key = 0x0C;
	else if (scancode == CHIP8_KEYBOARD_MAP[4])
		key = 0x04;
	else if (scancode == CHIP8_KEYBOARD_MAP[5])
		key = 0x05;
	else if (scancode == CHIP8_KEYBOARD_MAP[6])
		key = 0x06;
	else if (scancode == CHIP8_KEYBOARD_MAP[7])
		key = 0x0D;
	else if (scancode == CHIP8_KEYBOARD_MAP[8])
		key = 0x07;
	else if (scancode == CHIP8_KEYBOARD_MAP[9])
		key = 0x08;
	else if (scancode == CHIP8_KEYBOARD_MAP[10])
		key = 0x09;
	else if (scancode == CHIP8_KEYBOARD_MAP[11])
		key = 0x0E;
	else if (scancode == CHIP8_KEYBOARD_MAP[12])
		key = 0x0A;
	else if (scancode == CHIP8_KEYBOARD_MAP[13])
		key = 0x00;
	else if (scancode == CHIP8_KEYBOARD_MAP[14])
		key = 0x0B;
	else if (scancode == CHIP8_KEYBOARD_MAP[15])
		key = 0x0F;
	

	return key;
}