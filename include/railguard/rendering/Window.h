#pragma once

#include <cstdint>
#include <railguard/utils/Strings.h>

struct SDL_Window;

namespace railguard::rendering
{
    struct Extent2D {
        uint32_t width;
        uint32_t height;
    };

    class Window
    {
        SDL_Window *_window = nullptr;
        Extent2D _windowExtent;
        String _title;


      public:
        Window();
    };
} // namespace railguard::rendering