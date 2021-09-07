#include "Chip8.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <netinet/in.h>

std::ostream& operator<<(std::ostream& stream, Chip8& emulator) {
	stream << "[CHIP8 Emulator @ " << std::showbase << &stream << "]\n";
	stream << "Registers: \n";
	for (uint8_t i = 0; i < 16; i++) {
		stream << "V" << std::noshowbase << std::hex << (int)i << ": " << (int)emulator.readRegister(i) << "\n";
	}
	stream << "I: " << (int)emulator.index << "\n";
	stream << "PC: " << (int)emulator.program_counter << "\n";
	stream << "PC (debug): " << (int)emulator.debug_pc << "\n";
	stream << "[END]";
	return stream;
}


Chip8::Chip8(uint16_t size):
	stack_pointer(0),
	sound_timer(0),
	delay_timer(0),
	index(0),
	lock_emulator(0),
	program_counter(0),
	memory_size(size),
	key_wait(false),
	current_key(0xFF),
	screen_refresh(false),
	stack(std::make_unique<uint16_t[]>(64)),
	memory(std::make_unique<uint8_t[]>(size))
{
	std::memset(stack.get(), 0, 64);
	std::memset(memory.get(), 0, size);
	std::memset(registers, 0, 16);
	std::memset(keyboard, 0, 16);
	//std::fill(std::begin(registers), std::end(registers), 0);
	//std::fill(std::begin(keyboard), std::end(keyboard), 0);
}

void Chip8::step() {
	uint8_t* mem = memory.get();

	uint16_t val = htons(*(reinterpret_cast<uint16_t*>(mem + program_counter)));
	uint16_t pc = program_counter;
	program_counter = program_counter + 2;
	debug_pc = pc;

	uint8_t ins = (val >> 12) & 0xFF;

	if (ins == 0x00) { 
		if (val == 0x00E0) {
			clearFramebuffer();
		} else if (val == 0x00EE) {
			program_counter = popStack();
		}
	} else if (ins == 0x01) { 
		program_counter = val & 0x0FFF;
	} else if (ins == 0x02) {
		pushStack(program_counter);
		program_counter = val & 0x0FFF;
	} else if (ins == 0x03) {
		program_counter = (readRegister((val >> 8) & 0x0F) == (val & 0xFF)) ? program_counter + 2 : program_counter;
	} else if (ins == 0x04) {
		program_counter = (readRegister((val >> 8) & 0x0F) == (val & 0xFF)) ? program_counter : program_counter + 2;
	} else if (ins == 0x05) { 
		program_counter = (readRegister((val >> 8) & 0x0F) == readRegister((val >> 4) & 0x0F)) ? program_counter + 2 : program_counter;
	} else if (ins == 0x06) { 
		writeRegister((val >> 8) & 0x0F, val & 0xFF);
	} else if (ins == 0x07) {
		uint8_t r = (val >> 8) & 0x0F;
		writeRegister(r, readRegister(r) + (val & 0xFF));
	} else if (ins == 0x08) {
		uint16_t result;
		uint8_t rx = (val >> 8) & 0x0F;
		uint8_t x = readRegister(rx);
		uint8_t y = readRegister((val >> 4) & 0x0F);
		uint8_t sub = val & 0x0F;


		if (sub == 0) {
			result = y;
		} else if (sub == 1) {
			result = x | y;
		} else if (sub == 2) { 
			result = x & y;
		} else if (sub == 3) {
			result = x ^ y;
		} else if (sub == 4) {
			result = x + y;
			writeRegister(0x0F, (result >> 8) & 0x01);
		} else if (sub == 5) {
			result = x - y;
			writeRegister(0x0F, x > y);
		} else if (sub == 6) {
			writeRegister(0x0F, x & 0x01);
			result = x >> 1;
		} else if (sub == 7) {
			result = y - x;
			writeRegister(0x0F, y > x);
		} else if (sub == 0x0E) {
			writeRegister(0x0F, (x >> 7) & 0x01);
			result = x << 1;
		}
		writeRegister(rx, result);
	} else if (ins == 0x09) {
		program_counter = (readRegister((val >> 8) & 0x0F) == readRegister((val >> 4) & 0x0F)) ? program_counter : program_counter + 2;
	} else if (ins == 0x0A) { 
		index = val & 0x0FFF;
	} else if (ins == 0x0B) { 
		program_counter = readRegister(0) + (val & 0x0FFF);
	} else if (ins == 0x0C) { 
		writeRegister((val >> 8) & 0x0F, (std::rand() % 255) & (val & 0xFF));
	} else if (ins == 0x0D) {
		drawSprite(readRegister((val >> 8) & 0x0F), readRegister((val >> 4) & 0x0F), val & 0x0F);
	} else if (ins == 0x0E) { 
		uint8_t sub = val & 0xFF;
		uint8_t rx = (val >> 8) & 0x0F;

		uint8_t pos = keyboard[rx];
		if (sub == 0x9E) {
			program_counter = pos ? program_counter + 2 : program_counter;
		} else if (sub == 0xA1) {
			program_counter = !pos ? program_counter + 2 : program_counter;
		}
	} else if (ins == 0x0F){
		uint8_t sub = val & 0xFF;
		uint8_t rx = (val >> 8) & 0x0F; 
		uint8_t x = readRegister(rx);

		if (sub == 0x07) {
			writeRegister(rx, delay_timer);
		} else if (sub == 0x0A) {
			if (!key_wait && current_key == 0xFF) {
				key_wait = true;
				program_counter = pc;
			} else {
				writeRegister(rx, current_key);
				current_key = 0xFF;
			}
		} else if (sub == 0x15) { 
			delay_timer = x;
		} else if (sub == 0x18) {
			sound_timer = x;
		} else if (sub == 0x1E) {
			index = index + x;
		} else if (sub == 0x29) {
			index = x * 5;
		} else if (sub == 0x33) {
			writeToMemory(index, (rx / 100) % 10);
			writeToMemory(index + 1, (rx / 10) % 10);
			writeToMemory(index + 2, rx % 10);
		} else if (sub == 0x55) {
			for (uint8_t i = 0; i <= rx; i++) {
				writeToMemory(index++, readRegister(i));
			}
		} else if (sub == 0x65) { 
			for (uint8_t i = 0; i <= rx; i++) {
				writeRegister(i, readFromMemory(index++));
			}
		} else {
			errorString = "Incorrect instruction";
		}
	}
}

void Chip8::reset() {
	std::memset(stack.get(), 0, 64);
	std::memset(memory.get(), 0, memory_size);
	std::memset(registers, 0, 16);
	std::memset(keyboard, 0, 16);

	if (auto f = framebuffer.lock()) {
		std::memset(f.get(), 0, 2048);
	}

	program_counter = 0x200;
	sound_timer = 0;
	stack_pointer = 0;
}

void Chip8::clockTimers() {
	if (delay_timer > 0) {
		delay_timer--;
	}

	if (sound_timer > 0) {
		sound_timer--;
	}
}


bool Chip8::loadIntoMemory(uint8_t* source, std::size_t length, uint32_t location) {
	std::memcpy(memory.get() + location, source, length);
	return true;
}

bool Chip8::loadFileIntoMemory(const std::string& filename, uint32_t location) {
	std::ifstream stream(filename, std::ios::binary | std::ios::ate);

	if (stream.is_open()) {
		std::streampos length = stream.tellg();
		stream.seekg(std::ios_base::beg);

		uint8_t* source = new uint8_t[length];

		if (source != nullptr) {
			stream.read(reinterpret_cast<char*>(source), length);
			stream.close();

			loadIntoMemory(source, length, location);

			delete source;

			return true;
		}
	}

	return false;
}

void Chip8::writeToMemory(uint16_t addr, uint8_t val) {
	*(memory.get() + addr) = val;
}

uint8_t Chip8::readFromMemory(uint16_t addr) {
	return *(memory.get() + addr);
}

void Chip8::clearFramebuffer() {
	if (auto fb = framebuffer.lock()) {
		uint8_t* p = fb.get();
		std::memset(p, 0, 2048);
	}
}

void Chip8::registerKeydown(uint8_t key) { 
	keyboard[key] = 1;
}

void Chip8::registerKeyup(uint8_t key) {
	keyboard[key] = 0;
	current_key = key;
	key_wait = false;

}


void Chip8::drawSprite(uint8_t x, uint8_t y, uint8_t n) {
	if (auto fb = framebuffer.lock()) {
		uint8_t* framebuffer_mem = fb.get();
		uint8_t* mem = memory.get() + index;
		for (uint8_t j = y; j < y + n; j++) {
			uint8_t byte = *mem++;
			for (uint8_t i = 0; i < 8; i++) {
				uint16_t pos = ((j % 32) * 64) + (((x + i) % 64));

				uint8_t* fp = (framebuffer_mem + pos);
				uint8_t bitA = (byte >> (7 - i)) & 0x01;
				uint8_t bitB = (*fp) & 0x01;
				uint8_t result = bitA + bitB;
				writeRegister(0x0F, (result >> 1) & 0x01);
				*fp = (result & 0x01) * 0xFF;
			}
		}
	}
}

void Chip8::pushStack(uint16_t byte) {
	stack[++stack_pointer] = byte;
}
uint16_t Chip8::popStack() {
	return stack[stack_pointer--];
}
bool Chip8::isStackEmpty() {
	return stack_pointer == 0;
}