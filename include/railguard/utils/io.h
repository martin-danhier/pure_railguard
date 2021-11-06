#pragma once

#include <railguard/utils/string.h>

#include <stdbool.h>
#include <stddef.h>

bool rg_load_file_binary(rg_string file_name, void** data, size_t* size);