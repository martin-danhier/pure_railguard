#pragma once

namespace railguard::rendering
{
    class RenderDevice
    {
      public:
        /**
         * @brief Wait until the current frame has finished rendering.
         */
        virtual void WaitForCurrentFrame() = 0;
    };
} // namespace railguard::rendering