#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <ModelTriangle.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <Utils.h>
#include <RayTriangleIntersection.h>

#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>


#define WIDTH 1080
#define HEIGHT 1080

#define PI 3.14159265358979323846


std::vector<Colour> loadMtl(const std::string& filename) {
    std::vector<Colour> colours;

    std::ifstream inputStream(filename);
    std::string nextLine;
    std::string name;

    while (std::getline(inputStream, nextLine)) {
        auto line = split(nextLine, ' ');

        if (line[0] == "newmtl") {
            name = line[1];
        } else if (line[0] == "Kd") {
            colours.push_back(Colour(
                name,
                static_cast<int>(std::stof(line[1]) * 255),
                static_cast<int>(std::stof(line[2]) * 255),
                static_cast<int>(std::stof(line[3]) * 255)
            ));
        }
    }

    return colours;
}


std::vector<ModelTriangle> loadObj(const std::string& filename, float scale) {
    std::vector<glm::vec4> vertices;
    std::vector<TexturePoint> textureVertices;
    std::vector<ModelTriangle> models;
    std::vector<Colour> materials;
    Colour colour(255, 255, 255);

    std::ifstream inputStream(filename);
    std::string nextLine;

    while (std::getline(inputStream, nextLine)) {
        auto vector = split(nextLine, ' ');

        if (vector[0] == "mtllib") {
            materials = loadMtl(vector[1]);
        } else if (vector[0] == "usemtl") {
            for (const auto& material : materials) {
                if (material.name == vector[1]) {
                    colour = material;
                    break;
                }
            }
        } else if (vector[0] == "v") {
            vertices.push_back(glm::vec4(
                std::stof(vector[1]) * scale,
                std::stof(vector[2]) * scale,
                std::stof(vector[3]) * scale,
                1.0f
            ));
        } else if (vector[0] == "vt") {
            textureVertices.push_back(TexturePoint(
                static_cast<int>(std::stof(vector[1]) * 480),
                static_cast<int>(std::stof(vector[2]) * 395)
            ));
        } else if (vector[0] == "f") {
            auto v1 = split(vector[1], '/');
            auto v2 = split(vector[2], '/');
            auto v3 = split(vector[3], '/');
            auto triangle = ModelTriangle(
                vertices[std::stoi(v1[0]) - 1],
                vertices[std::stoi(v2[0]) - 1],
                vertices[std::stoi(v3[0]) - 1],
                colour
            );

            if (!v1[1].empty()) {
                triangle.texturePoints[0] = textureVertices[std::stoi(v1[1]) - 1];
                triangle.texturePoints[1] = textureVertices[std::stoi(v2[1]) - 1];
                triangle.texturePoints[2] = textureVertices[std::stoi(v3[1]) - 1];
            }

            triangle.normal = glm::normalize(glm::cross(glm::vec3(triangle.vertices[1] - triangle.vertices[0]), glm::vec3(triangle.vertices[2] - triangle.vertices[0])));
            models.push_back(triangle);
        }
    }

    return models;
}


float getNumberOfSteps(CanvasPoint from, CanvasPoint to) {
	return fmax(fmax(abs(to.x - from.x), abs(to.y - from.y)), 1);
}

void sortTriangleVertices(CanvasTriangle &triangle) {
	if (triangle[0].y > triangle[1].y) {
		std::swap(triangle[0], triangle[1]);
	}
	if (triangle[1].y > triangle[2].y) {
		std::swap(triangle[1], triangle[2]);
		if (triangle[0].y > triangle[1].y) {
			std::swap(triangle[0], triangle[1]);
		}	
	}

}

float interpolateComponent(float from, float to, int steps, int currentStep) {
    float stepValue = (to - from) / (steps - 1);
    return from + currentStep * stepValue;
}
template <typename T, typename U>
std::vector<T> interpolateGeneric(T from, T to, int steps, U xFunc, U yFunc) {
    std::vector<T> points;
    float xStep = (xFunc(to) - xFunc(from)) / (steps - 1);
    float yStep = (yFunc(to) - yFunc(from)) / (steps - 1);

    for (int i = 0; i < steps; i++) {
        points.push_back(T(xFunc(from) + i * xStep, yFunc(from) + i * yStep));
    }

    return points;
}

std::vector<TexturePoint> interpolatePoints(TexturePoint from, TexturePoint to, int steps) {
    std::vector<TexturePoint> texturePoints;
    for (int i = 0; i < steps; i++) {
        float x = interpolateComponent(from.x, to.x, steps, i);
        float y = interpolateComponent(from.y, to.y, steps, i);
        texturePoints.push_back(TexturePoint(round(x), round(y)));
    }
    return texturePoints;
}

std::vector<CanvasPoint> interpolatePoints(CanvasPoint from, CanvasPoint to, int steps) {
    std::vector<CanvasPoint> canvasPoints;
    auto texturePoints = interpolatePoints(from.texturePoint, to.texturePoint, steps);

    for (int i = 0; i < steps; i++) {
        float x = interpolateComponent(from.x, to.x, steps, i);
        float y = interpolateComponent(from.y, to.y, steps, i);
        float depth = interpolateComponent(from.depth, to.depth, steps, i);

        CanvasPoint point(round(x), round(y), depth);
        point.texturePoint = texturePoints[i];
        canvasPoints.push_back(point);
    }

    return canvasPoints;
}

//check if a point is within window bounds
bool isWithinBounds(float x, float y, DrawingWindow &window) {
    return (x >= 0 && x < window.width && y >= 0 && y < window.height);
}

//create a pixel color from a Colour object
uint32_t createPixelColor(const Colour &colour) {
    return (255u << 24u) | (static_cast<uint32_t>(colour.red) << 16u) | (static_cast<uint32_t>(colour.green) << 8u) | static_cast<uint32_t>(colour.blue);
}

void drawLine(DrawingWindow &window, std::vector<float> &buffer, CanvasPoint from, CanvasPoint to, float numberOfSteps, std::vector<Colour> colours) {
    auto interpolatedPoints = interpolatePoints(from, to, numberOfSteps + 1);
    
    for (float i = 0.0; i <= numberOfSteps; i++) {
        auto point = interpolatedPoints[i];
        auto x = point.x;
        auto y = point.y;
        
        if (isWithinBounds(x, y, window)) {
            int depthIndex = (y * window.width) + x;
            
            if (point.depth > buffer[depthIndex]) {
                buffer[depthIndex] = point.depth;
                uint32_t pixelColor = createPixelColor(colours[i]);
                window.setPixelColour(x, y, pixelColor);
            }
        }
    }
}


void drawLine(DrawingWindow &window, std::vector<float> &buffer, CanvasPoint from, CanvasPoint to, Colour colour) {
	float numberOfSteps = getNumberOfSteps(from, to);
	drawLine(window, buffer, from, to, numberOfSteps, std::vector<Colour>(numberOfSteps + 1, colour));
}

template <typename T>
void getRowStartsAndEnds(T p1, T p2, T p3, float height1, float height2, std::vector<T> &rowStarts, std::vector<T> &rowEnds) {
	rowStarts = interpolatePoints(p1, p2, height1);
	if (height2 > 1) {
		auto side2 = interpolatePoints(p2, p3, height2);
		rowStarts.pop_back();
		rowStarts.insert(rowStarts.end(), side2.begin(), side2.end());
	}
	rowEnds = interpolatePoints(p1, p3, height1 + height2 - 1);
}

template <typename T>
void getRowStartsAndEnds(CanvasTriangle triangle, float height1, float height2, std::vector<T> &rowStarts, std::vector<T> &rowEnds) {
	getRowStartsAndEnds(triangle[0], triangle[1], triangle[2], height1, height2, rowStarts, rowEnds);
}

CanvasTriangle generateTriangle(DrawingWindow &window) {
	float depth = (float(rand()) / float(RAND_MAX)) * -10;
	return CanvasTriangle(
		CanvasPoint(rand()%(window.width - 1), rand()%(window.height - 1), depth),
		CanvasPoint(rand()%(window.width - 1), rand()%(window.height - 1), depth),
		CanvasPoint(rand()%(window.width - 1), rand()%(window.height - 1), depth)
	);
}

CanvasTriangle generateTexturedTriangle() {
	float depth = (float(rand()) / float(RAND_MAX)) * -10;
	CanvasTriangle triangle = CanvasTriangle(CanvasPoint(160, 10, depth), CanvasPoint(300, 230, depth), CanvasPoint(10, 150, depth));
	triangle[0].texturePoint = TexturePoint(195, 5);
	triangle[1].texturePoint = TexturePoint(395, 380);
	triangle[2].texturePoint = TexturePoint(65, 330);
	return triangle;
}

void drawStrokedTriangle(DrawingWindow &window, std::vector<float> &buffer, CanvasTriangle triangle, Colour colour) {
	drawLine(window, buffer, triangle[0], triangle[1], colour);
	drawLine(window, buffer, triangle[0], triangle[2], colour);
	drawLine(window, buffer, triangle[1], triangle[2], colour);
}

void drawFilledTriangle(DrawingWindow &window, std::vector<float> &buffer, CanvasTriangle triangle, Colour colour, bool outline) {
	sortTriangleVertices(triangle);

	// Work out the start and end xPos of each row of the triangle
	std::vector<CanvasPoint> starts, ends;
	getRowStartsAndEnds(triangle, triangle[1].y - triangle[0].y + 1, triangle[2].y - triangle[1].y + 1, starts, ends);

	// Draw the outline (if there is one) then fill in the triangle
	if (outline) drawStrokedTriangle(window, buffer, triangle, Colour(255, 255, 255));

	for (int i=0; i<=triangle[2].y - triangle[0].y; i++) {
		starts[i].x = std::max(starts[i].x, 0.0f);
		ends[i].x = std::min(ends[i].x, float(window.width));
		drawLine(window, buffer, starts[i], ends[i], colour);
	}
}

void drawTexturedTriangle(DrawingWindow &window, std::vector<float> &buffer, CanvasTriangle triangle, TextureMap &texMap, bool outline) {
	sortTriangleVertices(triangle);

	float height1 = triangle[1].y - triangle[0].y + 1;
	float height2 = triangle[2].y - triangle[1].y + 1;
	std::vector<CanvasPoint> canvasStarts, canvasEnds;
	getRowStartsAndEnds(triangle, height1, height2, canvasStarts, canvasEnds);

	if (outline) drawStrokedTriangle(window, buffer, triangle, Colour(255, 255, 255));

	for(int i=0; i<=height1 + height2 - 2; i++) {
		float numberOfSteps = getNumberOfSteps(canvasStarts[i], canvasEnds[i]);
		auto points = interpolatePoints(canvasStarts[i].texturePoint, canvasEnds[i].texturePoint, numberOfSteps + 1);

		std::vector<Colour> colours;
		for (float i=0.0; i<=numberOfSteps; i++) {
			uint32_t c = texMap.pixels[(round(points[i].y) * texMap.width) + round(points[i].x)];
			colours.push_back(Colour((c & 0xFF0000) >> 16, (c & 0xFF00) >> 8, (c & 0xFF)));
		}
		drawLine(window, buffer, canvasStarts[i], canvasEnds[i], numberOfSteps, colours);
	}
}

glm::mat4 translationMatrix(glm::vec3 vector);

glm::mat4 rotationMatrixX(float radians);
glm::mat4 rotationMatrixY(float radians);
glm::mat4 rotationMatrixZ(float radians);

// Function to create a translation matrix
glm::mat4 translationMatrix(glm::vec3 v) {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,   // Row 1
        0.0f, 1.0f, 0.0f, 0.0f,   // Row 2
        0.0f, 0.0f, 1.0f, 0.0f,   // Row 3
        v[0], v[1], v[2], 1.0f    // Row 4 (translation values)
    };
}

// Function to create a rotation matrix around the X-axis
glm::mat4 rotationMatrixX(float rads) {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,   // Row 1
        0.0f, cos(rads), sin(rads), 0.0f,  // Row 2 (cos and sin for X-axis)
        0.0f, -sin(rads), cos(rads), 0.0f, // Row 3
        0.0f, 0.0f, 0.0f, 1.0f    // Row 4
    };
}

// Function to create a rotation matrix around the Y-axis
glm::mat4 rotationMatrixY(float rads) {
    return {
        cos(rads), 0.0f, -sin(rads), 0.0f,  // Row 1 (cos and sin for Y-axis)
        0.0f, 1.0f, 0.0f, 0.0f,             // Row 2
        sin(rads), 0.0f, cos(rads), 0.0f,   // Row 3
        0.0f, 0.0f, 0.0f, 1.0f              // Row 4
    };
}

// Function to create a rotation matrix around the Z-axis
glm::mat4 rotationMatrixZ(float rads) {
    return {
        cos(rads), sin(rads), 0.0f, 0.0f,   // Row 1 (cos and sin for Z-axis)
        -sin(rads), cos(rads), 0.0f, 0.0f,  // Row 2
        0.0f, 0.0f, 1.0f, 0.0f,             // Row 3
        0.0f, 0.0f, 0.0f, 1.0f              // Row 4
    };
}


void draw(DrawingWindow &window, int renderMode, glm::mat4 camera, float focalLength, float planeMultiplier, std::vector<ModelTriangle> models, TextureMap texMap) {
	window.clearPixels();
	std::vector<float> buffer = std::vector<float>(window.height * window.width, 0);

	for (int i=0; i<models.size(); i++) {
		auto model = models[i];
		CanvasTriangle triangle = CanvasTriangle();

		glm::vec4 camPos(camera[3]);
		glm::mat4 camOri(
			glm::vec4(camera[0]),
			glm::vec4(camera[1]),
			glm::vec4(camera[2]),
			glm::vec4(0.0, 0.0, 0.0, 1.0)
		);
		
		for (int j=0; j<model.vertices.size(); j++) {
			auto vertex = glm::vec3(model.vertices[j]) - glm::vec3(camPos);
			triangle.vertices[j] = CanvasPoint(
				round((planeMultiplier * focalLength * vertex[0] / vertex[2]) + (window.width / 2)),
				round((planeMultiplier * focalLength * vertex[1] / vertex[2]) + (window.height / 2)),
				-1/vertex[2]
			);
			triangle.vertices[j].texturePoint = model.texturePoints[j];
		}

		if (renderMode == 0) {
			drawStrokedTriangle(window, buffer, triangle, model.colour);
		}
		else if (model.colour.name == "Cobbles") {
			drawTexturedTriangle(window, buffer, triangle, texMap, false);
		}
		else {
			drawFilledTriangle(window, buffer, triangle, model.colour, false);
		}
	}
}

RayTriangleIntersection findClosestIntersection(glm::vec3 start, glm::vec3 dir, std::vector<ModelTriangle> triangles) {
    RayTriangleIntersection closestIntersection;
    float epsilon = 0.00001;
    closestIntersection.distanceFromCamera = std::numeric_limits<float>::infinity();

    for (int i = 0; i < triangles.size(); i++) {
        auto tri = triangles[i];
        auto barycentric = glm::inverse(glm::mat3(
            -dir,
            glm::vec3(tri.vertices[1] - tri.vertices[0]),
            glm::vec3(tri.vertices[2] - tri.vertices[0])
        )) * (start - glm::vec3(tri.vertices[0]));

        if (epsilon <= barycentric[0] && barycentric[0] < closestIntersection.distanceFromCamera &&
            0 <= barycentric[1] && barycentric[1] <= 1 &&
            0 <= barycentric[2] && barycentric[2] <= 1 &&
            barycentric[1] + barycentric[2] <= 1
        ) {
            closestIntersection.intersectionPoint = barycentric;
            closestIntersection.distanceFromCamera = barycentric[0];
            closestIntersection.intersectedTriangle = tri;
            closestIntersection.triangleIndex = i;
        }
    }

    return closestIntersection;
}

void drawRaytracedLight(DrawingWindow &window, glm::mat4 camera, float focalLength, float planeMultiplier, std::vector<ModelTriangle> models, glm::vec3 light, TextureMap texMap) {
	glm::vec3 camPos(camera[3]);
	glm::mat3 camOri(
		(glm::vec3(camera[0])),
		glm::vec3(camera[1]),
		glm::vec3(camera[2])
	);

	for (int x=0; x<window.width; x++) {
		for (int y=0; y<window.height; y++) {
			glm::vec3 direction((float(window.width / 2) - x) / planeMultiplier, (float(window.height / 2) - y) / planeMultiplier, -2);
			
			auto intersect = findClosestIntersection(glm::vec3(camPos), glm::normalize(camOri * direction), models);
			auto colour = intersect.intersectedTriangle.colour;
			
			if (intersect.distanceFromCamera != std::numeric_limits<float>::infinity()) {
				auto ps = intersect.intersectedTriangle.vertices;
				auto u = intersect.intersectionPoint[1];
				auto v = intersect.intersectionPoint[2];
				auto r = glm::vec3(ps[0] + u * (ps[1] - ps[0]) + v * (ps[2] - ps[0]));
				auto lightDirection = light - r;

				float lightStrength = 50;
				float specularScale = 256;
				double ambientLight = 0.05;
				double brightness;

				auto shadowIntersect = findClosestIntersection(r, glm::normalize(lightDirection), models);
				if (shadowIntersect.distanceFromCamera < glm::length(lightDirection)) {
					brightness = ambientLight;
				} else {
					auto length = glm::length(lightDirection);
					auto normal = intersect.intersectedTriangle.normal;
					brightness = std::min(1.0, lightStrength / (4 * PI * (length * length)));
					brightness *= std::max(0.0f, glm::dot(glm::normalize(lightDirection), normal));
					brightness = std::max(float(brightness), std::pow(glm::dot(glm::normalize(camOri * direction), glm::normalize(lightDirection - normal * 2.0f * glm::dot(lightDirection, normal))), specularScale));
					brightness = std::max(ambientLight, std::min(1.0, brightness));
				}
				colour.red *= brightness;
				colour.green *= brightness;
				colour.blue *= brightness;
			}
			uint32_t c = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
			window.setPixelColour(x, y, c);
		}
	}
}

glm::mat4 lookAt(glm::mat4 &camera, glm::vec3 target) {
	glm::vec3 z = glm::normalize(glm::vec3(camera[3] / camera[3][3]) - target);
	glm::vec3 x = glm::normalize(glm::cross(glm::vec3(0.0, 1.0, 0.0), z));
	glm::vec3 y = glm::normalize(glm::cross(z, x));
	return glm::mat4(
		glm::vec4(x, 0.0),
		glm::vec4(y, 0.0),
		glm::vec4(z, 0.0),
		glm::vec4(camera[3])
	);
}



void handleEvent(SDL_Event event, DrawingWindow &window, std::vector<float> &buffer, TextureMap &texMap, glm::mat4 &camera, int &renderMode) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
		//wasd,qe
		else if (event.key.keysym.sym == SDLK_w) {
			camera = translationMatrix(glm::vec3(0.0, 0.0, -0.5)) * camera; 
			std::cout << "LookAt" << std::endl;
		} 
		else if (event.key.keysym.sym == SDLK_s) camera = translationMatrix(glm::vec3(0.0, 0.0, 0.5)) * camera;
		else if (event.key.keysym.sym == SDLK_a) camera = translationMatrix(glm::vec3(0.5, 0.0, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_d) camera = translationMatrix(glm::vec3(-0.5, 0.0, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_q) camera = translationMatrix(glm::vec3(0.0, 0.5, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_e) camera = translationMatrix(glm::vec3(0.0, -0.5, 0.0)) * camera;
		//zxcvbn
		else if (event.key.keysym.sym == SDLK_z) camera = rotationMatrixY(0.1) * camera;
		else if (event.key.keysym.sym == SDLK_x) camera = rotationMatrixY(-0.1) * camera;
		else if (event.key.keysym.sym == SDLK_c) camera = rotationMatrixX(-0.1) * camera;
		else if (event.key.keysym.sym == SDLK_v) camera = rotationMatrixX(0.1) * camera;
		else if (event.key.keysym.sym == SDLK_b) camera = rotationMatrixZ(0.1) * camera;
		else if (event.key.keysym.sym == SDLK_n) camera = rotationMatrixZ(-0.1) * camera;
		//m
		else if (event.key.keysym.sym == SDLK_m) camera = lookAt(camera, glm::vec3(0.0, 0.0, 0.0));
		//012
		else if (event.key.keysym.sym == SDLK_0) {
			renderMode = 0; 
			std::cout << "Wireframe 3D scene rendering" << std::endl;
		}
		else if (event.key.keysym.sym == SDLK_1) {
			renderMode = 1; 
			std::cout << "Flat colour 3D scene rasterising" << std::endl;
		}
		else if (event.key.keysym.sym == SDLK_2) {
			renderMode = 2; 
			std::cout << "10-30seconds: Proximity/Angle of Incidence/Specular/Ambient Lighting" << std::endl;
		}

	} else if (event.type == SDL_MOUSEBUTTONDOWN) {
		window.savePPM("output.ppm"); 
		window.saveBMP("output.bmp");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	TextureMap texMap = TextureMap("models/m-texture-1.ppm");
	std::vector<float> buffer = std::vector<float>(window.height * window.width, 0);
	std::string objFile = "models/textured-cornell-box.obj";
    float scale = 1.0f;
    std::vector<ModelTriangle> loadedTriangles = loadObj(objFile, scale);
	//float vertexScale = 1.0;
	//std::vector<ModelTriangle> models = loadObj("models/textured-cornell-box.obj", vertexScale);
	glm::vec3 lightSource(0.0, 1.7, 0.0);
	glm::mat4 camera(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 10.0, 1.0
	);
	float focalLength = 2.0;
	float planeMultiplier = 250;
	int renderMode = 1;

	SDL_Event event;
	while (true) {
		if (window.pollForInputEvents(event)) {
			handleEvent(event, window, buffer, texMap, camera, renderMode);

			if (renderMode == 2) drawRaytracedLight(window, camera, focalLength, planeMultiplier, loadedTriangles, lightSource, texMap);
			else draw(window, renderMode, camera, focalLength, planeMultiplier, loadedTriangles, texMap);
		}
		window.renderFrame();
	}
}