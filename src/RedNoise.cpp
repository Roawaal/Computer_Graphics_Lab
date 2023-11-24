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


glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, -5.0f); // Position the camera slightly back
glm::vec3 cameraRot = glm::vec3(0.0f, 0.0f, 0.0f); // Rotation about X and Y axes
const float moveSpeed = 0.1f;
const float rotSpeed = 0.01f;

glm::mat3 createRotationMatrix(const glm::vec3& rot) {
    glm::mat3 rotX = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1, 0, 0));
    glm::mat3 rotY = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0, 1, 0));
    return rotY * rotX;
}

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
        if (event.key.keysym.sym == SDLK_LEFT) cameraPos.x -= moveSpeed;
        else if (event.key.keysym.sym == SDLK_RIGHT) cameraPos.x += moveSpeed;
        else if (event.key.keysym.sym == SDLK_UP) cameraPos.y += moveSpeed;
        else if (event.key.keysym.sym == SDLK_DOWN) cameraPos.y -= moveSpeed;
        else if (event.key.keysym.sym == SDLK_w) cameraPos.z += moveSpeed;
        else if (event.key.keysym.sym == SDLK_s) cameraPos.z -= moveSpeed;
        else if (event.key.keysym.sym == SDLK_a) cameraRot.y -= rotSpeed;
        else if (event.key.keysym.sym == SDLK_d) cameraRot.y += rotSpeed;
        else if (event.key.keysym.sym == SDLK_q) cameraRot.x -= rotSpeed;
        else if (event.key.keysym.sym == SDLK_e) cameraRot.x += rotSpeed;
    }
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
}




int main(int argc, char *argv[]) {
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    SDL_Event event;

    while (true) {
        // Poll for events
        if (window.pollForInputEvents(event)) handleEvent(event, window);

        // Apply rotation to camera position
        glm::mat3 rotMatrix = createRotationMatrix(cameraRot);
        glm::vec3 transformedCameraPos = rotMatrix * cameraPos;

        // Draw the scene with the updated camera position
        draw(window, transformedCameraPos);

        // Render the frame
        window.renderFrame();
    }
}
