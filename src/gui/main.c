#include <SDL2/SDL.h>
#include <chip8.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else

#define EMSCRIPTEN_KEEPALIVE
static bool quit = false;
static void emscripten_set_main_loop(void (*func)(void), int x, int y) {
    while (!quit) {
        func();
    }
}
static void emscripten_cancel_main_loop() { quit = true; }
#endif

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;

static Chip8 c8 = {0};

static bool running = false;

#define SCREEN_SCALE 16
#define SCREEN_WIDTH (SCREEN_SCALE * CHIP8_SCREEN_WIDTH)
#define SCREEN_HEIGHT (SCREEN_SCALE * CHIP8_SCREEN_HEIGHT)

static bool init(void) {
    srand(time(NULL));
    chip8_reset(&c8);

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not be initiliazed. SDL_Error: %s\n", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("emscripten chip8", SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
                              SCREEN_HEIGHT, SDL_WINDOW_SHOWN);

    if (!window) {
        printf("SDL_Window could not be initialized. SDL_Error: %s\n",
               SDL_GetError());
        return false;
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    return true;
}

void quit_game(void);

void draw_square(int x, int y, int fill) {
    const SDL_Rect rect = {.x = x * SCREEN_SCALE,
                           .y = y * SCREEN_SCALE,
                           .w = (SCREEN_SCALE - 1),
                           .h = (SCREEN_SCALE - 1)};

    fill = fill ? 255 : 0;

    SDL_SetRenderDrawColor(renderer, 0, fill, 0, 255);
    SDL_RenderFillRect(renderer, &rect);
}

/**
CHIP8:
1 2 3 C
4 5 6 D
7 8 9 E
A 0 C F

QWERTY:
1 2 3 4
Q W E R
A S D F
Z X C V
*/
static int key_index(const SDL_Keycode kc) {
    switch (kc) {
        case SDLK_x:
            return 0;
        case SDLK_1:
            return 1;
        case SDLK_2:
            return 2;
        case SDLK_3:
            return 3;
        case SDLK_q:
            return 4;
        case SDLK_w:
            return 5;
        case SDLK_e:
            return 6;
        case SDLK_a:
            return 7;
        case SDLK_s:
            return 8;
        case SDLK_d:
            return 9;
        case SDLK_z:
            return 10;
        case SDLK_c:
            return 11;
        case SDLK_4:
            return 12;
        case SDLK_r:
            return 13;
        case SDLK_f:
            return 14;
        case SDLK_v:
            return 15;
        default:
            return -1;
    }
}

static void handle_events() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            quit_game();
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            const int ki = key_index(e.key.keysym.sym);
            if (-1 != ki) {
                c8.key[ki] = (e.type == SDL_KEYDOWN) ? 1 : 0;
            }
        }
    }
}

void quit_game(void) {
    SDL_DestroyWindow(window);
    window = NULL;

    SDL_DestroyRenderer(renderer);
    renderer = NULL;

    SDL_Quit();

    emscripten_cancel_main_loop();
}

void main_loop(void) {
    handle_events();

    if (!running) return;

    for (int i = 0; i < 100; i++) {
        const CHIP8_ERROR err = chip8_cycle(&c8);
        if (CHIP8_ERROR_NONE != err) {
            printf("chip8_cycle err %u\n", err);
        }
    }

    SDL_SetRenderDrawColor(renderer, 0, 32, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(renderer);

    for (int x = 0; x < 64; x++) {
        for (int y = 0; y < 32; y++) {
            draw_square(x, y, c8.video[y][x]);
        }
    }

    SDL_RenderPresent(renderer);
}

EMSCRIPTEN_KEEPALIVE
bool jscallback(uint8_t *data, size_t length) {
    printf("loading rom, length %lu b\n", length);
    if (CHIP8_ERROR_NONE == chip8_load_rom_data(&c8, data, length)) {
        running = true;
        return true;
    }
    printf("failed to load romn\n");
    return false;
}

EMSCRIPTEN_KEEPALIVE
int main(int argc, char *argv[]) {
    printf("built %s @ %s\n", __DATE__, __TIME__);

    if (!init()) return -1;

    emscripten_set_main_loop(main_loop, 0, 1);

    quit_game();
    return 0;
}
