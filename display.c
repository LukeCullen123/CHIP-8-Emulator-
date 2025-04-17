#include "display.h"

#define WIDTH 64
#define HEIGHT 32
#define SCALE 10

void startDisplay(SDL_Window **window, SDL_Renderer **renderer) {
    SDL_Init(SDL_INIT_VIDEO);

    *window = SDL_CreateWindow(
        "64x32 Display", WIDTH * SCALE, HEIGHT * SCALE, 0
    );

    *renderer = SDL_CreateRenderer(*window, "opengl");
}

void renderDisplay(SDL_Renderer *renderer, uint8_t pixels[WIDTH * HEIGHT]) {
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);  // Black background
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);  // White pixels

    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            if (pixels[x + y * WIDTH]) {
                SDL_FRect rect = { x * (float)SCALE, y * (float)SCALE, (float)SCALE, (float)SCALE };
                SDL_RenderFillRect(renderer, &rect);  // Use SDL_FRect
            }
        }
    }

    SDL_RenderPresent(renderer);
}

void stopDisplay(SDL_Renderer *renderer, SDL_Window *window) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}