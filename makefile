GCC := g++
FLAGS := -Wall -std=c++17
LIBS := `sdl2-config --cflags --libs`



a.out: *.h *.cpp
	@$(GCC) $(FLAGS) *.cpp $(LIBS) -o a.out



clean:
	@rm -rf *.o
	@rm a.out

.PHONY: clean