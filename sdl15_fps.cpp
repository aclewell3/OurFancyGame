#include <iostream>
#include <vector>
#include <string>
#include <SDL.h>
#include <SDL_image.h>

constexpr int CAM_WIDTH = 640;
constexpr int CAM_HEIGHT = 480;
constexpr int LEVEL_LEN = 2000;
constexpr int TILE_SIZE = 100;

// Going to implement frame-independent movement
// Old speed limit was 5px/frame @60fps, so 300px/sec
constexpr double SPEED_LIMIT = 300.0;
// Old acceleration was 1px/frame^2 @60fps, so 3600px/s^2
// Both will accelerate to max speed in 1/12s
constexpr double ACCEL = 3600.0;

// Function declarations
bool init();
SDL_Texture* loadImage(std::string fname);
void close();

// Globals
SDL_Window* gWindow = nullptr;
SDL_Renderer* gRenderer = nullptr;
SDL_Texture* gBackground;
SDL_Texture* gPlayerSheet;
SDL_Texture* gBrickSheet;

bool init() {	
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		std::cout << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
		return false;
	}

	if(!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1")) {
		std::cout << "Warning: Linear texture filtering not enabled!" << std::endl;
	}
	
	gWindow = SDL_CreateWindow("Calculate FPS", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, CAM_WIDTH, CAM_HEIGHT, SDL_WINDOW_SHOWN);
	if (gWindow == nullptr) {
		std::cout << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return  false;
	}

	// Try both with/without SDL_RENDERER_PRESENTVSYNC
	//gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	gRenderer = SDL_CreateRenderer(gWindow, -1, SDL_RENDERER_ACCELERATED);	
	if (gRenderer == nullptr) {	
		std::cout << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		return  false;
	}

	int imgFlags = IMG_INIT_PNG;
	int retFlags = IMG_Init(imgFlags);
	if(retFlags != imgFlags) {
		std::cout << "SDL_image could not initialize! SDL_image Error: " << IMG_GetError() << std::endl;
		return false;
	}

	SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
			
	return true;
}

SDL_Texture* loadImage(std::string fname) {
	SDL_Texture* newText = nullptr;

	SDL_Surface* startSurf = IMG_Load(fname.c_str());
	if (startSurf == nullptr) {
		std::cout << "Unable to load image " << fname << "! SDL Error: " << SDL_GetError() << std::endl;
		return nullptr;
	}

	newText = SDL_CreateTextureFromSurface(gRenderer, startSurf);
	if (newText == nullptr) {
		std::cout << "Unable to create texture from " << fname << "! SDL Error: " << SDL_GetError() << std::endl;
	}

	SDL_FreeSurface(startSurf);

	return newText;
}


void close() {
	SDL_DestroyRenderer(gRenderer);
	SDL_DestroyWindow(gWindow);
	gWindow = nullptr;
	gRenderer = nullptr;

	SDL_DestroyTexture(gBackground);
	gBackground = nullptr;

	SDL_DestroyTexture(gPlayerSheet);
	gPlayerSheet = nullptr;

	SDL_DestroyTexture(gBrickSheet);
	gBrickSheet = nullptr;

	// Quit SDL subsystems
	SDL_Quit();
}

int main() {
	if (!init()) {
		std::cout <<  "Failed to initialize!" << std::endl;
		close();
		return 1;
	}
	
	gBackground = loadImage("images/small_bg.png");

	int scroll_offset = 0;
	int rem = 0;
	int c = 0;

	gPlayerSheet = loadImage("images/walking.png");
	gBrickSheet = loadImage("images/bricks.png");

	// NOTE: these now need to be fractional
	// speed/accel are expressed as per s, will be dealing with fractions of a s
	double x_vel = 0.0;
	double x_deltav = 0.0;
	// Can't use ints to track position
	// Otherwise, will never move to the right, due to constant int division
	// Can't add on the fractional amount of movement
	// Hence, no longer need move box
	double x_coord = CAM_WIDTH/4 - TILE_SIZE/2;
	double y_coord = CAM_HEIGHT - (TILE_SIZE * 2);

	SDL_RendererFlip flip = SDL_FLIP_NONE;

	int lthird = (CAM_WIDTH / 3) - TILE_SIZE/2;
	int rthird = (2 * CAM_WIDTH / 3) - TILE_SIZE/2;

	SDL_Rect playercam = {CAM_WIDTH/2 - TILE_SIZE/2, CAM_HEIGHT/2 - TILE_SIZE/2, TILE_SIZE, TILE_SIZE};
	SDL_Rect playerRect = {0, 0, TILE_SIZE, TILE_SIZE};

	int frames = 0;
	int frame_count = 0;

	SDL_Rect bgRect = {0, 0, CAM_WIDTH, CAM_HEIGHT};
	SDL_Rect brickcam = {0, CAM_HEIGHT - TILE_SIZE, TILE_SIZE, TILE_SIZE};
	SDL_Rect brickRects[4];
	for (int i = 0; i < 4; i++) {
		brickRects[i].x = i * TILE_SIZE;
		brickRects[i].y = 0;
		brickRects[i].w = TILE_SIZE;
		brickRects[i].h = TILE_SIZE;
	}

	// Keep track of time
	Uint32 fps_last_time = SDL_GetTicks();
	Uint32 fps_cur_time = 0;
	Uint32 move_last_time = SDL_GetTicks();
	Uint32 anim_last_time = SDL_GetTicks();
	double timestep = 0;

	SDL_Event e;
	bool gameon = true;
	while(gameon) {
		while(SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) {
				gameon = false;
			}
		}
		
		// figure out how much of a second has passed
		timestep = (SDL_GetTicks() - move_last_time) / 1000.0;
		x_deltav = 0.0;
		const Uint8* keystate = SDL_GetKeyboardState(nullptr);
		if (keystate[SDL_SCANCODE_A])
			x_deltav -= (ACCEL * timestep);
		if (keystate[SDL_SCANCODE_D])
			x_deltav += (ACCEL * timestep);

		if (x_deltav == 0) {
			if (x_vel > 0) {
				// Need to actually coverge back at 0
				if (x_vel < (ACCEL * timestep))
					x_vel = 0;
				else
					x_vel -= (ACCEL * timestep);
			}
			else if (x_vel < 0) {
				if (-x_vel < (ACCEL * timestep))
					x_vel = 0;
				else
					x_vel += (ACCEL * timestep);
			}
		}
		else
			x_vel += x_deltav;

		//std::cout << "x_deltav: " << x_deltav << std::endl;

		if (x_vel < -SPEED_LIMIT)
			x_vel = -SPEED_LIMIT;
		else if (x_vel > SPEED_LIMIT)
			x_vel = SPEED_LIMIT;

		//std::cout << "x_vel: " << x_vel << std::endl;
		//std::cout << "timestep: " << timestep << std::endl;
		//std::cout << "x diff: " << x_vel * timestep << std::endl;

		x_coord += (x_vel * timestep);
		// Using "just go back" approach to collision resolution becomes trickier with variable timesteps...
		if (x_coord < 0)
			x_coord = 0;
		if (x_coord + TILE_SIZE > LEVEL_LEN)
			x_coord = LEVEL_LEN - TILE_SIZE;
			
		move_last_time = SDL_GetTicks();

		if ((int) x_coord > (scroll_offset + rthird))
			scroll_offset = (int) x_coord - rthird;
		else if ((int) x_coord < (scroll_offset + lthird))
			scroll_offset = (int) x_coord - lthird;
		if (scroll_offset < 0)
			scroll_offset = 0;
		if (scroll_offset + CAM_WIDTH > LEVEL_LEN)
			scroll_offset = LEVEL_LEN - CAM_WIDTH;

		SDL_SetRenderDrawColor(gRenderer, 0x00, 0x00, 0x00, 0xFF);
		SDL_RenderClear(gRenderer);
		
		rem = scroll_offset % CAM_WIDTH;
		bgRect.x = -rem;
		SDL_RenderCopy(gRenderer, gBackground, nullptr, &bgRect);
		bgRect.x += CAM_WIDTH;
		SDL_RenderCopy(gRenderer, gBackground, nullptr, &bgRect);

		rem = scroll_offset % TILE_SIZE;
		brickcam.x = -rem;
		c = (scroll_offset % (TILE_SIZE * 4)) / TILE_SIZE;
		while (brickcam.x < CAM_WIDTH) {
			SDL_RenderCopy(gRenderer, gBrickSheet, &brickRects[c], &brickcam);

			brickcam.x += TILE_SIZE;
			c = (c + 1) % 4;
		}

		// Animation processing was frame-dependent as well
		// Need to change that to be time dependent
		if (x_vel != 0) {
			// Note that this decreases input lag!
			if (SDL_GetTicks() - anim_last_time > 100) {
				frames = (frames + 1) % 4;
				anim_last_time = SDL_GetTicks();
			}

			playerRect.x = frames * 100;
		}
		if (x_vel > 0 && flip == SDL_FLIP_HORIZONTAL)
			flip = SDL_FLIP_NONE;
		else if (x_vel < 0 && flip == SDL_FLIP_NONE)
			flip = SDL_FLIP_HORIZONTAL;

		playercam.x = (int) x_coord - scroll_offset;
		playercam.y = (int) y_coord;
		SDL_RenderCopyEx(gRenderer, gPlayerSheet, &playerRect, &playercam, 0.0, nullptr, flip);

		SDL_RenderPresent(gRenderer);

		// Possibly computer/output FPS
		frame_count++;
		fps_cur_time = SDL_GetTicks();
		// Only output every 10s
		if (fps_cur_time - fps_last_time > 10000) {
			std::cout << "Average FPS:  " << frame_count / ((fps_cur_time - fps_last_time) / 1000.0) << std::endl;

			// reset
			fps_last_time = fps_cur_time;
			frame_count = 0;
		}
	}

	close();
}
