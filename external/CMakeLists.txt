##################################################################
###                            VULKAN                          ###
##################################################################

# Imported in parent directory

##################################################################
###                             SDL2                           ###
##################################################################

if(DEFINED WINDOW_SDL2)
    if (DEFINED CACHE{sdl_include_dir} AND DEFINED CACHE{sdl_lib_dir})
        set(sdl_already_in_cache "true")
    endif()

    # Find the header directory
    find_path(sdl_include_dir
        "SDL2/SDL.h"
        PATHS   "$ENV{SDL2_PATH}/include"
                "/usr/include"
                "$ENV{sdl2_image_DIR}/include"
    )

    # Find the lib directory
    find_path(sdl_lib_dir
        NAMES   "libSDL2.so" "SDL2.lib" "SDL2.dll"
        PATHS   "$ENV{SDL2_PATH}/lib"
                "/usr/lib/x86_64-linux-gnu/"
                "$ENV{sdl2_image_DIR}/lib"
                "/usr/lib64"
                "/usr/lib64/cmake/SDL2"
        PATH_SUFFIXES ".lib" ".so" ".dll"
    )

    # Not found
    if((sdl_include_dir STREQUAL "sdl_include_dir-NOTFOUND") OR (sdl_lib_dir STREQUAL "sdl_lib_dir-NOTFOUND"))
        message(FATAL_ERROR "If SDL2 is not located at a conventional place, the root directory needs to be stored in the env variable SDL2_PATH.")
    endif()

    # Found

    if (NOT sdl_already_in_cache STREQUAL "true")
        message("-- Found SDL2")
        message("       Include dir: ${sdl_include_dir}")
        message("       Lib dir:     ${sdl_lib_dir}")
    endif()

    # Setup and link SDL
    add_library(sdl2 INTERFACE)
    target_include_directories(sdl2 INTERFACE ${sdl_include_dir})
    target_link_directories(sdl2 INTERFACE ${sdl_lib_dir})

    target_link_libraries(sdl2 INTERFACE SDL2)
    if (WIN32)
        target_link_libraries(sdl2 INTERFACE SDL2main)
    endif()

endif()

##################################################################
###                       VULKAN-MEM-ALLOC                     ###
##################################################################

if (DEFINED RENDERER_VULKAN)
    add_library(vma STATIC)
    target_sources(vma PRIVATE
        vma_implementation.cpp
    )
    target_include_directories(vma PUBLIC vma/include)
    target_link_libraries(vma PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
endif()

##################################################################
###                             VOLK                           ###
##################################################################

if (DEFINED RENDERER_VULKAN)
    add_library(volk STATIC)
    target_sources(volk PRIVATE
        volk/volk.c
        volk/volk.h
    )
    target_include_directories(volk PUBLIC volk)
    target_link_libraries(volk PUBLIC Vulkan::Vulkan $<$<BOOL:UNIX>:${CMAKE_DL_LIBS}>)
endif()
