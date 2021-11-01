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
        "src/utils/io.c"
        "src/utils/maps.c"
        "src/utils/event_sender.c"
        )

set(test_resources
        "tests/resources/test.txt"
        )

# The data is just added to the executable, because in some IDEs (QtCreator)
# files are invisible when they are not explicitly part of the project.
add_executable(railguard)
target_sources(railguard PRIVATE src/main.c)

# Set main project as a lib that will be used in the main executable and in tests
add_library(railguard_lib STATIC)
target_sources(railguard_lib PRIVATE ${sources})

# Add include directory to allow includes with <> pair
target_include_directories(railguard_lib PUBLIC include)

# Link with executable
target_link_libraries(railguard railguard_lib)

# Copy resource files in the build directory
# That way, they can be resolved relatively to the executable
#file(COPY ${data} DESTINATION resources)


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
    target_link_libraries(railguard_lib PUBLIC
            Vulkan::Vulkan
            vma
            volk
            )
endif ()

if (DEFINED WINDOW_SDL2)
    target_link_libraries(railguard_lib PUBLIC
            sdl2
            )
endif ()


##################################################################
###                           TESTING                          ###
##################################################################


add_executable(unit_tests ${test_resources})
target_sources(unit_tests PRIVATE
        tests/main.c
        tests/framework/test_framework.c
        ${sources}
        ${test_sources}
        )

# Link original executable as lib to inherit dependencies
target_link_libraries(unit_tests PUBLIC railguard_lib)

# Definition to recognize tests from inside the code
target_compile_definitions(unit_tests PUBLIC UNIT_TESTS)

# Copy resource files in the build directory
file(COPY ${test_resources} DESTINATION bin/resources)


##################################################################
###                          PACKAGING                         ###
##################################################################

# TODO if needed


##################################################################
###                     SHADER COMPILATION                     ###
##################################################################

# Link dependencies
if (DEFINED RENDERER_VULKAN)

    find_program(glsl_validator glslangValidator HINTS
            /usr/bin
            /usr/local/bin
            $ENV{VULKAN_SDK}/Bin/
            $ENV{VULKAN_SDK}/Bin32/
            ${PROJECT_SOURCE_DIR}/glslangValidator/bin/ # For GitHub Actions
            )
    # find all the shader files under the shaders folder
    file(GLOB_RECURSE glsl_source_files
            "${PROJECT_SOURCE_DIR}/resources/shaders/*.frag"
            "${PROJECT_SOURCE_DIR}/resources/shaders/*.vert"
            "${PROJECT_SOURCE_DIR}/resources/shaders/*.comp"
            )

    # Create directory "bin/resources/shaders"
    file(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/resources/shaders")

    # iterate each shader
    foreach(glsl ${glsl_source_files})
        message(STATUS "Building shader")
        get_filename_component(file_name ${glsl} NAME)
        set(spirv "${PROJECT_SOURCE_DIR}/bin/resources/shaders/${file_name}.spv")
        message(STATUS ${glsl})

        # execute glslang command to compile that specific shader
        add_custom_command(
                OUTPUT ${spirv}
                COMMAND ${glsl_validator} -V ${glsl} -o ${spirv}
                DEPENDS ${glsl})
        list(APPEND spirv_binary_files ${spirv})
    endforeach(glsl)

    add_custom_target(shaders DEPENDS ${spirv_binary_files})

    add_dependencies(railguard_lib shaders)
endif()