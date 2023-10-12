#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasTriangle.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/DrawingWindow.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Utils.h>
#include <fstream>
#include <vector>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/glm-0.9.7.2/glm/glm.hpp>


#define WIDTH 320
#define HEIGHT 240

// Task2/3: Add interpolateSingleFloats and modify draw
std::vector<float> interpolateSingleFloats(float from, float to, int numberOfValues) {
    std::vector<float> result;

    // If only one value is required, it should be the starting value
    if (numberOfValues == 1) {
        result.push_back(from);
        return result;
    }

    // Calculate the step value to add for each successive float in the series
    float step = (to - from) / static_cast<float>(numberOfValues - 1);

    for (int i = 0; i < numberOfValues; i++) {
        float value = from + step * i;
        result.push_back(value);
    }

    return result;
}

// Task4/5: Add interpolateThreeElementValues and interpolate2DColors
std::vector<glm::vec3> interpolateThreeElementValues(const glm::vec3 &from, const glm::vec3 &to, int numberOfValues) {
    std::vector<glm::vec3> interpolatedValues;

    for(int i = 0; i < numberOfValues; i++) {
        float factor = float(i) / float(numberOfValues - 1);
        glm::vec3 interpolatedValue = from + factor * (to - from);
        interpolatedValues.push_back(interpolatedValue);
    }

    return interpolatedValues;
}

uint32_t vec3ToPixelColour(const glm::vec3 &color) {
    return (255 << 24) + (int(color.x) << 16) + (int(color.y) << 8) + int(color.z);
}


void interpolate2DColors(DrawingWindow &window) {
    glm::vec3 topLeft(255, 0, 0);        // red 
    glm::vec3 topRight(0, 0, 255);       // blue 
    glm::vec3 bottomRight(0, 255, 0);    // green 
    glm::vec3 bottomLeft(255, 255, 0);   // yellow

    std::vector<glm::vec3> leftColumnColors = interpolateThreeElementValues(topLeft, bottomLeft, window.height);
    std::vector<glm::vec3> rightColumnColors = interpolateThreeElementValues(topRight, bottomRight, window.height);

    for (size_t y = 0; y < window.height; y++) {
        std::vector<glm::vec3> rowColors = interpolateThreeElementValues(leftColumnColors[y], rightColumnColors[y], window.width);

        for (size_t x = 0; x < window.width; x++) {
            window.setPixelColour(x, y, vec3ToPixelColour(rowColors[x]));
        }
    }
}

void draw(DrawingWindow &window) {
    window.clearPixels();

    // Interpolating between 255 (white) and 128 (grey) for the width of the window
    std::vector<float> greyscales = interpolateSingleFloats(255.0, 128.0, window.width);

    for (size_t y = 0; y < window.height; y++) {
        for (size_t x = 0; x < window.width; x++) {
            float grey = greyscales[x];

            // Using the interpolated value as R, G, and B channels to get the gradient
            uint32_t colour = (255 << 24) + (int(grey) << 16) + (int(grey) << 8) + int(grey);
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
    /*
    glm::vec3 from(1.0, 4.0, 9.2);
    glm::vec3 to(4.0, 1.0, 9.8);

    std::vector<glm::vec3> result = interpolateThreeElementValues(from, to, 4);

    for(size_t i = 0; i < result.size(); i++) {
        std::cout << "(" << result[i].x << ", " << result[i].y << ", " << result[i].z << ")" << std::endl;
    }  
    */
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    interpolate2DColors(window);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		draw(window);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
}