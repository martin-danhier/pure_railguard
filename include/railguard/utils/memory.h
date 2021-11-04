#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

// Use this module to short circuit the use of malloc, free, etc.
// This way, in unit tests, we can keep track of what is still allocated, and detect memory leaks.

#ifndef MEMORY_CHECKS

/**
 * @brief Allocates memory, but does not initialize it.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
#define rg_malloc malloc

/**
 * @brief Allocates memory, and sets the memory to zero.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
#define rg_calloc calloc

/**
 * @brief Reallocates memory.
 * @param ptr The pointer to the memory to reallocate.
 * @param size The new size of the memory.
 * @return A pointer to the reallocated memory, or NULL if the reallocation failed.
 */
#define rg_realloc realloc

/**
 * @brief Frees memory.
 * @param ptr The pointer to the memory to free.
 */
#define rg_free free

#else

// --=== Memory watcher ===--

/**
 * @brief Sets up the memory watcher. Call this before using any of the allocation functions.
 * @note Only defined if MEMORY_CHECKS is defined.
 */
bool rg_mem_watcher_init(void);

/**
 * @brief Cleans up the memory watcher. Call this after using all of the allocation functions.
 * @note Only defined if MEMORY_CHECKS is defined.
 */
void rg_mem_watcher_cleanup(void);

/**
 * @brief Print all the allocations that are still allocated and the segfaults that were prevented.
 * @note Only defined if MEMORY_CHECKS is defined.
 * @return false if there are allocations that are still allocated, true otherwise.
 */
bool rg_mem_watcher_print_leaks(void);

// --=== Macros ===--

/**
 * @brief Allocates memory, but does not initialize it.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
#define rg_malloc(size)        rg_mem_watcher_malloc(size, __FILE__, __LINE__)

/**
 * @brief Allocates memory, and sets the memory to zero.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
#define rg_calloc(count, size) rg_mem_watcher_calloc(count, size, __FILE__, __LINE__)

/**
 * @brief Reallocates memory.
 * @param ptr The pointer to the memory to reallocate.
 * @param size The new size of the memory.
 * @return A pointer to the reallocated memory, or NULL if the reallocation failed.
 */
#define rg_realloc(ptr, size)  rg_mem_watcher_realloc(ptr, size, __FILE__, __LINE__)

/**
 * @brief Frees memory.
 * @param ptr The pointer to the memory to free.
 */
#define rg_free(ptr)           rg_mem_watcher_free(ptr, __FILE__, __LINE__)

// --=== Override memory functions ===--

void *rg_mem_watcher_malloc(size_t size, const char *file, size_t line);

void *rg_mem_watcher_calloc(size_t count, size_t size, const char *file, size_t line);

void *rg_mem_watcher_realloc(void *ptr, size_t size, const char *file, size_t line);

void rg_mem_watcher_free(void *ptr, const char *file, size_t line);

#endif
