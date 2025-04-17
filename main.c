#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "display.h"
#include "main.h"

#define MEMORY_SIZE 4096
#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

uint8_t memory[MEMORY_SIZE];
uint8_t V[16];         // Registers V0 to VF
uint16_t I;            // Index register
uint16_t pc;           // Program counter
uint16_t stack[16];
uint8_t sp;            // Stack pointer
uint8_t delay_timer;
uint8_t sound_timer;
uint8_t gfx[VIDEO_WIDTH * VIDEO_HEIGHT];
uint8_t keypad[16];

SDL_Window* window;
SDL_Renderer* renderer;


void initialize() {
    pc = 0x200; // Programs start at 0x200
    I = 0;
    sp = 0;
    delay_timer = 0;
    sound_timer = 0;
    memset(memory, 0, MEMORY_SIZE);
    memset(V, 0, sizeof(V));
    memset(stack, 0, sizeof(stack));
    memset(gfx, 0, sizeof(gfx));
    memset(keypad, 0, sizeof(keypad));

    // Load fontset into memory
    for (int i = 0; i < 80; ++i) {
        memory[0x050 + i] = chip8_fontset[i];
    }
}

void load_rom(const char* filename) {
    FILE* rom = fopen(filename, "rb");
    if (!rom) {
        perror("Error loading ROM");
        exit(1);
    }
    fread(&memory[pc], 1, MEMORY_SIZE - pc, rom);
    fclose(rom);
}

void emulate_cycle() {
    uint16_t opcode = memory[pc] << 8 | memory[pc + 1];
    pc += 2;

    uint8_t x, nn, y;
    switch (opcode & 0xF000) {
        case 0x0000:
            if (opcode == 0x00E0) {
                memset(gfx, 0, sizeof(gfx));
            } else if (opcode == 0x00EE) {
                sp--;
                pc = stack[sp];
            }
            break;
        case 0x1000:
            pc = opcode & 0x0FFF;
            break;
        case 0x2000:
            stack[sp] = pc;
            sp++;
            pc = opcode & 0x0FFF;
            break;
        case 0x3000:
            x = (opcode & 0x0F00) >> 8;
            nn = opcode & 0x00FF;
            if (V[x] == nn) {
                pc += 2;
            }
            break;
        case 0x4000:
            x = (opcode & 0x0F00) >> 8;
            nn = opcode & 0x00FF;
            if (V[x] != nn) {
                pc += 2;
            }
            break;
        case 0x5000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            if (V[x] == V[y]) {
                pc += 2;
            }
            break;
        case 0x6000:
            x = (opcode & 0x0F00) >> 8;
            nn = opcode & 0x00FF;
            V[x] = nn;
            break;
        case 0x7000:
            x = (opcode & 0x0F00) >> 8;
            nn = opcode & 0x00FF;
            V[x] += nn;
            break;
        case 0x8000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            switch (opcode & 0x000F) {
                case 0x0:
                    V[x] = V[y];
                    break;
                case 0x1:
                    V[x] |= V[y];
                    break;
                case 0x2:
                    V[x] &= V[y];
                    break;
                case 0x3:
                    V[x] ^= V[y];
                    break;
                case 0x4:
                    V[x] += V[y];
                    V[0xF] = (V[x] < V[y]) ? 1 : 0;
                    break;
                case 0x5:
                    V[0xF] = (V[x] > V[y]) ? 1 : 0;
                    V[x] -= V[y];
                    break;
                case 0x6:
                    V[0xF] = V[x] & 0x1;
                    V[x] >>= 1;
                    break;
                case 0x7:
                    V[0xF] = (V[y] > V[x]) ? 1 : 0;
                    V[x] = V[y] - V[x];
                    break;
                case 0x8:
                    V[0xF] = V[x] >> 7;
                    V[x] <<= 1;
                    break;
                default:
                    break;
            }
            break;
        case 0x9000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            if (V[x] != V[y]) {
                pc += 2;
            }
            break;
        case 0xA000:
            I = opcode & 0x0FFF;
            break;
        case 0xB000:
            pc = (opcode & 0x0FFF) + V[0];
            break;
        case 0xC000:
            x = (opcode & 0x0F00) >> 8;
            nn = opcode & 0x00FF;
            V[x] = (rand() % 256) & nn;
            break;
        case 0xD000:
            x = (opcode & 0x0F00) >> 8;
            y = (opcode & 0x00F0) >> 4;
            uint8_t N = opcode & 0x000F;

            uint8_t xCord = V[x] % VIDEO_WIDTH;
            uint8_t yCord = V[y] % VIDEO_HEIGHT;

            V[0xF] = 0;

            for (int row = 0; row < N; row++) {
                if (yCord + row >= VIDEO_HEIGHT) break;

                uint8_t spriteByte = memory[I + row];

                for (int col = 0; col < 8; col++) {
                    if (xCord + col >= VIDEO_WIDTH) break;

                    uint8_t spritePixel = (spriteByte >> (7 - col)) & 1;
                    int index = (xCord + col) + (yCord + row) * VIDEO_WIDTH;

                    if (spritePixel == 1) {
                        if (gfx[index] == 1) {
                            V[0xF] = 1;
                        }
                        gfx[index] ^= spritePixel;
                    }
                }
            }
            break;
        case 0xE000:
            if ((opcode & 0x00FF) == 0x009E) {
                x = (opcode & 0x0F00) >> 8;
                if (keypad[V[x]] != 0) {
                    pc += 2;
                }
            } else if ((opcode & 0x00FF) == 0x00A1) {
                x = (opcode & 0x0F00) >> 8;
                if (keypad[V[x]] == 0) {
                    pc += 2;
                }
            }
            break;
        case 0xF000:
            x = (opcode & 0x0F00) >> 8;
            switch (opcode & 0x00FF) {
                case 0x07:
                    V[x] = delay_timer;
                    break;
                case 0x0A: {
                    bool key_pressed = false;
                    while (!key_pressed) {
                        SDL_Event event;
                        while (SDL_PollEvent(&event)) {
                            if (event.type == SDL_EVENT_KEY_DOWN) {
                                for (int i = 0; i < 16; i++) {
                                    if (event.key.scancode == SDL_SCANCODE_1 + i || 
                                        (i == 0 && event.key.scancode == SDL_SCANCODE_0)) {
                                        V[x] = i;
                                        key_pressed = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if (delay_timer > 0) {
                            delay_timer--;
                        }
                        if (sound_timer > 0) {
                            sound_timer--;
                        }
                        SDL_Delay(16);
                    }
                    break;
                }
                case 0x15:
                    delay_timer = V[x];
                    break;
                case 0x18:
                    sound_timer = V[x];
                    break;
                case 0x1E:
                    I += V[x];
                    break;
                case 0x29:
                    I = V[x] * 5;
                    break;
                case 0x33:
                    memory[I] = V[x] / 100;
                    memory[I + 1] = (V[x] / 10) % 10;
                    memory[I + 2] = V[x] % 10;
                    break;
                case 0x55:
                    for (int i = 0; i <= x; i++) {
                        memory[I + i] = V[i];
                    }
                    break;
                case 0x65:
                    for (int i = 0; i <= x; i++) {
                        V[i] = memory[I + i];
                    }
                    break;
            }
            break;
        default:
            fprintf(stderr, "Unknown opcode: 0x%X\n", opcode);
            exit(1);
            break;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s romfile\n", argv[0]);
        return 1;
    }

    SDL_Init(SDL_INIT_VIDEO);
    initialize();
    load_rom(argv[1]);

    startDisplay(&window, &renderer);
    if (!window || !renderer) {
        fprintf(stderr, "Failed to initialize display: %s\n", SDL_GetError());
        SDL_Quit();
        return EXIT_FAILURE;
    }

    bool running = true;
    SDL_Event event;

    const int FPS = 60;
    const int CYCLES_PER_FRAME = 10;
    const int FRAME_DELAY = 1000 / FPS;

    uint32_t last_time = SDL_GetTicks();
    uint32_t timer_time = last_time;

    while (running) {
        uint32_t current_time = SDL_GetTicks();
        uint32_t elapsed_time = current_time - last_time;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                for (int i = 0; i < 16; i++) {
                    if ((i == 0 && event.key.scancode == SDL_SCANCODE_X) || 
                        (i == 1 && event.key.scancode == SDL_SCANCODE_1) || 
                        (i == 2 && event.key.scancode == SDL_SCANCODE_2) || 
                        (i == 3 && event.key.scancode == SDL_SCANCODE_3) || 
                        (i == 4 && event.key.scancode == SDL_SCANCODE_Q) || 
                        (i == 5 && event.key.scancode == SDL_SCANCODE_W) || 
                        (i == 6 && event.key.scancode == SDL_SCANCODE_E) || 
                        (i == 7 && event.key.scancode == SDL_SCANCODE_A) || 
                        (i == 8 && event.key.scancode == SDL_SCANCODE_S) || 
                        (i == 9 && event.key.scancode == SDL_SCANCODE_D) || 
                        (i == 10 && event.key.scancode == SDL_SCANCODE_Z) || 
                        (i == 11 && event.key.scancode == SDL_SCANCODE_C) || 
                        (i == 12 && event.key.scancode == SDL_SCANCODE_4) || 
                        (i == 13 && event.key.scancode == SDL_SCANCODE_R) || 
                        (i == 14 && event.key.scancode == SDL_SCANCODE_F) || 
                        (i == 15 && event.key.scancode == SDL_SCANCODE_V)) {
                        keypad[i] = (event.type == SDL_EVENT_KEY_DOWN) ? 1 : 0;
                    }
                }
            }
        }

        for (int i = 0; i < CYCLES_PER_FRAME; i++) {
            emulate_cycle();
        }

        if (current_time - timer_time >= 1000 / 60) {
            if (delay_timer > 0) delay_timer--;
            if (sound_timer > 0) sound_timer--;
            timer_time = current_time;
        }

        renderDisplay(renderer, gfx);

        uint32_t frame_time = SDL_GetTicks() - current_time;
        if (frame_time < FRAME_DELAY) {
            SDL_Delay(FRAME_DELAY - frame_time);
        }

        last_time = current_time;
    }

    stopDisplay(renderer, window);
    SDL_Quit();
    return 0;
}


