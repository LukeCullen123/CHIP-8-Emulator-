#ifndef DISPLAY_H
#define DISPLAY_H

#include <SDL3/SDL.h>
#include <stdbool.h>

#define WIDTH 64
#define HEIGHT 32
#define SCALE 10

void startDisplay(SDL_Window **window, SDL_Renderer **renderer);
void renderDisplay(SDL_Renderer *renderer, uint8_t pixels[WIDTH * HEIGHT]);
void stopDisplay(SDL_Renderer *renderer, SDL_Window *window);
#endif // DISPLAY_H
