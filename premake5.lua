-- Disable diagnostics for Lua because premake tags are not recognized
---@diagnostic disable: undefined-field
---@diagnostic disable: undefined-global

-- Global settings
workspace "railguard";


-------------------------------------------------------------------
---                        CONFIGURATIONS                       ---
-------------------------------------------------------------------

configurations {"Debug", "Release"}

-- Debug config --
filter ("configurations:Debug")

    defines {"DEBUG", "USE_VK_VALIDATION_LAYERS", "RENDERER_VULKAN", "WINDOW_SDL2"}
    symbols "On"

-- Release config --
filter ("configurations:Release")

    defines {"NDEBUG", "RENDERER_VULKAN", "WINDOW_SDL2"}
    optimize "On"

filter {}

------------------------------------------------------------------
---                          PLATFORMS                         ---
------------------------------------------------------------------

platforms {"Win64", "Linux"}

-- Windows platform --
filter "platforms:Win64"

    system "windows"

-- Linux platform --
filter "platforms:Linux"

    system "linux"

filter {}

-------------------------------------------------------------------
---                       ACTIONS SETTINGS                      ---
-------------------------------------------------------------------

-- CMake (requires action from GitHub)

filter "action:cmake"

   location "cmakebuild" -- Put CMake in its own directory

-- Not CMake (GMake, Visual Studio...)
filter "action:not cmake"

   location "build"

filter {}


-------------------------------------------------------------------
---                     EXTERNAL DEPENDENCIES                   ---
-------------------------------------------------------------------

-- Some libs need to be found outside the project and be installed.

-- === SDL2 ===--

SDL_INCLUDE_DIR = os.findheader("SDL.h", {
    "$(SDL2_PATH)/include/SDL2",
    "/usr/include",
    "/usr/include/SDL2",
    "$(sdl2_image_DIR)/include"
})
SDL_LIB_DIR = os.findlib("SDL2", {
    "$(SDL2_PATH)/lib",
    "/usr/lib/x86_64-linux-gnu/",
    "$(sdl2_image_DIR)/lib",
    "/usr/lib64",
    "/usr/lib64/cmake/SDL2"
})

-- Check if is was found
if SDL_INCLUDE_DIR == nil or SDL_LIB_DIR == nil then

    -- If not, get the SDL2_PATH env variable
    local sdl_path = os.getenv('SDL2_PATH')
    if sdl_path == nil then
        error("If SDL2 is not located at a conventional place, the root directory needs to be stored in the env variable SDL2_PATH.")
    end

    -- Get header from sdl path if missing
    if SDL_INCLUDE_DIR == nil then
        SDL_INCLUDE_DIR = sdl_path .. '/include'
    end

    -- Get lib from sdl path if missing
    if SDL_LIB_DIR == nil then
        SDL_LIB_DIR = sdl_path .. '/lib'
    end
end

print "-> Found SDL2"

-- === Vulkan ===--

-- For Vulkan, we will look the env variable first
local vulkan_path = os.getenv('VULKAN_SDK')
if vulkan_path == nil then
    -- If nil, search a conventional location
    VULKAN_INCLUDE_DIR = os.findheader("vulkan/vulkan.h", {
        '/usr/include'
    })
    VULKAN_LIB_DIR = os.findlib("vulkan", {
        '/usr/lib/x86_64-linux-gnu'
    })
    -- And assume that glslangValidator is in PATH
    VULKAN_GLSLANG_VALIDATOR = 'glslangValidator'

    if VULKAN_INCLUDE_DIR == nil or VULKAN_LIB_DIR == nil then
        error("Could not find Vulkan SDK. Either set the VULKAN_SDK env variable or place the SDK at a conventional location.")
    end
else
    -- If not nil, simply use the path
    VULKAN_INCLUDE_DIR = vulkan_path .. "/include"
    VULKAN_LIB_DIR = vulkan_path .. "/lib"
    VULKAN_GLSLANG_VALIDATOR = vulkan_path .. "/bin/glslangValidator"
end

print "-> Found Vulkan"

------------------------------------------------------------------
---                        BUILD PROJECTS                      ---
------------------------------------------------------------------

--==== Shaders compilation ====--

project "shaders"

   kind "Utility"
   os.mkdir "bin/shaders" -- Create output directory

    -- Place cmake and others builds in different directories so clion and vscode dont mix
   filter "action:cmake"
      location "cmakebuild/shaders"
   filter "action:not cmake"
      location "build/shaders"
   filter {}

   -- Take all shader files
   files { "shaders/**.vert", "shaders/**.frag", "shaders/**.glsl", "shaders/**.comp" }

   -- Build shaders with glslangValidator
   filter {"files:**"}
        buildcommands { '"' .. VULKAN_GLSLANG_VALIDATOR .. '"' ..  ' -V "%{file.relpath}" -o "../../bin/shaders/%{file.name}.spv"' }
        buildoutputs {"bin/shaders/%{file.name}.spv"}
   filter {}

--==== Volk Vulkan Loader ====--

project "volk"
   kind "StaticLib"
   language "C"
   architecture "x64"
   files {
      "external/volk/volk.c",
      "external/volk/volk.h"
   }

   includedirs {
       VULKAN_INCLUDE_DIR
   }
   libdirs {
       VULKAN_LIB_DIR
   }

   -- Platform specific settings
   filter "platforms:Linux"
      links "dl"
   filter{}


--==== VkBootstrap ====--



-- project "vkbootstrap"
--    kind "StaticLib"
--    language "C++"
--    architecture "x64"
--
--    files {
--       "./external/vk-bootstrap/src/**.cpp",
--       "./external/vk-bootstrap/src/**.h"
--    }
--
--    includedirs {
--       VULKAN_INCLUDE_DIR
--    }
--    libdirs {
--       VULKAN_LIB_DIR
--    }
--
--    -- Platform specific settings
--    filter "platforms:Linux"
--        links "dl"
--    filter{}

--==== VulkanMemoryAllocator ====--

project "vma"

   kind "StaticLib"
   language "C++"
   architecture "x64"
   targetdir "bin/%{cfg.buildcfg}"
   files {
       "external/vma_implementation.cpp",
   }
   includedirs {
       VULKAN_INCLUDE_DIR,
       "external/vma/include"
   }
   filter "platforms:Linux"
       links "dl"
   filter{}

--==== Railguard lib ====--

project "railguard"
   -- Global project settings
   kind "StaticLib"
   language "C++"
   cdialect "C11"
   dependson {"shaders", "volk", "vma"}
   architecture "x64"
   targetdir "bin/%{cfg.buildcfg}"
   defines {
       "VK_NO_PROTOTYPE"
   }

   -- Add source files
   files "src/**.c"
   removefiles "src/main.c"

   -- Add header dependencies
   includedirs {
       VULKAN_INCLUDE_DIR,
       SDL_INCLUDE_DIR,
       "external/volk",
       "external/vma/include",
       "include"
   }

   -- Add lib dependencies
   libdirs {
      VULKAN_LIB_DIR,
      SDL_LIB_DIR
   }

   -- Add links
   links {"SDL2", "volk", "vma"}

   -- Platform specific settings
   filter "platforms:Win64"
       links "SDL2main"
   filter "platforms:Linux"
       links "dl"
   filter{}

--==== Main  ====--

-- Place the lines in common between each test/main target in a function
function common_requirements()
    kind "ConsoleApp"
    dependson {"railguard", "shaders", "volk", "vma"}
    language "C++"
    cdialect "C11"
    targetdir "bin/%{cfg.buildcfg}"

    -- Add lib dependencies
    libdirs {
       VULKAN_LIB_DIR,
       SDL_LIB_DIR
    }

    includedirs "include"

    links {"railguard", "SDL2", "volk", "vma"}

    -- Platform specific settings
    filter "platforms:Win64"
        links "SDL2main"
    filter "platforms:Linux"
        links "dl"
    filter{}
end

-- Main target
project "main"
    common_requirements()
    files "src/main.c"

-- Hash map test
project "test_hash_map"
    common_requirements()
    files "tests/utils/test_hash_map.c"