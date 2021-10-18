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

# Debug vs Release
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(
            DEBUG
            USE_VK_VALIDATION_LAYERS
    )
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(
            NDEBUG
    )
endif ()

##################################################################
###                        FILE SELECTION                      ###
##################################################################

set(sources
        "src/core/renderer/renderer_vulkan.c"
        "src/core/window/window_sdl2.c"
        "src/core/engine.c"
        "src/utils/arrays.c"
        "src/utils/maps.c"
        "src/utils/event_sender.c"
        )

set(test_sources
        "tests/utils/test_hash_map.c"
        )

##################################################################
###                       DEFINE TARGETS                       ###
##################################################################

# The data is just added to the executable, because in some IDEs (QtCreator)
# files are invisible when they are not explicitly part of the project.
add_executable(railguard)

target_sources(railguard PRIVATE
        src/main.c
        ${sources}
        )

# Add include directory to allow includes with <> pair
target_include_directories(railguard PUBLIC include)


# Copy resource files in the build directory
# That way, they can be resolved relatively to the executable
#file(COPY ${data} DESTINATION resources)

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
if (DEFINED RENDERER_VULKAN)
    find_package(Vulkan REQUIRED)
endif ()

# Load external script
add_subdirectory(external)

# Link dependencies
if (DEFINED RENDERER_VULKAN)
    target_link_libraries(railguard PUBLIC
            Vulkan::Vulkan
            vma
            volk
            )
endif ()

if (DEFINED WINDOW_SDL2)
    target_link_libraries(railguard PUBLIC
            sdl2
            )
endif ()


##################################################################
###                           TESTING                          ###
##################################################################


add_executable(unit_tests)
target_sources(unit_tests PRIVATE
        tests/main.c
        ${sources}
        ${test_sources}
        )

# Link original executable as lib to inherit dependencies
set_target_properties(railguard PROPERTIES ENABLE_EXPORTS on)
target_link_libraries(unit_tests PUBLIC railguard)

# Definition to recognize tests from inside the code
target_compile_definitions(unit_tests PUBLIC UNIT_TESTS)


##################################################################
###                          PACKAGING                         ###
##################################################################

# TODO if needed