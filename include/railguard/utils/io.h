#pragma once

#include <stddef.h>
#include <stdbool.h>

bool rg_load_file_binary(const char* file_name, void** data, size_t* size);