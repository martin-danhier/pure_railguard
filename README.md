# Railguard Engine

## Setup development environment

### Summary

1. Clone this repository with argument `--recurse-submodules`
1. Install Vulkan SDK
2. Install SDK2 SDK and add an environment variable `SDL2_PATH` pointing to it if it is not at a standard location
3. Install Premake5
4. Install a C++ compiler

### Windows

With [Scoop](https://scoop.sh):
1. Run `scoop install vulkan SDL2 premake`
2. Install [Visual C++ Build Tools 2019](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2019)
3. In the search menu, search "env" and select "Edit the system environment variables"
4. Add a new user variable `SDL2_PATH` pointing to `C:/Users/<your name>/scoop/apps/SDL2/current`
5. Edit the `PATH` user variable and add `C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin` to add MSBuild to your PATH

## Set up an IDE

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
