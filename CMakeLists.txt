cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

set(CMAKE_CXX_STANDARD 20)

add_executable(JoshEngine src/main.cpp
        src/engine/gfx/opengl/gfx_gl41.cpp
        src/engine/engine.cpp
        src/engine/sound/engineaudio.cpp
        src/engine/gfx/modelutil.cpp
        src/engine/gfx/imgui/imgui.cpp
        src/engine/gfx/imgui/imgui_demo.cpp
        src/engine/gfx/imgui/imgui_draw.cpp
        src/engine/gfx/imgui/imgui_impl_glfw.cpp
        src/engine/gfx/imgui/imgui_impl_opengl3.cpp
        src/engine/gfx/imgui/imgui_tables.cpp
        src/engine/gfx/imgui/imgui_widgets.cpp
        src/engine/enginedebug.cpp
        src/engine/gfx/vk/gfx_vk.cpp)

# Vulkan Required Includes
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)

# OpenGL Required Includes
find_package(OpenGL REQUIRED)

# Engine Required Includes
find_package(glfw3 3.3 REQUIRED)
find_package(OpenAL REQUIRED)

target_link_libraries(JoshEngine glfw OpenGL::GL OpenAL::OpenAL Vulkan::Vulkan glslang::glslang glslang::SPIRV)