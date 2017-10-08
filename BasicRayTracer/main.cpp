#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "Timer.h"
#include "Framebuffer.h"
#include "Scene.h"
#include "Renderer.h"
#include "Camera.h"
#include <iostream>
#include "ctpl_stl.h"
#include <mutex>

// Arguments
#define IMAGE_WIDTH		800
#define IMAGE_HEIGHT	800
#define SCENE_PATH "../Scenes/test1.scene"
#define OUTPUT_NAME "phasepositive.bmp"
#define NUM_THREADS 4
#define SAMPLES_PER_PIXEL 1	// Must be a power of 2

// Depth of Field Arguments
#define FOCAL_LENGTH 12.0f
// DoF can be disabled by setting radius to 0
// For reference, distance from image to lens ('m') is 1.0f
#define LENS_RADIUS 0.0f 

// Multithreading
// Image will be broken into TILE_SIZE x TILE_SIZE blocks
#define TILE_SIZE 32
// The number of tiles that have been rendered
int NumTilesRendered = 0;
// Mutex to protect NumTilesRendered because each thread will increment it when they finish
std::mutex tilesRenderedMutex;
// The total number of tiles in the image
int NumTiles = 0;

// Defines a region of the screen to be rendered
struct Tile {
	int min_x, max_x;
	int min_y, max_y;
};

// Renders a portion of the image
void renderTile(int id, Renderer* renderer, FrameBuffer* fb, Tile tile) {
	//iterate over all pixels in this region
	for (int j = tile.min_y; j < tile.max_y; j++) {
		for (int i = tile.min_x; i < tile.max_x; i++) {	
			//trace ray
			Vector3 color;
			renderer->ColorPixel(i, j, color);

			//quantize and save color to framebuffer
			fb->getPixelPtr(i, j)[0] = (u08)(255.0f * max(min(color.x, 1.0f), 0.0f));
			fb->getPixelPtr(i, j)[1] = (u08)(255.0f * max(min(color.y, 1.0f), 0.0f));
			fb->getPixelPtr(i, j)[2] = (u08)(255.0f * max(min(color.z, 1.0f), 0.0f));
		}
	}

	//update percent complete
	//grab mutex
	std::lock_guard<std::mutex> guard(tilesRenderedMutex);
	//increase count and display
	NumTilesRendered++;
	printf("\r%.2f%% complete", (NumTilesRendered/(float)NumTiles) * 100.0f);
	//mutex is automatically released when guard goes out of scope
}

int main(int argc, char *argv[]) {
	//start timer
	Timer total_timer;
	total_timer.startTimer();

	//load scene data
	printf("Loading scene data...\n");
	Scene scene(SCENE_PATH, IMAGE_WIDTH, IMAGE_HEIGHT, FOCAL_LENGTH, LENS_RADIUS);
	printf("Scene loaded.\n");

	//create some BSSRDFs
	BSSRDF* chicken = new BSSRDF(Vector3(0.018, 0.088, 0.20), Vector3(0.19, 0.25, 0.32), 0.0, 1.3);
	BSSRDF* potato = new BSSRDF(Vector3(0.0024, 0.0090, 0.12), Vector3(0.68, 0.70, 0.55), 0.0, 1.3);
	BSSRDF* skin = new BSSRDF(Vector3(0.032, 0.17, 0.48), Vector3(0.74, 0.88, 1.01), 0.0f, 1.3);
	BSSRDF* marble = new BSSRDF(Vector3(0.0021, 0.0041, 0.0071), Vector3(2.19,2.62, 3.0), 0.0f, 1.5);
	BSSRDF* apple = new BSSRDF(Vector3(0.0030, 0.0034, 0.046), Vector3(2.29, 2.39, 1.97), 0.0f, 1.3);
	BSSRDF* ketchup = new BSSRDF(Vector3(0.0061, 0.97, 1.45), Vector3(0.18, 0.07, 0.03), 0.0f, 1.3);
	
	// Settings for 'scene5a.bmp'
	//scene.SetObjectBSSRDF(0, apple);
	//scene.SetObjectBSSRDF(1, potato);
	//scene.SetObjectBSSRDF(2, chicken);
	//scene.SetObjectBSSRDF(4, marble);
	//scene.SetObjectBSSRDF(5, ketchup);
	//scene.RemoveObject(3);

	// Settings for 'skin1.bmp'
	//scene.SetObjectBSSRDF(0, skin);
	//scene.SetObjectBSSRDF(1, marble);
	//remove lights
	//scene.lights.erase(scene.lights.begin() + 1);
	//scene.lights.erase(scene.lights.begin() + 1);

	// Settings for 'skin2.bmp'
	scene.SetObjectBSSRDF(0, skin);
	scene.SetObjectBSSRDF(1, marble);
	//remove lights
	scene.lights.erase(scene.lights.begin() + 1);
	scene.lights.erase(scene.lights.begin() + 1);
	//move remaining light
	scene.lights[0] = new PointLightSource(Vector3(-1.8464, 1.378452, -3.6750));
	//make light more powerful
	scene.lights[0]->color = Vector3(2, 2, 2);

	// Settings for 'skin3.bmp'
	//scene.SetObjectBSSRDF(0, skin);
	//scene.SetObjectBSSRDF(1, marble);
	//remove lights
	//scene.lights.erase(scene.lights.begin() + 1);
	//scene.lights.erase(scene.lights.begin() + 1);
	//make light more powerful
	//scene.lights[0]->color = Vector3(2, 2, 2);

	//create renderer
	Renderer renderer(&scene, SAMPLES_PER_PIXEL);

	//create image buffer
	FrameBuffer frameBuffer(IMAGE_WIDTH, IMAGE_HEIGHT);

	Timer render_timer;
	render_timer.startTimer();

	//create thread pool
	ctpl::thread_pool pool(NUM_THREADS);
	
	//divide image up into tiles
	std::vector<Tile> tiles;
	for (int x = 0; x < IMAGE_WIDTH; x += TILE_SIZE) {
		for (int y = 0; y < IMAGE_HEIGHT; y += TILE_SIZE) {
			Tile tile;
			tile.min_x = x;
			tile.min_y = y;
			tile.max_x = min(x + TILE_SIZE, IMAGE_WIDTH);
			tile.max_y = min(y + TILE_SIZE, IMAGE_HEIGHT);
			tiles.push_back(tile);
		}
	}

	//render all tiles
	NumTiles = tiles.size();
	NumTilesRendered = 0;
	printf("Rendering...\n");
	for (int i = 0; i < tiles.size(); i++) {
		//queue up a thread to render this tile
		pool.push(renderTile, &renderer, &frameBuffer, tiles[i]);
	}

	//wait for all threads to finish
	pool.stop(true);
	//all threads have ended
	std::cout << std::endl;
	render_timer.stopTimer();
	printf("Render time: %.5lf secs\n", render_timer.getTime());
	
	//save output
	printf("Saving to '%s'...\n", OUTPUT_NAME);
	frameBuffer.SaveToFile(OUTPUT_NAME);
	printf("Done.\n");
	
	total_timer.stopTimer();
	printf("Total time: %.5lf secs\n", total_timer.getTime());
	std::cin.get();
	return 0;
}
