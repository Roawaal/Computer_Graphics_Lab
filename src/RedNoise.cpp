#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <vector>
#include <glm/glm.hpp>
#include <CanvasPoint.h>
#include <CanvasTriangle.h>
#include <Colour.h>
#include <ModelTriangle.h>
#include <Model.h>
#include <TextureMap.h>
#include <TexturePoint.h>
#include <algorithm>
#include <Maths.h>
#include <RayTriangleIntersection.h>
#include <cmath>
#include <OBJParser.h>
#include <Light.h>

#define WIDTH 640
#define HEIGHT 480

#define PI 3.14159265358979323846

struct LightingSettings {
	bool enabled;
	bool phongShading;
	bool softShadows;
	bool reflective;
	bool proximity;
	bool incidence;
	bool specular;
	float specularStrength;
	float specularScale;
	glm::vec3 ambientLight{};
	LightingSettings() = default;
	LightingSettings(bool enabled, bool phongShading, bool softShadows, bool reflective, bool proximity, bool incidence, bool specular, float specularStrength, float specularScale, const glm::vec3 &ambientLight) :
		enabled(enabled), phongShading(phongShading), softShadows(softShadows), reflective(reflective), proximity(proximity), incidence(incidence), specular(specular), specularStrength(specularStrength), specularScale(specularScale), ambientLight(ambientLight) {};
};

RayTriangleIntersection getClosestIntersection(glm::vec3 startPoint, glm::vec3 direction, const std::vector<Model> &models) {
    RayTriangleIntersection intersection;
    intersection.distanceFromCamera = std::numeric_limits<float>::infinity();
    float epsilon = 0.0001;

    for (int modelIndex = 0; modelIndex < models.size(); ++modelIndex) {
        const Model& model = models[modelIndex];
        
        for (int triangleIndex = 0; triangleIndex < model.faces.size(); ++triangleIndex) {
            const ModelTriangle& triangle = model.faces[triangleIndex];
            
            glm::vec3 edge1 = glm::vec3(triangle.vertices[1]) - glm::vec3(triangle.vertices[0]);
            glm::vec3 edge2 = glm::vec3(triangle.vertices[2]) - glm::vec3(triangle.vertices[0]);
            
            glm::vec3 h = glm::cross(direction, edge2);
            float a = glm::dot(edge1, h);
            
            if (a > -epsilon && a < epsilon) {
                continue;  // The ray is parallel to this triangle.
            }
            
            float f = 1.0f / a;
            glm::vec3 s = startPoint - glm::vec3(triangle.vertices[0]);
            float u = f * glm::dot(s, h);
            
            if (u < 0.0f || u > 1.0f) {
                continue;  // The intersection point is outside the triangle.
            }
            
            glm::vec3 q = glm::cross(s, edge1);
            float v = f * glm::dot(direction, q);
            
            if (v < 0.0f || u + v > 1.0f) {
                continue;  // The intersection point is outside the triangle.
            }
            
            float t = f * glm::dot(edge2, q);
            
            if (t > epsilon && t < intersection.distanceFromCamera) {
                intersection.distanceFromCamera = t;
                intersection.intersectionPoint = startPoint + direction * t;
                intersection.intersectedTriangle = triangle;
                intersection.modelIndex = modelIndex;
                intersection.triangleIndex = triangleIndex;
            }
        }
    }
    
    return intersection;
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

std::vector<TexturePoint> interpolatePoints(TexturePoint from, TexturePoint to, int steps) {
	std::vector<TexturePoint> TexturePoints;
	float xStep = (to.x - from.x) / (steps - 1);
	float yStep = (to.y - from.y) / (steps - 1);
	for (int i=0; i<steps; i++) {
		TexturePoints.push_back(TexturePoint(from.x + (i * xStep),  from.y + (i * yStep)));
	}
	return TexturePoints;
}
template<typename T>
std::vector<T> interpolateValues(const T& from, const T& to, int steps) {
    std::vector<T> result;
    T step = (to - from) / (steps - 1);
    for (int i = 0; i < steps; i++) {
        result.push_back(from + (i * step));
    }
    return result;
}

std::vector<CanvasPoint> interpolatePoints(CanvasPoint from, CanvasPoint to, int steps) {
    std::vector<CanvasPoint> canvasPoints;
    auto xValues = interpolateValues(from.x, to.x, steps);
    auto yValues = interpolateValues(from.y, to.y, steps);
    auto depthValues = interpolateValues(from.depth, to.depth, steps);
    auto texturePoints = interpolatePoints(from.texturePoint, to.texturePoint, steps);

    for (int i = 0; i < steps; i++) {
        CanvasPoint point(round(xValues[i]), round(yValues[i]), depthValues[i]);
        point.texturePoint = texturePoints[i];
        canvasPoints.push_back(point);
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

CanvasTriangle generateTriangle(const DrawingWindow &window) {
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

void drawTexturedTriangle(DrawingWindow &window, std::vector<float> &depthBuffer, CanvasTriangle triangle, const TextureMap &texMap, bool outline) {
	sortTriangleVertices(triangle);

	float height1 = triangle[1].y - triangle[0].y + 1;
	float height2 = triangle[2].y - triangle[1].y + 1;
	std::vector<CanvasPoint> canvasStarts, canvasEnds;
	getRowStartsAndEnds(triangle, height1, height2, canvasStarts, canvasEnds);

	if (outline) drawStrokedTriangle(window, depthBuffer, triangle, Colour(255, 255, 255));

	for(int i=0; i<=height1 + height2 - 2; i++) {
		float numberOfSteps = getNumberOfSteps(canvasStarts[i], canvasEnds[i]);
		auto points = interpolatePoints(canvasStarts[i].texturePoint, canvasEnds[i].texturePoint, numberOfSteps + 1);

		auto width = texMap.width;
		auto height = texMap.height;
		
		std::vector<Colour> colours;
		for (float i=0.0; i<=numberOfSteps; i++) {
			uint32_t c = texMap.pixels[(int(round(points[i].y * height)) % height) * width + int(round(points[i].x * width)) % width];
			colours.push_back(Colour((c & 0xFF0000) >> 16, (c & 0xFF00) >> 8, (c & 0xFF)));
		}
		drawLine(window, depthBuffer, canvasStarts[i], canvasEnds[i], numberOfSteps, colours);
	}
}

void draw(DrawingWindow &window, int renderMode, glm::mat4 camera, float focalLength, float planeMultiplier, const std::vector<Model> &models) {
	window.clearPixels();
	std::vector<float> depthBuffer = std::vector<float>(window.height * window.width, 0);

	for (int j=0; j<models.size(); j++) {
		auto faces = models[j].faces;
		for (int i=0; i<faces.size(); i++) {
			auto face = faces[i];
			CanvasTriangle triangle = CanvasTriangle();

			glm::vec4 camPos(camera[3]);
			glm::mat4 camOri(
				glm::vec4(camera[0]),
				glm::vec4(camera[1]),
				glm::vec4(camera[2]),
				glm::vec4(0.0, 0.0, 0.0, 1.0)
			);
			
			for (int k=0; k<face.vertices.size(); k++) {
				auto modelPosition = models[j].position;
				auto vertex = glm::vec3((face.vertices[k] -  camPos) * camOri);
				triangle.vertices[k] = CanvasPoint(
					round(-(planeMultiplier * focalLength * (vertex[0] + modelPosition[0]) / (vertex[2] + modelPosition[2])) + (window.width / 2)),
					round((planeMultiplier * focalLength * (vertex[1] + modelPosition[1]) / (vertex[2] + modelPosition[2])) + (window.height / 2)),
					-1/vertex[2]
				);
				triangle.vertices[k].texturePoint = face.texturePoints[k];
			}

			if (renderMode == 0) {
				drawStrokedTriangle(window, depthBuffer, triangle, face.material.colour);
			}
			else if (face.material.textureMap.width != 0) {
				drawTexturedTriangle(window, depthBuffer, triangle, face.material.textureMap, false);
			}
			else {
				drawFilledTriangle(window, depthBuffer, triangle, face.material.colour, false);
			}
		}
	}
	
}

glm::vec3 getPixelBrightness(glm::vec3 camDir, float u, float v, ModelTriangle triangle, glm::vec3 modelPosition, const std::vector<Model> &models, const std::vector<Light> &lights, const LightingSettings &lightingSettings) {
	auto ps = triangle.vertices;
	auto r = glm::vec3(ps[0] + u * (ps[1] - ps[0]) + v * (ps[2] - ps[0])) + modelPosition;
	glm::vec3 brightness(0.0, 0.0, 0.0);
	for (const auto &light : lights) {
		std::vector<glm::vec3> directions;
		directions.push_back(light.position - r);
		// Poor attempt at softened shadows lol
		if (lightingSettings.softShadows) {
			for (int i=0; i<light.size * 2; i++) {
				auto j = (i + 1) * 0.025;
				directions.push_back(light.position + glm::vec3(-j, -j, j) - r);
				directions.push_back(light.position + glm::vec3(-j, -j, -j) - r);
				directions.push_back(light.position + glm::vec3(-j, j, j) - r);
				directions.push_back(light.position + glm::vec3(-j, j, -j) - r);
				directions.push_back(light.position + glm::vec3(j, -j, j) - r);
				directions.push_back(light.position + glm::vec3(j, -j, -j) - r);
				directions.push_back(light.position + glm::vec3(j, j, j) - r);
				directions.push_back(light.position + glm::vec3(j, j, -j) - r);
			}
		}
		
		for (const auto &direction : directions) {
			auto intersect = getClosestIntersection(r, glm::normalize(direction), models);
			auto length = glm::length(direction);
			if (intersect.distanceFromCamera < length) continue;
			
			auto normals = triangle.vertexNormals;
			auto normal = (1 - u - v) * normals[0] + u * normals[1] + v * normals[2];
			if ((normal[0] == 0 && normal[1] == 0 && normal[2] == 0) || !lightingSettings.phongShading) normal = triangle.normal;
			float proximity = lightingSettings.proximity ? (light.strength / directions.size()) / ( 4 * PI * std::pow(length, 2)) : 0.0f;
			float incidence = lightingSettings.incidence ? std::max(0.0f, glm::dot(glm::normalize(direction), normal)) : 0.0f;
			float specular = lightingSettings.specular ? (lightingSettings.specularStrength / directions.size()) * std::pow(glm::dot(glm::normalize(camDir), glm::normalize(direction - normal * 2.0f * glm::dot(direction, normal))), lightingSettings.specularScale) : 0.0;
			float lightBrightness = std::max(specular, proximity * incidence);
			brightness += glm::vec3(light.colour.red * lightBrightness / 255, light.colour.green * lightBrightness / 255, light.colour.blue * lightBrightness / 255);
			if (brightness[0] >= 1.0 && brightness[1] >= 1.0 && brightness[2] >= 1.0) break;
		}		
	}
	brightness[0] = std::max(lightingSettings.ambientLight[0], std::min(1.0f, brightness[0]));
	brightness[1] = std::max(lightingSettings.ambientLight[1], std::min(1.0f, brightness[1]));
	brightness[2] = std::max(lightingSettings.ambientLight[2], std::min(1.0f, brightness[2]));
	return brightness;
}

void drawRaytraced(DrawingWindow &window, glm::mat4 camera, float focalLength, float planeMultiplier, const std::vector<Model> &models, const std::vector<Light> &lights, const LightingSettings &lightingSettings) {
	glm::vec3 camPos(camera[3]);
	glm::mat3 camOri(
		(glm::vec3(camera[0])),
		glm::vec3(camera[1]),
		glm::vec3(camera[2])
	);

	for (int x=0; x<window.width; x++) {
		for (int y=0; y<window.height; y++) {
			glm::vec3 direction(-(float(window.width / 2) - x) / planeMultiplier, (float(window.height / 2) - y) / planeMultiplier, -2);
			glm::vec3 camDir = glm::normalize(camOri * direction);
			auto intersect = getClosestIntersection(glm::vec3(camPos), camDir, models);
			auto colour = intersect.intersectedTriangle.material.colour;
			
			if (lightingSettings.enabled && intersect.distanceFromCamera != std::numeric_limits<float>::infinity()) {
				auto modelPosition = models[intersect.modelIndex].position;
				auto tri = intersect.intersectedTriangle;
				auto u = intersect.intersectionPoint[1];
				auto v = intersect.intersectionPoint[2];
				auto baseBrightness = 1.0;
				
				if (tri.material.name == "REFLECTIVE" && lightingSettings.reflective) {
					// HACK: Hardcoded material name for reflective materials yikes
					auto ps = tri.vertices;
					auto r = glm::vec3(ps[0] + u * (ps[1] - ps[0]) + v * (ps[2] - ps[0])) + modelPosition;
					auto normal = (1 - u - v) * tri.vertexNormals[0] + u * tri.vertexNormals[1] + v * tri.vertexNormals[2];
					if (normal[0] == 0 && normal[1] == 0 && normal[2] == 0) normal = tri.normal;
					auto newCamDir = glm::normalize(camDir - normal * 2.0f * glm::dot(camDir, normal));
					intersect = getClosestIntersection(r, newCamDir, models);
					colour = intersect.intersectedTriangle.material.colour;
					baseBrightness = 0.5;
					modelPosition = models[intersect.modelIndex].position;
					tri = intersect.intersectedTriangle;
					u = intersect.intersectionPoint[1];
					v = intersect.intersectionPoint[2];
				}
				
				if (tri.material.textureMap.width != 0) {
					auto w = tri.material.textureMap.width;
					auto h = tri.material.textureMap.height;
					int x = int((1 - u - v) * tri.texturePoints[0].x * w + u * tri.texturePoints[1].x * w + v * tri.texturePoints[2].x * w) % w;
					int y = int((1 - u - v) * tri.texturePoints[0].y * h + u * tri.texturePoints[1].y * h + v * tri.texturePoints[2].y * h) % h;
					uint32_t tColour = tri.material.textureMap.pixels[w * y + x];
					colour = Colour(tColour >> 16 & 0xFF, tColour >> 8 & 0xFF, tColour & 0xFF);
				}
				auto brightness = getPixelBrightness(camDir, u, v, tri, modelPosition, models, lights, lightingSettings);
				colour.red *= baseBrightness * brightness[0];
				colour.green *= baseBrightness * brightness[1];
				colour.blue *= baseBrightness * brightness[2];
			}
			uint32_t c = (255 << 24) + (colour.red << 16) + (colour.green << 8) + colour.blue;
			window.setPixelColour(x, y, c);
		}
	}
}

void handleEvent(SDL_Event event, DrawingWindow &window, glm::mat4 &camera, int &renderMode, LightingSettings &lightingSettings, bool &drawBool) {
	auto rotRate = (2 * PI) / 60;
	if (event.type == SDL_KEYDOWN) {
		drawBool = true;
		if (event.key.keysym.sym == SDLK_w) camera = translationMatrix(glm::vec3(0.0, 0.0, -rotRate)) * camera;
		else if (event.key.keysym.sym == SDLK_s) camera = translationMatrix(glm::vec3(0.0, 0.0, rotRate)) * camera;
		else if (event.key.keysym.sym == SDLK_z) camera = translationMatrix(glm::vec3(-rotRate, 0.0, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_c) camera = translationMatrix(glm::vec3(rotRate, 0.0, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_q) camera = translationMatrix(glm::vec3(0.0, rotRate, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_e) camera = translationMatrix(glm::vec3(0.0, -rotRate, 0.0)) * camera;
		else if (event.key.keysym.sym == SDLK_a) camera = rotationMatrixY(-rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_d) camera = rotationMatrixY(rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_r) camera = rotationMatrixX(-rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_v) camera = rotationMatrixX(rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_1) camera = rotationMatrixZ(rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_3) camera = rotationMatrixZ(-rotRate) * camera;
		else if (event.key.keysym.sym == SDLK_l) camera = lookAt(camera, glm::vec3(0.0, 0.0, 0.0));
		else if (event.key.keysym.sym == SDLK_b) { renderMode = 0; std::cout << "[RENDERMODE: " << renderMode << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_n) { renderMode = 1; std::cout << "[RENDERMODE: " << renderMode << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_m) { renderMode = 2; std::cout << "[RENDERMODE: " << renderMode << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_4) { lightingSettings.enabled = !lightingSettings.enabled; std::cout << "[LIGHTING: " << lightingSettings.enabled << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_5) { lightingSettings.phongShading = !lightingSettings.phongShading; std::cout << "[PHONG SHADING: " << lightingSettings.phongShading << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_6) { lightingSettings.softShadows = !lightingSettings.softShadows; std::cout << "[SOFT SHADOWS: " << lightingSettings.softShadows << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_7) { lightingSettings.reflective = !lightingSettings.reflective; std::cout << "[REFLECTIONS: " << lightingSettings.reflective << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_8) { lightingSettings.proximity = !lightingSettings.proximity; std::cout << "[PROXIMITY LIGHTING: " << lightingSettings.proximity << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_9) { lightingSettings.incidence = !lightingSettings.incidence; std::cout << "[INCIDENCE LIGHTING: " << lightingSettings.incidence << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_0) { lightingSettings.specular = !lightingSettings.specular; std::cout << "[SPECULAR HIGHLIGHTS: " << lightingSettings.specular << "]" << std::endl; }
		else if (event.key.keysym.sym == SDLK_SPACE) window.savePPM("img/output.ppm");
	}
}

int main(int argc, char *argv[]) {
	DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
	
	std::vector<Model> models;
	models.push_back(loadObjFile("hackspace-logo/textured-cornell-box.obj", 1.0));
	models.push_back(loadObjFile("hackspace-logo/sphere.obj", 1.0));
	models.push_back(loadObjFile("hackspace-logo/logo.obj", 0.005));
	translateModel(models, 1, glm::vec3(-0.5, 0.0, 0.0));
	translateModel(models, 2, glm::vec3(-0.25, -1.0, 3.0));

	glm::vec3 ambient(0.1);
	LightingSettings lightingSettings(true, true, true, true, true, true, true, 1.0, 256, ambient);
	std::vector<Light> lights;
	lights.push_back(Light(glm::vec3(1.0, 2.0, 4.0), 1, 50, Colour(128, 255, 128)));
	lights.push_back(Light(glm::vec3(1.0, 0.0, 0.0), 1, 50, Colour(255, 128, 128)));
	lights.push_back(Light(glm::vec3(-1.0, -1.0, 1.0), 1, 50, Colour(128, 128, 255)));

	glm::mat4 camera(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 10.0, 1.0
	);
	float focalLength = 2.0;
	float planeMultiplier = 250;
	int renderMode = 1;

	bool drawBool = true;
	SDL_Event event;
	while (true) {
		if (window.pollForInputEvents(event)) {
			handleEvent(event, window, camera, renderMode, lightingSettings, drawBool);
			if (drawBool) {
				if (renderMode == 2) drawRaytraced(window, camera, focalLength, planeMultiplier, models, lights, lightingSettings);
				else draw(window, renderMode, camera, focalLength, planeMultiplier, models);
				drawBool = false;
			}
		}
		window.renderFrame();
	}
}
