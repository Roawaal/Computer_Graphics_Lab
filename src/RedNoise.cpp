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

std::vector<TexturePoint> interpolatePoints(TexturePoint from, TexturePoint to, int steps) {
	std::vector<TexturePoint> TexturePoints;
	float xStep = (to.x - from.x) / (steps - 1);
	float yStep = (to.y - from.y) / (steps - 1);
	for (int i=0; i<steps; i++) {
		TexturePoints.push_back(TexturePoint(from.x + (i * xStep),  from.y + (i * yStep)));
	}
	return TexturePoints;
}

std::vector<CanvasPoint> interpolatePoints(CanvasPoint from, CanvasPoint to, int steps) {
	std::vector<CanvasPoint> canvasPoints;
	float xStep = (to.x - from.x) / (steps - 1);
	float yStep = (to.y - from.y) / (steps - 1);
	float dStep = (to.depth - from.depth) / (steps - 1);
	auto texturePoints = interpolatePoints(from.texturePoint, to.texturePoint, steps);
	for (int i=0; i<steps; i++) {
		CanvasPoint point(round(from.x + (i * xStep)),  round(from.y + (i * yStep)), from.depth + (i * dStep));
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
	frameNumber++;
	window.saveBMP("output" + frameNumber + ".bmp");    
    }
}
