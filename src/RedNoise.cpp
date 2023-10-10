#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasTriangle.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/DrawingWindow.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Utils.h>
#include <fstream>
#include <vector>

#define WIDTH 320
#define HEIGHT 240

// Function to interpolate grayscale values
std::vector<uint32_t> interpolateGrayscale(int width) {
    std::vector<uint32_t> gradient;

    for (int i = 0; i < width; ++i) {
        // Calculate the grayscale value based on the interpolation
        float grayscale = (static_cast<float>(i) / (width - 1)) * 255.0f;
    
        
        // Pack the grayscale value into a 32-bit integer as RGB
        uint32_t pixelColour = (static_cast<uint32_t>(grayscale) << 16) |
                              (static_cast<uint32_t>(grayscale) << 8) |
                              static_cast<uint32_t>(grayscale);

        gradient.push_back(pixelColour);
    }

    return gradient;
}


void draw(DrawingWindow &window) {
	//Task3: Interpolate grayscale values
    std::vector<uint32_t> gradient = interpolateGrayscale(WIDTH);
	window.clearPixels();
    for (int x = 0; x < window.width; x++) {
        for (int y = 0; y < window.height; y++) {
            uint32_t colour = gradient[x];  // Get the pixel color from the gradient

            // Set the pixel color with alpha set to 255 (fully opaque)
            //rgb & 0xff000000 means that you apply a mask to your rgb color to get the alpha value of it.
			colour |= 0xFF000000;

            // Draw the pixel at the current (x, y) position
            window.setPixelColour(x, y, colour);
        }
    }
	/*
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
	*/
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

/*Task2
std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    std::vector<float> result;
    
    // Calculate the step size
    float step = (to - from) / (numberOfValues - 1);

    // Generate the values and add them to the result vector
    for (int i = 0; i < numberOfValues; ++i) {
        result.push_back(from + i * step);
    }

    return result;
}
*/



int main(int argc, char *argv[]) {

	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	std::vector<uint32_t> gradient = interpolateGrayscale(WIDTH);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
	return 0;
}
