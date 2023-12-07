#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <ModelTriangle.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <Utils.h>
#include <algorithm>
#include <RayTriangleIntersection.h>
#include <cmath>

#define WIDTH 640
#define HEIGHT 480

#define PI 3.14159265358979323846

std::vector<Colour> loadMtl(const std::string &filename) {
	std::vector<Colour> colours;

	std::ifstream inputStream(filename, std::ifstream::in);
	std::string nextLine;
	std::string name;

	while (std::getline(inputStream, nextLine)) {
		auto line = split(nextLine, ' ');
		
		if (line[0] == "newmtl") {
			name = line[1];
		} else if (line[0] == "Kd") {
			colours.push_back(Colour(
				name,
				(int)(std::stof(line[1]) * 255),
				(int)(std::stof(line[2]) * 255),
				(int)(std::stof(line[3]) * 255)
			));
		}
	}
	return colours;
}

std::vector<ModelTriangle> loadObj(const std::string &filename, float scale) {
	std::vector<glm::vec4> vertices;
	std::vector<TexturePoint> textureVertices;
	std::vector<ModelTriangle> models;

	std::ifstream inputStream(filename, std::ifstream::in);
	std::string nextLine;
	std::vector<Colour> materials;
	Colour colour = Colour(255, 255, 255);

	while (std::getline(inputStream, nextLine)) {
		auto vector = split(nextLine, ' ');
		if (vector[0] == "mtllib") {
			materials = loadMtl(vector[1]);
		}
		else if (vector[0] == "usemtl") {
			for (int i=0; i<materials.size(); i++) {
				if (materials[i].name == vector[1]) {
					colour = materials[i];
					break;
				}
			}
		}
		else if (vector[0] == "v") {
			vertices.push_back(glm::vec4(
				std::stof(vector[1]) * scale,
				std::stof(vector[2]) * scale,
				std::stof(vector[3]) * scale,
				1.0
			));
		}
		else if (vector[0] == "vt") {
			textureVertices.push_back(TexturePoint(
				round(std::stof(vector[1]) * 480),
				round(std::stof(vector[2]) * 395)
			));
		}
		else if (vector[0] == "f") {
			auto v1 = split(vector[1], '/');
			auto v2 = split(vector[2], '/');
			auto v3 = split(vector[3], '/');
			auto triangle = ModelTriangle(
				vertices[std::stoi(v1[0]) - 1],
				vertices[std::stoi(v2[0]) - 1],
				vertices[std::stoi(v3[0]) - 1],
				colour
			);
			if (v1[1] != "") {
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

template <typename PointType>
std::vector<PointType> interpolatePoints(PointType from, PointType to, int steps) {
    std::vector<PointType> points;
    float xStep = (to.x - from.x) / (steps - 1);
    float yStep = (to.y - from.y) / (steps - 1);

    for (int i = 0; i < steps; i++) {
        points.push_back(PointType(from.x + i * xStep, from.y + i * yStep));
    }
    return points;
}

float getNumberOfSteps(CanvasPoint from, CanvasPoint to) {
    return std::max({std::abs(to.x - from.x), std::abs(to.y - from.y), 1.0f});
}

void sortTriangleVertices(CanvasTriangle &triangle) {
    if (triangle[0].y > triangle[1].y) std::swap(triangle[0], triangle[1]);
    if (triangle[1].y > triangle[2].y) std::swap(triangle[1], triangle[2]);
    if (triangle[0].y > triangle[1].y) std::swap(triangle[0], triangle[1]);
}

std::vector<TexturePoint> interpolatePoints(TexturePoint from, TexturePoint to, int steps) {
	std::vector<TexturePoint> TexturePoints;
	float xStep = (to.x - from.x) / (steps - 1);
	float yStep = (to.y - from.y) / (steps - 1);
	for (int i=0; i<steps; i++) {
		TexturePoints.push_back(TexturePoint(round(from.x + (i * xStep)),  round(from.y + (i * yStep))));
	}
	return TexturePoints;
}

std::vector<CanvasPoint> interpolateCanvasPoints(CanvasPoint from, CanvasPoint to, int steps) {
    auto canvasPoints = interpolatePoints(from, to, steps);
    float depthStep = (to.depth - from.depth) / (steps - 1);
    
    for (int i = 0; i < steps; i++) {
        canvasPoints[i].depth = from.depth + i * depthStep;
        canvasPoints[i].texturePoint = interpolatePoints(from.texturePoint, to.texturePoint, steps)[i];
    }
    return canvasPoints;
}




void drawLine(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasPoint from, CanvasPoint to, float numberOfSteps, std::vector<Colour> colours) {
	auto points = interpolatePoints(from, to, numberOfSteps + 1);
	for (float i=0.0; i<=numberOfSteps; i++) {
		auto x = points[i].x;
		auto y = points[i].y;
		if (0 <= y && y < window.height && 0 <= x && x < window.width) {
			int depthIndex = (y * window.width) + x;
			if (points[i].depth > depthBuffer[depthIndex]) {
				depthBuffer[depthIndex] = points[i].depth;
				uint32_t c = (255 << 24) + (colours[i].red << 16) + (colours[i].green << 8) + colours[i].blue;
				window.setPixelColour(x, y, c);
			}
		}
	}
}

void drawLine(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasPoint from, CanvasPoint to, Colour colour) {
	float numberOfSteps = getNumberOfSteps(from, to);
	drawLine(window, depthBuffer, from, to, numberOfSteps, std::vector<Colour>(numberOfSteps + 1, colour));
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

void drawStrokedTriangle(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasTriangle triangle, Colour colour) {
	drawLine(window, depthBuffer, triangle[0], triangle[1], colour);
	drawLine(window, depthBuffer, triangle[0], triangle[2], colour);
	drawLine(window, depthBuffer, triangle[1], triangle[2], colour);
}

void drawFilledTriangle(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasTriangle triangle, Colour colour, bool outline) {
	sortTriangleVertices(triangle);

	// Work out the start and end xPos of each row of the triangle
	std::vector<CanvasPoint> starts, ends;
	getRowStartsAndEnds(triangle, triangle[1].y - triangle[0].y + 1, triangle[2].y - triangle[1].y + 1, starts, ends);

	// Draw the outline (if there is one) then fill in the triangle
	if (outline) drawStrokedTriangle(window, depthBuffer, triangle, Colour(255, 255, 255));

	for (int i=0; i<=triangle[2].y - triangle[0].y; i++) {
		starts[i].x = std::max(starts[i].x, 0.0f);
		ends[i].x = std::min(ends[i].x, float(window.width));
		drawLine(window, depthBuffer, starts[i], ends[i], colour);
	}
}

void drawTexturedTriangle(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasTriangle triangle, TextureMap &texMap, bool outline) {
	sortTriangleVertices(triangle);

	float height1 = triangle[1].y - triangle[0].y + 1;
	float height2 = triangle[2].y - triangle[1].y + 1;
	std::vector<CanvasPoint> canvasStarts, canvasEnds;
	getRowStartsAndEnds(triangle, height1, height2, canvasStarts, canvasEnds);

	if (outline) drawStrokedTriangle(window, depthBuffer, triangle, Colour(255, 255, 255));

	for(int i=0; i<=height1 + height2 - 2; i++) {
		float numberOfSteps = getNumberOfSteps(canvasStarts[i], canvasEnds[i]);
		auto points = interpolatePoints(canvasStarts[i].texturePoint, canvasEnds[i].texturePoint, numberOfSteps + 1);

		std::vector<Colour> colours;
		for (float i=0.0; i<=numberOfSteps; i++) {
			uint32_t c = texMap.pixels[(round(points[i].y) * texMap.width) + round(points[i].x)];
			colours.push_back(Colour((c & 0xFF0000) >> 16, (c & 0xFF00) >> 8, (c & 0xFF)));
		}
		drawLine(window, depthBuffer, canvasStarts[i], canvasEnds[i], numberOfSteps, colours);
	}
}

glm::mat4 translationMatrix(glm::vec3 vector);

glm::mat4 rotationMatrixX(float radians);
glm::mat4 rotationMatrixY(float radians);
glm::mat4 rotationMatrixZ(float radians);

glm::mat4 translationMatrix(glm::vec3 v) {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        v[0], v[1], v[2], 1.0f
    };
}

glm::mat4 rotationMatrixX(float rads) {
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, cos(rads), sin(rads), 0.0f,
        0.0f, -sin(rads),  cos(rads), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

glm::mat4 rotationMatrixY(float rads) {
    return {
        cos(rads), 0.0f, -sin(rads), 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        sin(rads), 0.0f, cos(rads), 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

glm::mat4 rotationMatrixZ(float rads) {
    return {
        cos(rads), sin(rads), 0.0f, 0.0f,
        -sin(rads), cos(rads), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

void draw(DrawingWindow &window, int renderMode, glm::mat4 camera, float focalLength, float planeMultiplier, std::vector<ModelTriangle> models, TextureMap texMap) {
	window.clearPixels();
	std::vector<float> depthBuffer = std::vector<float>(window.height * window.width, 0);

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
			auto vertex = (model.vertices[j] -  camPos) * camOri;
			triangle.vertices[j] = CanvasPoint(
				round((planeMultiplier * focalLength * vertex[0] / vertex[2]) + (window.width / 2)),
				round((planeMultiplier * focalLength * vertex[1] / vertex[2]) + (window.height / 2)),
				-1/vertex[2]
			);
			triangle.vertices[j].texturePoint = model.texturePoints[j];
		}

		if (renderMode == 0) {
			drawStrokedTriangle(window, depthBuffer, triangle, model.colour);
		}
		else if (model.colour.name == "Cobbles") {
			drawTexturedTriangle(window, depthBuffer, triangle, texMap, false);
		}
		else {
			drawFilledTriangle(window, depthBuffer, triangle, model.colour, false);
		}
	}
}

RayTriangleIntersection getClosestIntersection(glm::vec3 startPoint, glm::vec3 direction, std::vector<ModelTriangle> triangles) {
	RayTriangleIntersection intersection;
	float epsilon = 0.00001;
	intersection.distanceFromCamera = std::numeric_limits<float>::infinity();
	for (int i=0; i<triangles.size(); i++) {
		auto triangle = triangles[i];
		auto point = glm::inverse(glm::mat3(
			-direction,
			glm::vec3(triangle.vertices[1] - triangle.vertices[0]),
			glm::vec3(triangle.vertices[2] - triangle.vertices[0])
		)) * (startPoint - glm::vec3(triangle.vertices[0]));

		if (epsilon <= point[0] && point[0] < intersection.distanceFromCamera &&
			0 <= point[1] && point[1] <= 1 &&
			0 <= point[2] && point[2] <= 1 &&
			point[1] + point[2] <= 1
		) {
			intersection.intersectionPoint = point;
			intersection.distanceFromCamera = point[0];
			intersection.intersectedTriangle = triangle;
			intersection.triangleIndex = i;
		}
	}
	return intersection;
}

void drawRaytraced(DrawingWindow &window, glm::mat4 camera, float focalLength, float planeMultiplier, std::vector<ModelTriangle> models, glm::vec3 light, TextureMap texMap) {
	glm::vec3 camPos(camera[3]);
	glm::mat3 camOri(
		(glm::vec3(camera[0])),
		glm::vec3(camera[1]),
		glm::vec3(camera[2])
	);

	for (int x=0; x<window.width; x++) {
		for (int y=0; y<window.height; y++) {
			glm::vec3 direction((float(window.width / 2) - x) / planeMultiplier, (float(window.height / 2) - y) / planeMultiplier, -2);
			
			auto intersect = getClosestIntersection(glm::vec3(camPos), glm::normalize(camOri * direction), models);
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

				auto shadowIntersect = getClosestIntersection(r, glm::normalize(lightDirection), models);
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

void update(DrawingWindow &window, glm::mat4 &camera) {
	camera = lookAt(camera, glm::vec3(0, 0, 0));
}

void handleEvent(SDL_Event event, DrawingWindow &window, std::vector<float> &depthBuffer, TextureMap &texMap, glm::mat4 &camera, int &renderMode) {
	if (event.type == SDL_KEYDOWN) {
		if (event.key.keysym.sym == SDLK_LEFT) std::cout << "LEFT" << std::endl;
		else if (event.key.keysym.sym == SDLK_RIGHT) std::cout << "RIGHT" << std::endl;
		else if (event.key.keysym.sym == SDLK_UP) std::cout << "UP" << std::endl;
		else if (event.key.keysym.sym == SDLK_DOWN) std::cout << "DOWN" << std::endl;
		//wasd,qe
		else if (event.key.keysym.sym == SDLK_w) camera = translationMatrix(glm::vec3(0.0, 0.0, -0.5)) * camera;
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
	std::vector<float> depthBuffer = std::vector<float>(window.height * window.width, 0);
	
	float vertexScale = 1.0;
	std::vector<ModelTriangle> models = loadObj("models/textured-cornell-box.obj", vertexScale);
	glm::vec3 lightSource(0.0, 1.5, 0.0);
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
			handleEvent(event, window, depthBuffer, texMap, camera, renderMode);
			// update(window, camera);
			if (renderMode == 2) drawRaytraced(window, camera, focalLength, planeMultiplier, models, lightSource, texMap);
			else draw(window, renderMode, camera, focalLength, planeMultiplier, models, texMap);
		}
		window.renderFrame();
	}
}
