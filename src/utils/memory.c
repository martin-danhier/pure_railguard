#include "railguard/utils/memory.h"

#ifdef MEMORY_CHECKS

#include <railguard/utils/maps.h>
#include <railguard/utils/storage.h>

#include <stdio.h>

// --=== Memory watcher ===--

// Types

typedef struct rg_mem_watcher_allocation
{
    const char *allocated_from_file;
    size_t      allocated_from_line;
    void       *base;
    size_t      size;
} rg_mem_watcher_allocation;

typedef struct rg_mem_watcher_segfault
{
    const char *freed_from_file;
    size_t      freed_from_line;
} rg_mem_watcher_segfault;

/**
 * @brief A structure that keeps track of allocated memory in order to detect memory leaks in debug mode.
 * @note Only defined if MEMORY_CHECKS is not defined.
 */
typedef struct rg_mem_watcher
{
    rg_struct_map *allocations;
    rg_storage *prevented_segfaults;
    // Disable watch when inside a watcher function to avoid infinite recursion.
    bool locked;
} rg_mem_watcher;

// Functions

rg_mem_watcher *RG_MEMORY_WATCHER = NULL;

bool rg_mem_watcher_init(void)
{
    // Do nothing if a watcher is already set up
    if (RG_MEMORY_WATCHER != NULL)
    {
        return true;
    }

    // Allocate memory for the watcher
    // Use a local variable until it is completely initialized
    // Otherwise the inner struct maps may cause problem, since they use the watcher's malloc functions
    rg_mem_watcher *watcher = malloc(sizeof(rg_mem_watcher));
    if (watcher == NULL)
    {
        return false;
    }

    // Initialize the allocations map
    watcher->allocations = rg_create_struct_map(sizeof(rg_mem_watcher_allocation));
    if (watcher->allocations == NULL)
    {
        free(watcher);
        return false;
    }

    // Initialize the prevented segfaults map
    watcher->prevented_segfaults = rg_create_storage(sizeof(rg_mem_watcher_segfault));
    if (watcher->prevented_segfaults == NULL)
    {
        rg_destroy_struct_map(&watcher->allocations);
        free(watcher);
        return false;
    }

    // Set the watcher
    RG_MEMORY_WATCHER = watcher;
    return true;
}

void rg_mem_watcher_cleanup(void)
{
    if (RG_MEMORY_WATCHER == NULL)
    {
        return;
    }

    // Take the watcher and prevent it from being used again
    rg_mem_watcher *watcher = RG_MEMORY_WATCHER;
    RG_MEMORY_WATCHER = NULL;

    // Cleanup the allocations map
    rg_destroy_struct_map(&watcher->allocations);

    // Cleanup the prevented segfaults map
    rg_destroy_storage(&watcher->prevented_segfaults);

    // Free the watcher
    free(watcher);
}

bool rg_mem_watcher_print_leaks(void)
{
    if (RG_MEMORY_WATCHER == NULL)
    {
        return true;
    }

    // Get the number of allocations and the number of prevented segfaults
    size_t allocations_count         = rg_struct_map_count(RG_MEMORY_WATCHER->allocations);
    size_t prevented_segfaults_count = rg_storage_count(RG_MEMORY_WATCHER->prevented_segfaults);

    // If everything is clean, do not do anything
    if (allocations_count == 0 && prevented_segfaults_count == 0)
    {
        return true;
    }

    // There are still un-freed allocations, print them
    if (allocations_count > 0)
    {
        printf("\n\n[MEMORY WATCHER]: Some allocations weren't freed !\n\n");

        rg_struct_map_it it = rg_struct_map_iterator(RG_MEMORY_WATCHER->allocations);
        while (rg_struct_map_next(&it))
        {
            rg_mem_watcher_allocation *allocation = it.value;

            // Print the allocation
            printf(" - [%s:%zu]\n\t-> Allocation of %zu bytes at:\t 0x%p\n",
                   allocation->allocated_from_file,
                   allocation->allocated_from_line,
                   allocation->size,
                   allocation->base);
        }
    }

    // Segfaults were prevented, print them
    if (prevented_segfaults_count > 0)
    {
        printf("\n\n[MEMORY WATCHER]: Some segfaults were prevented !\n\n");

        rg_storage_it it = rg_storage_iterator(RG_MEMORY_WATCHER->prevented_segfaults);
        while (rg_storage_next(&it))
        {
            rg_mem_watcher_segfault *segfault = it.value;

            // Print the segfault
            printf(" - [%s:%zu]\n\t-> Segfault was prevented (free was called with NULL parameter)\n",
                   segfault->freed_from_file,
                   segfault->freed_from_line);
        }
    }

    printf("\n");

    return false;
}

// --=== Override memory functions ===--

/**
 * @brief Allocates memory, but does not initialize it.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
void *rg_mem_watcher_malloc(size_t size, const char *file, size_t line)
{
    // Allocate the memory
    void *ptr = malloc(size);

    // If the allocation succeeded, add it to the allocations map
    if (ptr != NULL && RG_MEMORY_WATCHER != NULL && !RG_MEMORY_WATCHER->locked)
    {
        // Allocate memory for the allocation
        rg_mem_watcher_allocation allocation = {
            .allocated_from_file = file,
            .allocated_from_line = line,
            .base                = ptr,
            .size                = size,
        };

        // Lock watcher to avoid recursion
        RG_MEMORY_WATCHER->locked = true;

        // Add the allocation to the map
        rg_struct_map_set(RG_MEMORY_WATCHER->allocations, (rg_hash_map_key_t) ptr, &allocation);

        // Unlock watcher
        RG_MEMORY_WATCHER->locked = false;
    }

    return ptr;
}

/**
 * @brief Allocates memory, and sets the memory to zero.
 * @param size The size of the memory to allocate.
 * @return A pointer to the allocated memory, or NULL if the allocation failed.
 */
void *rg_mem_watcher_calloc(size_t count, size_t size, const char *file, size_t line)
{
    // Allocate the memory
    void *ptr = calloc(count, size);

    // If the allocation succeeded, add it to the allocations map
    if (ptr != NULL && RG_MEMORY_WATCHER != NULL && !RG_MEMORY_WATCHER->locked)
    {
        // Allocate memory for the allocation
        rg_mem_watcher_allocation allocation = {
            .allocated_from_file = file,
            .allocated_from_line = line,
            .base                = ptr,
            .size                = count * size,
        };

        // Lock watcher to avoid recursion
        RG_MEMORY_WATCHER->locked = true;

        // Add the allocation to the map
        rg_struct_map_set(RG_MEMORY_WATCHER->allocations, (rg_hash_map_key_t) ptr, &allocation);

        // Unlock watcher
        RG_MEMORY_WATCHER->locked = false;
    }

    return ptr;
}

/**
 * @brief Reallocates memory.
 * @param ptr The pointer to the memory to reallocate.
 * @param size The new size of the memory.
 * @return A pointer to the reallocated memory, or NULL if the reallocation failed.
 */
void *rg_mem_watcher_realloc(void *ptr, size_t size, const char *file, size_t line)
{
    if (ptr == NULL)
    {
        return rg_mem_watcher_malloc(size, file, line);
    }

    if (size == 0)
    {
        rg_mem_watcher_free(ptr, file, line);
        return NULL;
    }

    // Reallocate the memory
    void *new_ptr = realloc(ptr, size);

    // If the reallocation succeeded, update the allocations map
    if (new_ptr != NULL && RG_MEMORY_WATCHER != NULL && !RG_MEMORY_WATCHER->locked)
    {
        // Lock watcher to avoid recursion
        RG_MEMORY_WATCHER->locked = true;

        // Remove the old allocation
        rg_struct_map_erase(RG_MEMORY_WATCHER->allocations, (rg_hash_map_key_t) ptr);

        // Add the new allocation
        rg_mem_watcher_allocation allocation = {
            .allocated_from_file = file,
            .allocated_from_line = line,
            .base                = new_ptr,
            .size                = size,
        };
        rg_struct_map_set(RG_MEMORY_WATCHER->allocations, (rg_hash_map_key_t) new_ptr, &allocation);

        // Unlock watcher
        RG_MEMORY_WATCHER->locked = false;
    }

    return new_ptr;
}

/**
 * @brief Frees memory.
 * @param ptr The pointer to the memory to free.
 */
void rg_mem_watcher_free(void *ptr, const char *file, size_t line)
{
    // If the pointer is NULL, save segfault
    if (ptr == NULL)
    {
        if (RG_MEMORY_WATCHER != NULL && !RG_MEMORY_WATCHER->locked)
        {
            rg_mem_watcher_segfault segfault = {
                .freed_from_file = file,
                .freed_from_line = line,
            };

            // Lock watcher to avoid recursion
            RG_MEMORY_WATCHER->locked = true;

            // Add the segfault to the map
            rg_storage_push(RG_MEMORY_WATCHER->prevented_segfaults, &segfault);

            // Unlock watcher
            RG_MEMORY_WATCHER->locked = false;
        }

        return;
    }

    if (RG_MEMORY_WATCHER != NULL && !RG_MEMORY_WATCHER->locked)
    {
        // Lock watcher to avoid recursion
        RG_MEMORY_WATCHER->locked = true;

        // Remove the allocation from the map
        rg_struct_map_erase(RG_MEMORY_WATCHER->allocations, (rg_hash_map_key_t) ptr);

        // Unlock watcher
        RG_MEMORY_WATCHER->locked = false;
    }

    // Free the memory
    free(ptr);
}

#endif