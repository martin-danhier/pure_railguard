# Config based on https://github.com/krux02/minimal_cmake_example

cmake_minimum_required(VERSION 3.17)
project(railguard)

##################################################################
###                          SETTINGS                          ###
##################################################################

# Select dependencies
set(WINDOW_SDL2 "true")
add_compile_definitions(WINDOW_SDL2)

set(RENDERER_VULKAN "true")
add_compile_definitions(
    RENDERER_VULKAN
    VK_NO_PROTOTYPE
)

##################################################################
###                        FILE GLOBBING                       ###
##################################################################

# These instructions search the directory tree when cmake is
# invoked and put all files that match the pattern in the variables
# `sources` and `data`.
file(GLOB_RECURSE sources           src/*.c     include/*.h)
file(GLOB_RECURSE sources_test      tests/*.c)
file(GLOB_RECURSE data              resources/*)

##################################################################
###                       DEFINE TARGETS                       ###
##################################################################

# The data is just added to the executable, because in some IDEs (QtCreator)
# files are invisible when they are not explicitly part of the project.
add_executable(railguard ${sources} ${data})

# Add include directory to allow includes with <> pair
target_include_directories(railguard PUBLIC include)


# Copy resource files in the build directory
# That way, they can be resolved relatively to the executable
file(COPY ${data} DESTINATION resources)

# Redirect outputs
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")

##################################################################
###                   COMPILER CONFIGURATION                   ###
##################################################################

# Define C and C++ versions
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)

##################################################################
###                        DEPENDENCIES                        ###
##################################################################

# Find Vulkan if needed
if(DEFINED RENDERER_VULKAN)
    find_package(Vulkan REQUIRED)
endif()

# Load external script
add_subdirectory(external)

# Link dependencies
if (DEFINED RENDERER_VULKAN)
    target_link_libraries(railguard PUBLIC
        Vulkan::Vulkan
        vma
        volk
    )
endif()

if (DEFINED WINDOW_SDL2)
    target_link_libraries(railguard PUBLIC
        sdl2
    )
endif()


##################################################################
###                           TESTING                          ###
##################################################################

# TODO

##################################################################
###                          PACKAGING                         ###
##################################################################

# TODO if needed