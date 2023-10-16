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

struct ProjectedPoint {
    float u;
    float v;
    float depth;
};


ProjectedPoint getCanvasIntersectionPoint(const glm::vec3& cameraPosition, const glm::vec3& vertexPosition, float focalLength, float W, float H) {
    // Convert the vertex position to camera coordinates
    float x = vertexPosition.x - cameraPosition.x;
    float y = vertexPosition.y - cameraPosition.y;
    float z = vertexPosition.z - cameraPosition.z;

    // Project the point onto the image plane
    float u = focalLength * (x / z) + W / 2;
    float v = focalLength * (y / z) + H / 2;

    return ProjectedPoint{u, v};
}



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


void drawStrokedTriangle(DrawingWindow &window, CanvasTriangle triangle, Colour colour) {
    drawLine(window, triangle.vertices[0], triangle.vertices[1], colour);
    drawLine(window, triangle.vertices[1], triangle.vertices[2], colour);
    drawLine(window, triangle.vertices[2], triangle.vertices[0], colour);
}

// Use an unordered_map to store colors with their names as keys
std::unordered_map<std::string, Colour> loadMaterialsFile(const std::string& filename) {
    std::unordered_map<std::string, Colour> materials;
    std::ifstream file(filename);
    std::string line;
    std::string currentMaterial;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            ss >> currentMaterial;
        } else if (token == "Kd") {
            float r, g, b;
            ss >> r >> g >> b;
            materials[currentMaterial] = Colour(int(r * 255), int(g * 255), int(b * 255));
        }
    }
    
    return materials;
}
//The operator[] for std::unordered_map is non-const because it can potentially insert a new element into the map if the key is not found. 
std::vector<ModelTriangle> loadGeometryFile(const std::string& filename, std::unordered_map<std::string, Colour>& materials) {
    std::vector<ModelTriangle> triangles;
    std::vector<glm::vec3> vertices;
    std::ifstream file(filename);
    std::string line;
    Colour currentColor;

    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "v") {
            glm::vec3 vertex;
            ss >> vertex.x >> vertex.y >> vertex.z;
            vertices.push_back(vertex);
        } else if (token == "usemtl") {
            std::string materialName;
            ss >> materialName;
            currentColor = materials[materialName];
        } else if (token == "f") {
            // Assuming triangles only for simplicity
            int v1, v2, v3;
            ss >> v1 >> v2 >> v3;
            triangles.push_back(ModelTriangle(vertices[v1-1], vertices[v2-1], vertices[v3-1], currentColor));
        }
    }

    return triangles;
}


std::vector<ModelTriangle> loadOBJ(const std::string& filename, float scalingFactor) {
    std::ifstream inFile(filename);
    std::vector<glm::vec3> vertices;
    std::vector<ModelTriangle> triangles;

    if (!inFile.is_open()) {
        std::cerr << "Error: Unable to open file " << filename << std::endl;
        return triangles;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        //instead of Utils::split
        std::vector<std::string> tokens = split(line, ' ');
        if (tokens.empty()) continue;

        if (tokens[0] == "v") {
            glm::vec3 vertex;
            vertex.x = std::stof(tokens[1]) * scalingFactor;
            vertex.y = std::stof(tokens[2]) * scalingFactor;
            vertex.z = std::stof(tokens[3]) * scalingFactor;
            vertices.push_back(vertex);
        } else if (tokens[0] == "f") {
            Colour color; // Assuming a Colour class is defined somewhere. If it's not, you can remove this line.
            ModelTriangle triangle(vertices[std::stoi(tokens[1]) - 1], 
                                   vertices[std::stoi(tokens[2]) - 1], 
                                   vertices[std::stoi(tokens[3]) - 1], color);
            triangles.push_back(triangle);
        }
    }

    inFile.close();
    return triangles;
}
/*
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
*/

void draw(DrawingWindow &window, std::vector<ModelTriangle> triangles, const glm::vec3& cameraPosition, float focalLength) {
    window.clearPixels();
    
    float scalingFactor = 240.0f;  // Scaling to make the points visible on our canvas
    
    for(const ModelTriangle& triangle : triangles) {
        CanvasTriangle canvasTriangle;
        
        for(int i = 0; i < 3; i++) {
            ProjectedPoint projectedPoint = getCanvasIntersectionPoint(cameraPosition, triangle.vertices[i], focalLength, WIDTH, HEIGHT);
            CanvasPoint point;
            point.x = projectedPoint.u * scalingFactor;
            point.y = projectedPoint.v * scalingFactor;
            point.depth = projectedPoint.depth;
            
            canvasTriangle.vertices[i] = point;
        }
        
        drawStrokedTriangle(window, canvasTriangle, Colour(255, 255, 255));
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
	//std::vector<ModelTriangle> triangles = loadOBJ("/home/kz21093/Downloads/RedNoise/RedNoise/cornell-box.obj", 0.35);
    auto materials = loadMaterialsFile("/home/kz21093/Downloads/RedNoise/RedNoise/cornell-box.mtl");
    auto triangles = loadGeometryFile("/home/kz21093/Downloads/RedNoise/RedNoise/cornell-box.obj", materials);

    // Print the loaded triangles
    for (const auto& triangle : triangles) {
        std::cout << triangle << std::endl;
    }
    glm::vec3 cameraPosition = glm::vec3(0, 0, -4); // Placing the camera at a suitable location
    float focalLength = 1.0f; // Just a starting point, adjust as needed
   
   
    DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	SDL_Event event;
	while (true) {
		// We MUST poll for events - otherwise the window will freeze !
		if (window.pollForInputEvents(event)) handleEvent(event, window);
		// draw(window);
        draw(window, triangles, cameraPosition, focalLength);
		// Need to render the frame at the end, or nothing actually gets shown on the screen !
		window.renderFrame();
	}
    return 0;
}