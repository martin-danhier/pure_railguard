#pragma once

namespace railguard
{
    struct String
    {
        char *data        = nullptr;
        unsigned int size = 0;

        char *CStr();
    };
} // namespace railguard