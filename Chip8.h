#ifndef CHIP8_H
#define CHIP8_H

#include <memory>
#include <cstdint>
#include <ostream>
#include <forward_list>


class Chip8 {
	friend std::ostream& operator<<(std::ostream&, Chip8&);

//registers
	uint8_t registers[16];
	uint8_t keyboard[16];

	uint8_t stack_pointer;
	uint8_t sound_timer;
	uint8_t delay_timer;
	uint16_t index;

	uint8_t lock_emulator;
	uint16_t program_counter;
	uint16_t memory_size;

	uint16_t debug_pc;


	bool key_wait;
	uint8_t current_key;
	bool screen_refresh;


	std::string errorString;

////////////////////////////////////////////////
	std::unique_ptr<uint16_t[]> stack;
	std::unique_ptr<uint8_t[]> memory;
	std::weak_ptr<uint8_t[]> framebuffer;

	std::forward_list<uint16_t> update_list;
///////////////////////////////////////////////
	void writeToMemory(uint16_t, uint8_t);
	uint8_t readFromMemory(uint16_t);


	bool isFramebufferReady();
	void drawSprite(uint8_t, uint8_t, uint8_t);

	void clearFramebuffer();

	void pushStack(uint16_t);
	uint16_t popStack();
	bool isStackEmpty();

	uint8_t readRegister(uint8_t n) { return registers[n]; }
	void writeRegister(uint8_t n, uint8_t val) { registers[n] = val; }
public:
	Chip8(): Chip8(4096){}
	Chip8(uint16_t);
	Chip8(const Chip8& other) {}


	void setPC(uint16_t pc) { program_counter = pc; }
	uint16_t getPC() { return program_counter; }


	void step();
	void reset();

	bool loadIntoMemory(uint8_t*, std::size_t, uint32_t);
	bool loadFileIntoMemory(const std::string&, uint32_t);
	void setupFramebuffer(std::shared_ptr<uint8_t[]>& fb) { framebuffer = fb; }


	std::forward_list<uint16_t>::iterator updateBegin() { return update_list.begin(); }
	std::forward_list<uint16_t>::iterator updateEnd() { return update_list.end(); }
	bool framebufferInvalidatePending() { bool result = screen_refresh; screen_refresh = false; return result; }

	void registerKeydown(uint8_t);
	void registerKeyup(uint8_t);
	void invalidateFramebuffer() { update_list.clear(); }

	bool isKeyWait() { return key_wait; }
	void clockTimers();

	bool error() { return !errorString.empty(); }
	std::string getError() { return errorString; }
};



#endif