#include <railguard/rendering/Window.h>
#include <SDL2/SDL.h>

namespace railguard::rendering {
    Window::Window()
    {
        // Create a window
        SDL_WindowFlags windowFlags = SDL_WINDOW_VULKAN;

        _window = SDL_CreateWindow(_title.data,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   static_cast<int32_t>(_width),
                                   static_cast<int32_t>(_height),
                                   windowFlags);
        SDL_SetWindowResizable(_window, SDL_TRUE);

        if (_window == nullptr)
        {
            HandleError();
        }
    }
}