/**
 * This is a very early prototype of a flappy bird game that wil run on the PocketBoy.
 * It is using SDL2 for graphics so it can be developed on the PC first before deploying
 * it on the PocketBoy.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>

#define SCR_WIDTH 128
#define SCR_HEIGHT 64
#define REFRESH_RATE 40 // TODO: review this value. 30 may be more stable

// TODO: add passive buzzer sound and text rendering for score and game over screen

bool gFrameBuffer[SCR_HEIGHT * SCR_WIDTH] = { 0 };

SDL_Renderer* renderer;

bool bird[7 * 7] = {
	0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,
	1,1,1,1,1,1,1,
	0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,
	0,0,0,1,0,0,0,
};

typedef struct
{
	int offset;
	int holePos;
} Obstacle;

#define OBSTACLE_COUNT 3
#define OBSTACLE_HOLE_SIZE 13
#define OBSTACLE_SPACING 52
#define OBSTACLE_WIDTH 8
Obstacle obstacles[OBSTACLE_COUNT] = { 0 };

int birdHeight = SCR_HEIGHT / 2;

/**
 * @return true if any drawn pixel of the sprite overlapped a drawn pixel on the screen.
 * i.e. a pixel was meant to be drawn by the sprite but it's already drawn on the framebuffer.
 */
bool drawSprite(int inX, int inY, int inWidth, int inHeight, const bool* inData)
{
	bool anyPixelAlreadyDrawn = false;

	for (int y = 0; y < inHeight; y++)
	{
		for (int x = 0; x < inWidth; x++)
		{
			if (inData[y * inWidth + x])
			{
				int realX = (inX + x) - inWidth / 2;
				int realY = (inY + y) - inHeight / 2;

				if (realX < 0 || realY < 0 || realX >= SCR_WIDTH || realY >= SCR_HEIGHT)
					continue;

				if (gFrameBuffer[realY * SCR_WIDTH + realX])
					anyPixelAlreadyDrawn = true;

				gFrameBuffer[realY * SCR_WIDTH + realX] = true;
			}
		}
	}

	return anyPixelAlreadyDrawn;
}

void drawObstacle(const Obstacle* inObst)
{
	for (int y = 0; y < SCR_HEIGHT; y++)
	{
		if (abs(y - inObst->holePos) <= OBSTACLE_HOLE_SIZE)
			continue;

		for (int x = inObst->offset; x < OBSTACLE_WIDTH + inObst->offset; x++)
		{
			if (x < 0 || y < 0 || x >= SCR_WIDTH || y >= SCR_HEIGHT)
				continue;

			gFrameBuffer[y * SCR_WIDTH + x] = true;
		}
	}
}

int main(void)
{
	SDL_Init(SDL_INIT_VIDEO);

	SDL_Window* window = SDL_CreateWindow("HandheldConsole",
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCR_WIDTH * 3, SCR_HEIGHT * 3, 0);

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	SDL_RenderSetLogicalSize(renderer, SCR_WIDTH, SCR_HEIGHT);

	SDL_Event e;
	int running = 1;

	uint64_t curTime = SDL_GetPerformanceCounter();
	uint64_t lastTime = 0;
	float deltaTime = 0.0f;
	uint64_t frameID = 0;

	for (int i = 0; i < OBSTACLE_COUNT; i++)
	{
		Obstacle* o = &obstacles[i];
		o->offset = OBSTACLE_SPACING + (i * OBSTACLE_SPACING);
		o->holePos = rand() % SCR_HEIGHT - OBSTACLE_HOLE_SIZE - 7;
	}

	while (running)
	{
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				running = 0;
				break;

			case SDL_KEYDOWN:
				if (e.key.keysym.scancode == SDL_SCANCODE_UP)
				{
					birdHeight += 12;
					if (birdHeight > SCR_HEIGHT)
						birdHeight = SCR_HEIGHT;
				}

			default:
				break;
			}
		}

		lastTime = curTime;
		curTime = SDL_GetPerformanceCounter();
		deltaTime = (float)((curTime - lastTime) * 1000 / (float)SDL_GetPerformanceFrequency()) / 1000.0f;

		// bird falls one pixel every 2nd frame
		if (frameID % 2 == 0)
			birdHeight--;

		if (birdHeight == 0)
		{
			// TODO: game over
			return 0;
		}

		// obstacles move every 2nd frame
		if (frameID % 2 == 0)
		{
			for (int i = 0; i < OBSTACLE_COUNT; i++)
			{
				Obstacle* o = &obstacles[i];
				o->offset--;
			}

			const Obstacle* firstObst = &obstacles[0];
			if (firstObst->offset < 0 - OBSTACLE_WIDTH)
			{
				// shift obstacles backwards by one
				memmove(obstacles, obstacles + 1, sizeof(Obstacle) * (OBSTACLE_COUNT - 1));

				// new obstacle at the end
				const obstIdx = OBSTACLE_COUNT - 1;
				Obstacle* lastObst = &obstacles[obstIdx];
				Obstacle* secondLastObst = &obstacles[obstIdx - 1];
				lastObst->offset = secondLastObst->offset + OBSTACLE_SPACING;
				lastObst->holePos = rand() % (SCR_HEIGHT - OBSTACLE_HOLE_SIZE);
			}
		}

		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		for (int i = 0; i < SCR_WIDTH * SCR_HEIGHT; i++)
			gFrameBuffer[i] = 0;

		for (int i = 0; i < OBSTACLE_COUNT; i++)
		{
			Obstacle* o = &obstacles[i];
			drawObstacle(o);
		}

		bool collision = drawSprite(16, SCR_HEIGHT - birdHeight, 7, 7, bird);
		if (collision)
		{
			// TODO: game over
			return 0;
		}

		SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
		for (int y = 0; y < SCR_HEIGHT; y++)
			for (int x = 0; x < SCR_WIDTH; x++)
				if (gFrameBuffer[y * SCR_WIDTH + x])
					SDL_RenderDrawPoint(renderer, x, y);

		SDL_RenderPresent(renderer);

		// TODO: is this a good way to limit framerate ?
		const float kTargetDeltaTime = 1.0f / (float)REFRESH_RATE;
		while (deltaTime <= kTargetDeltaTime)
		{
			SDL_Delay(1);
			deltaTime += 0.001f;
		}

		frameID++;
	}

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
