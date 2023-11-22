#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasTriangle.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/DrawingWindow.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Utils.h>
#include <fstream>
#include <vector>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/glm-0.9.7.2/glm/glm.hpp>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Colour.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasPoint.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/TextureMap.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/ModelTriangle.h>
#include <sstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasPoint.h>

#define WIDTH 320
#define HEIGHT 240

void draw(DrawingWindow &window) {
	window.clearPixels();
	for (size_t y = 0; y < window.height; y++) {
		for (size_t x = 0; x < window.width; x++) {
			float red = rand() % 256;
			float green = 0.0;
			float blue = 0.0;
			uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
			window.setPixelColour(x, y, colour);
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm");
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}
