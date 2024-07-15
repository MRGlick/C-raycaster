#define SDL_MAIN_HANDLED

#include <SDL.h>
#include <SDL_gpu.h>
#include <stdio.h>

int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    // Create an SDL_gpu window and renderer
    GPU_Target* screen = GPU_Init(800, 600, GPU_DEFAULT_INIT_FLAGS);
    if (screen == NULL) {
        printf("Unable to initialize SDL_gpu: %s\n", GPU_GetErrorString(GPU_PopErrorCode().error));
        SDL_Quit();
        return 1;
    }

    // Load an image as a texture
    GPU_Image* image = GPU_LoadImage("Textures/vignette.png");
    if (image == NULL) {
        printf("Unable to load image: %s\n", GPU_GetErrorString(GPU_PopErrorCode().error));
        GPU_Quit();
        SDL_Quit();
        return 1;
    }

    // Main loop
    int running = 1;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
        }

        // Clear the screen
        GPU_Clear(screen);

        // Draw the image at the center of the screen
        GPU_Blit(image, NULL, screen, screen->w / 2, screen->h / 2);

        // Update the screen
        GPU_Flip(screen);
    }

    // Clean up
    GPU_FreeImage(image);
    GPU_Quit();
    SDL_Quit();

    return 0;
}