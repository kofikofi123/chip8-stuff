#include "App.h"
#include <iostream>


int main(int argc, char* argv[]){
	App app;

	if (argc > 1) {
		app.resetEmulator(argv[1]);
	}

	app.loop();

	if (app.error()) {
		std::cout << app.getError() << std::endl;
	}
}