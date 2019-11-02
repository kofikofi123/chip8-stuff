#ifndef CHIP8_H
#define CHIP8_H

#include <SDL2/SDL.h>
#include <stdint.h>


struct Chip8 {
	 uint8_t gpRegisters[16];
	 uint16_t I;
	 uint8_t soundRegister;
	 uint8_t delayRegister;
	 uint16_t pc;
	 uint16_t sp;

	 void* memory;
	 uint16_t* stackBase;
	 uint32_t* videoBuffer;

	 uint8_t dispWidth;
	 uint8_t dispHeight;
	 uint8_t dispScale;

	 uint8_t isKeyWait;
	 uint8_t keyboard[16];
	 uint8_t currentKey;

	 uint8_t error;
};


struct Chip8* newEmulator(uint8_t, uint8_t, uint8_t);
void releaseEmulator(struct Chip8*);


void stepEmulator(struct Chip8*);
void loadIntoEmulator(struct Chip8*, void*, uint16_t, uint16_t);
void displayEmulator(struct Chip8*, struct SDL_Texture*);
void dumpEmulator(struct Chip8*);
void debugDisplay(struct Chip8*);


#endif