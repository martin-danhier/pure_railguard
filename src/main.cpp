// To prevent linking bugs, we handle the main wrapper of SDL ourselves.
// We define this constant to let it know
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <railguard/rendering/device/RenderDeviceVulkan.h>

int main() {
    // Init SDL2
    SDL_SetMainReady();
    SDL_Init(SDL_INIT_VIDEO);

    // Clean SDL
    SDL_Quit();
    return 0;
}