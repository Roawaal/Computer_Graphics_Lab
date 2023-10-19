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

#define SCALING_FACTOR 0.35
std::vector<ModelTriangle> triangles;

std::vector<ModelTriangle> readOBJ(const std::string &filename, float scalingFactor, const std::unordered_map<std::string, Colour> &materials) {
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;
    std::ifstream file(filename);

    Colour currentColour;

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << filename << std::endl;
        return triangles;
    }

    std::string line;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;
        if (type == "usemtl") {
            std::string materialName;
            ss >> materialName;

            currentColour = materials.at(materialName);
        } else if (type == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;

            // Scaling the vertex
            vertex *= scalingFactor;

            vertices.push_back(vertex);
        } else if (type == "f") {
            ModelTriangle triangle;
            int vertexIndex;

            for (int i = 0; i < 3; i++) {
                ss >> vertexIndex;
                triangle.vertices[i] = vertices[vertexIndex - 1]; // Adjusting for 1-based index
            }
            triangle.colour = currentColour;
            triangles.push_back(triangle);
        }
    }
    return triangles;
}


std::unordered_map<std::string, Colour> readMTL(const std::string &filename) {
    std::unordered_map<std::string, Colour> materials;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << filename << std::endl;
        return materials;
    }

    std::string line;
    std::string currentMaterialName;
    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "newmtl") {
            ss >> currentMaterialName;
        } else if (type == "Kd") {
            float r, g, b;
            ss >> r >> g >> b;

            Colour colour(int(r * 255), int(g * 255), int(b * 255));
            materials[currentMaterialName] = colour;
        }
    }

    return materials;
}


// adjust the cameraPosition, focalLength, or the scaling factor as necessary to achieve the desired view of the Cornell Box.
glm::vec3 cameraPosition = glm::vec3(0.0f, 0.0f, 5.0f); 
// Set your camera position
float focalLength = 1.0f;
// Set your focal length


CanvasPoint getCanvasIntersectionPoint(glm::vec3 cameraPosition, glm::vec3 vertexPosition, float focalLength) {
    // Transpose the vertex to the camera coordinate system
    glm::vec3 relativePosition = vertexPosition - cameraPosition;

    // Project onto the image plane
    float u = (focalLength * relativePosition.x / relativePosition.z) + WIDTH / 2;
    float v = (focalLength * relativePosition.y / relativePosition.z) + HEIGHT / 2;

    return CanvasPoint(u, v);
}

void renderPointCloud(DrawingWindow &window) {
    for (const ModelTriangle &triangle : triangles) {
        for (const glm::vec3 &vertex : triangle.vertices) {
            // Project the 3D vertex to 2D
            CanvasPoint projectedPoint = getCanvasIntersectionPoint(cameraPosition, vertex, focalLength);
            projectedPoint.x *= 240;
            projectedPoint.y *= 240;
            // Scale the projected point
            //projectedPoint.x *= SCALING_FACTOR * HEIGHT;  
            //projectedPoint.y *= SCALING_FACTOR * HEIGHT;

            // Print the coordinates for debugging
            std::cout << "Projected Point: (" << projectedPoint.x << ", " << projectedPoint.y << ")" << std::endl;

            // Draw the point (in white color) without window bounds check
            uint32_t colour = (255 << 24) + (255 << 16) + (255 << 8) + 255;  // White color
            window.setPixelColour(projectedPoint.x, projectedPoint.y, colour);
        }
    }
}





// Debug
/*void draw(DrawingWindow &window) {
    window.clearPixels();

    // Image plane scaling factor
    float imagePlaneScaling = 240.0f;

    for (const auto &triangle : triangles) {
        for (const glm::vec3 &vertex : triangle.vertices) {
            std::cout << "Original Vertex: " << vertex.x << ", " << vertex.y << ", " << vertex.z << std::endl;

            CanvasPoint projectedPoint = getCanvasIntersectionPoint(cameraPosition, vertex, focalLength);

            std::cout << "Projected Point (before scaling): " << projectedPoint.x << ", " << projectedPoint.y << std::endl;
            
            // Apply the image plane scaling
            projectedPoint.x *= imagePlaneScaling;
            projectedPoint.y *= imagePlaneScaling;

            std::cout << "Projected Point (after scaling): " << projectedPoint.x << ", " << projectedPoint.y << std::endl;
            
            if (projectedPoint.x >= 0 && projectedPoint.x < window.width && projectedPoint.y >= 0 && projectedPoint.y < window.height) {
                uint32_t colour = (255 << 24) + (255 << 16) + (255 << 8) + 255;  // White color
                window.setPixelColour(projectedPoint.x, projectedPoint.y, colour);
            }
        }
    }
}
*/




void draw(DrawingWindow & window) {
    window.clearPixels();
    renderPointCloud(window);  // This will render the point cloud on the DrawingWindow
}


/*
void draw(DrawingWindow & window)
{
        window.clearPixels();
        for (size_t y = 0; y < window.height; y++)
        {
            for (size_t x = 0; x < window.width; x++)
            {
                float red = rand() % 256;
                float green = 0.0;
                float blue = 0.0;
                uint32_t colour = (255 << 24) + (int(red) << 16) + (int(green) << 8) + int(blue);
                window.setPixelColour(x, y, colour);
            }
        }
}
*/
void handleEvent(SDL_Event event, DrawingWindow & window)
{
        if (event.type == SDL_KEYDOWN)
        {
            if (event.key.keysym.sym == SDLK_LEFT)
                std::cout << "LEFT" << std::endl;
            else if (event.key.keysym.sym == SDLK_RIGHT)
                std::cout << "RIGHT" << std::endl;
            else if (event.key.keysym.sym == SDLK_UP)
                std::cout << "UP" << std::endl;
            else if (event.key.keysym.sym == SDLK_DOWN)
                std::cout << "DOWN" << std::endl;
        }
        else if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            window.savePPM("output.ppm");
            window.saveBMP("output.bmp");
        }
}

    int main(int argc, char *argv[])
    {
        DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
        SDL_Event event;
        auto materials = readMTL("/home/kz21093/Downloads/RedNoise/RedNoise/cornell-box.mtl");
        triangles = readOBJ("/home/kz21093/Downloads/RedNoise/RedNoise/cornell-box.obj", SCALING_FACTOR, materials);
        for (const auto &triangle : triangles)
        {
            std::cout << triangle << std::endl;
        }
        while (true)
        {
            // We MUST poll for events - otherwise the window will freeze !
            if (window.pollForInputEvents(event))
                handleEvent(event, window);
            draw(window);
            // Need to render the frame at the end, or nothing actually gets shown on the screen !
            window.renderFrame();
        }
    }