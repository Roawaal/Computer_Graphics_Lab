#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasTriangle.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/DrawingWindow.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Utils.h>
#include <fstream>
#include <vector>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/glm-0.9.7.2/glm/glm.hpp>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/Colour.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/CanvasPoint.h>
#include </home/kz21093/Downloads/RedNoise/RedNoise/libs/sdw/TextureMap.h>


#define WIDTH 320
#define HEIGHT 240


// Task2
std::vector<float> interpolate(float from, float to, int numberOfValues) {
    std::vector<float> values;
    for (int i = 0; i < numberOfValues; i++) {
        float value = from + (i * ((to - from) / float(numberOfValues - 1)));
        values.push_back(value);
    }
    return values;
}

void drawLine(DrawingWindow &window, const CanvasPoint &from, const CanvasPoint &to, const Colour &colour) {
    float length = std::sqrt((to.x - from.x) * (to.x - from.x) + (to.y - from.y) * (to.y - from.y));
    std::vector<float> xValues = interpolate(from.x, to.x, length);
    std::vector<float> yValues = interpolate(from.y, to.y, length);

    for (int i = 0; i < length; i++) {
        uint32_t color = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
        window.setPixelColour(xValues[i], yValues[i], color);
    }
}

// Task3
void drawTriangle(DrawingWindow &window, CanvasTriangle &triangle, const Colour &colour) {
    drawLine(window, triangle.v0(), triangle.v1(), colour);
    drawLine(window, triangle.v1(), triangle.v2(), colour);
    drawLine(window, triangle.v2(), triangle.v0(), colour);
}

void sortVerticesByY(CanvasPoint &p0, CanvasPoint &p1, CanvasPoint &p2) {
    if(p0.y > p1.y) std::swap(p0, p1);
    if(p0.y > p2.y) std::swap(p0, p2);
    if(p1.y > p2.y) std::swap(p1, p2);
}

// Task4

void drawFilledTriangle(DrawingWindow &window, CanvasTriangle &triangle, const Colour &colour) {
    CanvasPoint v0 = triangle.v0();
    CanvasPoint v1 = triangle.v1();
    CanvasPoint v2 = triangle.v2();

    sortVerticesByY(v0, v1, v2);

    // For flat-bottom triangle
    for (float y = v0.y; y <= v1.y; y++) {
        float x0 = v0.x + ((y - v0.y) * (v1.x - v0.x)) / (v1.y - v0.y);
        float x1 = v0.x + ((y - v0.y) * (v2.x - v0.x)) / (v2.y - v0.y);
        if (x1 < x0) std::swap(x0, x1); // make sure x0 is left of x1
        for (float x = x0; x <= x1; x++) {
            uint32_t packedColor = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
            window.setPixelColour(std::round(x), std::round(y), packedColor);
    }
    }
    // For flat-top triangle
    for (float y = v1.y; y <= v2.y; y++) {
        float x0 = v1.x + ((y - v1.y) * (v2.x - v1.x)) / (v2.y - v1.y);
        float x1 = v0.x + ((y - v0.y) * (v2.x - v0.x)) / (v2.y - v0.y);
        if (x1 < x0) std::swap(x0, x1); // make sure x0 is left of x1
        for (float x = x0; x <= x1; x++) {
            uint32_t packedColor = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
            window.setPixelColour(std::round(x), std::round(y), packedColor);
        }
    }

    // Draw a white stroked triangle over the filled triangle
    Colour white(255, 255, 255);
    drawTriangle(window, triangle, white);
}

// Task5

TextureMap texture("/home/kz21093/Downloads/RedNoise/RedNoise/texture.ppm");

TexturePoint addTexturePoints(const TexturePoint &tp1, const TexturePoint &tp2) {
    return TexturePoint(tp1.x + tp2.x, tp1.y + tp2.y);
}

TexturePoint subtractTexturePoints(const TexturePoint &tp1, const TexturePoint &tp2) {
    return TexturePoint(tp1.x - tp2.x, tp1.y - tp2.y);
}

TexturePoint multiplyTexturePointByScalar(const TexturePoint &tp, float scalar) {
    return TexturePoint(tp.x * scalar, tp.y * scalar);
}

void drawTexturedTriangle(DrawingWindow &window, CanvasTriangle &triangle, TextureMap &texture) {
    // Note: This method assumes that the CanvasTriangle vertices are sorted by y value.

    // For brevity, I'm omitting the filling logic that we've done previously.
    // Instead, I'll focus on mapping the texture to the triangle.

    for (int y = triangle.v0().y; y < triangle.v2().y; y++) {
        // Linearly interpolate to find start and end x values (and their corresponding texture points)
        float alpha1 = (y - triangle.v0().y) / (triangle.v2().y - triangle.v0().y);
        float alpha2 = (y - triangle.v0().y) / (triangle.v1().y - triangle.v0().y);

        int x1 = triangle.v0().x + alpha1 * (triangle.v2().x - triangle.v0().x);
        int x2 = triangle.v0().x + alpha2 * (triangle.v1().x - triangle.v0().x);

        TexturePoint diff1 = subtractTexturePoints(triangle.v2().texturePoint, triangle.v0().texturePoint);
        TexturePoint tp1 = addTexturePoints(triangle.v0().texturePoint, multiplyTexturePointByScalar(diff1, alpha1));

        TexturePoint diff2 = subtractTexturePoints(triangle.v1().texturePoint, triangle.v0().texturePoint);
        TexturePoint tp2 = addTexturePoints(triangle.v0().texturePoint, multiplyTexturePointByScalar(diff2, alpha2));
        for (int x = x1; x < x2; x++) {
            // Interpolate the texture point
            float beta = (x - x1) / (float)(x2 - x1);
            TexturePoint texDiff = subtractTexturePoints(tp2, tp1);
            TexturePoint texPoint = addTexturePoints(tp1, multiplyTexturePointByScalar(texDiff, beta));

            // Sample the texture
            int textureIndex = texPoint.y * texture.width + texPoint.x;
            uint32_t sampledColour = texture.pixels[textureIndex];

            // Set the pixel on the window
            window.setPixelColour(x, y, sampledColour);
        }
    }
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
        if (event.key.keysym.sym == SDLK_u) {
            // Generate random triangle and colour for triangle drawing
            CanvasPoint p0(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p1(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p2(rand()%WIDTH, rand()%HEIGHT);
            CanvasTriangle randomTriangle(p0, p1, p2);

            Colour randomColour(rand()%256, rand()%256, rand()%256);
            drawTriangle(window, randomTriangle, randomColour);
        }
        else if (event.key.keysym.sym == SDLK_f) {
            // Generate random triangle and colour for filled triangle drawing
            CanvasPoint p0(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p1(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p2(rand()%WIDTH, rand()%HEIGHT);
            CanvasTriangle randomTriangle(p0, p1, p2);

            Colour randomColour(rand()%256, rand()%256, rand()%256);
            drawFilledTriangle(window, randomTriangle, randomColour);
        }else if (event.key.keysym.sym == SDLK_t) {
            // Generate random triangle
            CanvasPoint p0(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p1(rand()%WIDTH, rand()%HEIGHT);
            CanvasPoint p2(rand()%WIDTH, rand()%HEIGHT);
            CanvasTriangle randomTriangle(p0, p1, p2);

            // Link CanvasPoints to TexturePoints if needed
            drawTexturedTriangle(window, randomTriangle, texture);
        }   
        else if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
        else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
        else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
        else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
    } 
    else if (event.type == SDL_MOUSEBUTTONDOWN) {
        window.savePPM("output.ppm");
        window.saveBMP("output.bmp");
    }
}


int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
    window.clearPixels();
    // Test: Draw a textured triangle
    CanvasPoint p1(160, 10);
    p1.texturePoint = TexturePoint(195, 5);

    CanvasPoint p2(300, 230);
    p2.texturePoint = TexturePoint(395, 380);

    CanvasPoint p3(10, 150);
    p3.texturePoint = TexturePoint(65, 330);

    CanvasTriangle triangle(p1, p2, p3);
    TextureMap texture("/home/kz21093/Downloads/RedNoise/RedNoise/texture.ppm");
    drawTexturedTriangle(window, triangle, texture);

    SDL_Event event;
    while (true) {
        // We MUST poll for events - otherwise the window will freeze!
        if (window.pollForInputEvents(event)) handleEvent(event, window);
        // You may want to invoke some other drawing function here if needed
        window.renderFrame();
    }
	return 0;
}