#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <assert.h>
#include "Chip8.h"
#include "Chip8Renderer.h"


static void writeMemory8(struct Chip8*, uint16_t, uint8_t);
static uint8_t readMemory8(struct Chip8*, uint16_t);
static uint16_t readMemory16(struct Chip8*, uint16_t);

static uint8_t readRegister(struct Chip8*, uint8_t);
static void writeRegister(struct Chip8*, uint8_t, uint8_t);

static uint16_t popStack(struct Chip8*);
static void pushStack(struct Chip8*, uint16_t);

static void drawSprite(struct Chip8*, uint8_t, uint8_t, uint8_t);



//////////////////////////////////////////////////////////////////////////////
struct Chip8* newEmulator(uint8_t dispW, uint8_t dispH, uint8_t scale){
	uint32_t width = dispW * scale;
	uint32_t height = dispH * scale;
	uint32_t bufferSize = (sizeof(uint32_t) * width * height);


	struct Chip8* c8 = malloc(sizeof(struct Chip8));
	void* memory = malloc(sizeof(char) * 0x1000);
	void* videoBuffer = malloc(bufferSize);

	if (!c8 || !memory){
		free(videoBuffer);
		free(c8);
		free(memory);
	}else{
		memset(c8, 0, sizeof(struct Chip8));
		memset(memory, 0, sizeof(char) * 0x1000);
		memset(videoBuffer, 0, bufferSize);

		c8->memory = memory;
		c8->stackBase = memory + 0xEA0;
		c8->videoBuffer = videoBuffer;

		c8->dispWidth = dispW;
		c8->dispHeight = dispH;
		c8->dispScale = scale;
		c8->currentKey = 0xFF;
	}


	return c8;
}

void releaseEmulator(struct Chip8* chip){
	free(chip->videoBuffer);
	free(chip->memory);
	free(chip);
}

void stepEmulator(struct Chip8* chip8){
	uint16_t oldPC = chip8->pc;
	uint16_t inst = readMemory16(chip8, oldPC);

	uint8_t opcode = inst >> 12;

	chip8->pc = oldPC + 2;

	uint8_t scale = chip8->dispScale;
	uint32_t width = chip8->dispWidth * scale;
	uint32_t height = chip8->dispHeight * scale;



	switch (opcode){
		case 0x00: {
			uint8_t subop = inst & 0xFF;

			if (subop == 0xE0){
				memset(chip8->videoBuffer, 0x00, sizeof(uint32_t) * width * height);
			}else if (subop == 0xEE)
				chip8->pc = popStack(chip8);
			else{
				printf("Unknown instruction: %#x at %x\n", inst, chip8->pc - 2);
				chip8->error = 1;
			}

			break;
		}
		case 0x01: {
			chip8->pc = inst & 0x0FFF;
			break;
		}
		case 0x02: {
			pushStack(chip8, chip8->pc);
			chip8->pc = inst & 0x0FFF;
			break;
		}
		case 0x03: {
			if (readRegister(chip8, ((inst >> 8) & 0x0F)) == (inst & 0xFF))
				chip8->pc += 2;
			break;
		}
		case 0x04: {
			if (readRegister(chip8, ((inst >> 8) & 0x0F)) != (inst & 0xFF))
				chip8->pc += 2;
			break;
		}
		case 0x05: {
			if (readRegister(chip8, ((inst >> 8) & 0x0F)) == readRegister(chip8, ((inst >> 4) & 0x0F)))
				chip8->pc += 2;
			break;
		}
		case 0x06: {
			writeRegister(chip8, ((inst >> 8) & 0x0F), (inst & 0xFF));
			break;
		}
		case 0x07: {
			uint8_t x = (inst >> 8) & 0x0F;
			writeRegister(chip8, x, readRegister(chip8, x) + (inst & 0xFF));
			break;
		}
		case 0x08: {
			uint8_t subop = inst & 0x0F;

			uint8_t regX = (inst >> 8) & 0x0F;

			uint8_t cregX = readRegister(chip8, (inst >> 8) & 0x0F);
			uint8_t cregY = readRegister(chip8, (inst >> 4) & 0x0F);

			if (subop == 0)
				writeRegister(chip8, regX, cregY);
			else if (subop == 1)
				writeRegister(chip8, regX, cregX | cregY);
			else if (subop == 2)
				writeRegister(chip8, regX, cregX & cregY);
			else if (subop == 3)
				writeRegister(chip8, regX, cregX ^ cregY);
			else if (subop == 4){
				uint16_t result = cregX + cregY;
				writeRegister(chip8, regX, result & 0xFF);
				writeRegister(chip8, 0x0F, (result > 255));
			}
			else if (subop == 5){
				writeRegister(chip8, regX, cregX - cregY);
				writeRegister(chip8, 0x0F, cregX > cregY);
			}
			else if (subop == 6){
				writeRegister(chip8, 0x0F, cregX & 0x01);
				writeRegister(chip8, regX, cregX >> 1);
			}else if (subop == 7){
				writeRegister(chip8, regX, cregX - cregY);
				writeRegister(chip8, 0x0F, cregY > cregX);
			}else if (subop == 0xE){
				writeRegister(chip8, 0x0F, cregX >> 7);
				writeRegister(chip8, regX, cregX << 1);
			}else{
				printf("Unknown instruction %x\n", inst);
				chip8->error = 1;
			}

			break;
		}
		case 0x09: {
			if (readRegister(chip8, ((inst >> 8) & 0x0F)) != readRegister(chip8, ((inst >> 4) & 0x0F)))
				chip8->pc += 2;
			break;
		}
		case 0x0A:
			chip8->I = inst & 0x0FFF;
			break;
		case 0x0B:
			chip8->pc = (inst & 0x0FFF) + readRegister(chip8, 0);
			break;
		case 0x0C:
			writeRegister(chip8, ((inst >> 8) & 0x0F), rand() & inst & 0xFF);
			break;
		case 0x0D: {
			drawSprite(chip8, readRegister(chip8, (inst >> 8) & 0x0F), readRegister(chip8, (inst >> 4) & 0x0F), inst & 0x0F);
			break;
		}
		case 0x0E: {
			uint8_t subop = inst & 0xFF;
			uint8_t regX = ((inst >> 8) & 0x0F);
			
			if (subop == 0x9E){
				if (chip8->keyboard[regX]){
					chip8->pc += 2;
				}
			}else if (subop == 0xA1){
				if (!chip8->keyboard[regX]){
					chip8->pc += 2;
				}
			}else{
				printf("Unknown instruction of 0x0E opcode: %x\n", subop);
				chip8->error = 1;
			}

			break;
		}
		case 0x0F: {
			uint8_t subop = inst & 0xFF;
			uint8_t regX = ((inst >> 8) & 0x0F);
			uint8_t cregX = readRegister(chip8, regX);

			if (subop == 0x07)
				writeRegister(chip8, regX, chip8->delayRegister);
			else if (subop == 0x0A){
				uint8_t keypressed = chip8->currentKey;
				if (keypressed == 0xFF){
					chip8->isKeyWait = 1;
					chip8->pc = oldPC;
				}else{
					writeRegister(chip8, regX, keypressed);
					chip8->currentKey = 0xFF;
				}
			}else if (subop == 0x15){
				chip8->delayRegister = cregX;
			}
			else if (subop == 0x18)
				chip8->soundRegister = cregX;
			else if (subop == 0x1E)
				chip8->I += cregX;
			else if (subop == 0x29)
				chip8->I = readRegister(chip8, (inst >> 8) & 0x0F) * 5; 
			else if (subop == 0x33){
				uint16_t I = chip8->I;
				writeMemory8(chip8, I, (regX / 100) % 10);
				writeMemory8(chip8, I + 1, (regX / 10) % 10);
				writeMemory8(chip8, I + 2, regX % 10);
			}else if (subop == 0x55){
				uint16_t tI = chip8->I;

				for (uint8_t i = 0; i <= regX; i++){
					writeMemory8(chip8, tI++, readRegister(chip8, i));
				}
			}else if (subop == 0x65){
				uint16_t tI = chip8->I;

				for (uint8_t i = 0; i <= regX; i++){
					writeRegister(chip8, i, readMemory8(chip8, tI++));
				}
			}else{
				printf("Unknown instruction: %x\n", inst);
			}
		}

		break;
	}
}


void dumpEmulator(struct Chip8* chip8){
	printf("\nGeneral Purpose Registers:\n");

	for (uint8_t i = 0; i < 16; i++){
		printf("Reg(%X): %#x\n", i, readRegister(chip8, i));
	}

	printf("\nProgram Pointer = %#x\nStack Pointer = %#x\nSound Register = %#x\nDelay Register = %#x\nI = %#x\n", 
		   chip8->pc, chip8->sp, chip8->soundRegister, chip8->delayRegister, chip8->I);
}

static void drawSprite(struct Chip8* chip8, uint8_t x, uint8_t y, uint8_t n){
	uint32_t width = chip8->dispWidth;
	uint32_t height = chip8->dispHeight;
	uint32_t scale = chip8->dispScale;

	uint32_t width_scale = width * scale;

	uint32_t* videoBuffer = chip8->videoBuffer;
	uint8_t* memory = chip8->memory + chip8->I;

	writeRegister(chip8, 0x0F, 0);

	for (uint16_t j = y; j < y + n; j++){
		uint8_t byte = *memory++;
		for (uint16_t i = 0; i < 8; i++){


			uint32_t position = ((j % height) * width_scale * scale) + (((x + i) % width) * scale);
			uint32_t* temp = (videoBuffer + position);

			uint8_t pixelA = ((byte >> (7 - i)) & 0x01) * 0xFF;
			uint8_t pixelB = *(uint8_t*)temp;
			uint32_t pixelF = pixelA ^ pixelB;

			if (pixelA && pixelB)
				writeRegister(chip8, 0x0F, 1);
			
			for (uint8_t k = 0; k < scale; k++){
				memset(temp, pixelF, sizeof(uint32_t) * scale);
				temp = temp + width_scale;
			}
			
		}
	}
}


void debugDisplay(struct Chip8* chip){
	uint32_t* videoBuffer = chip->videoBuffer;
	uint8_t width = chip->dispWidth;
	uint8_t height = chip->dispHeight;
	printf("    ");
	for (uint8_t x = 0; x < width; x++){
		printf("%.2d ", x);
	}
	printf("\n    ");
	for (uint8_t x = 0; x < width; x++){
		printf("---");
	}
	printf("\n");
	for (uint8_t y = 0; y < height; y++){
		printf("%.2d| ", y);
		for (uint8_t x = 0; x < width; x++){
			uint32_t p = ((y % height) * width) + (x % width);
			
			printf("%x  ", (*(videoBuffer + p)) & 0x01);
		}
		printf("\n");
	}
}

inline void loadIntoEmulator(struct Chip8* chip, void* data, uint16_t size, uint16_t addr){
	memcpy(chip->memory + addr, data, size);
}

static uint16_t readMemory16(struct Chip8* chip8, uint16_t addr){
	return htons(*(uint16_t*)(chip8->memory + addr));
}

static uint8_t readMemory8(struct Chip8* chip8, uint16_t addr){
	return *(uint8_t*)(chip8->memory + addr);
}

static void writeMemory8(struct Chip8* chip8, uint16_t addr, uint8_t val){
	*(uint8_t*)(chip8->memory + addr) = val;
}

static inline uint8_t readRegister(struct Chip8* chip8, uint8_t regN){
	return chip8->gpRegisters[regN];
}

static void writeRegister(struct Chip8* chip8, uint8_t regN, uint8_t val){
	chip8->gpRegisters[regN] = val;
}

static uint16_t popStack(struct Chip8* chip8){
	uint8_t sp = chip8->sp;
	uint16_t value = chip8->stackBase[sp];
	chip8->sp--;

	return value;
}

static void pushStack(struct Chip8* chip8, uint16_t value){
	uint8_t sp = ++chip8->sp;
	chip8->stackBase[sp] = value;
}