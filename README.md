# Railguard Engine

## Development environment

### Common

1. Install Vulkan SDK
2. Install SDK2 SDK and add an environment variable `SDL2_PATH` pointing to it if it is not at a standard location
3. Install Premake5

### Visual Studio Code (official)

1. Install recommended extensions
2. Create a `.ccls` file containing the following:

```
clang
%h -x
%h c-header
-I./include
-I${SDL2_PATH}/include
-I${VULKAN_SDK}/include
```
where `${SDL2_PATH}` and `${VULKAN_SDK}` are replaced with the actual paths for SDL2 and Vulkan (CCLS does not support env variables in the config file).

### CLion

1. Install the Premake CMake action
2. Run `premake5 cmake`
3. Open in CLion
4. In the properties of CMake, replace the build type with ``Win64_Debug`` (or ``Linux_Debug``)