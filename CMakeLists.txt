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
        src/engine/gfx/vk/gfx_vk.cpp
        src/engine/gfx/renderable.cpp
        src/engine/gfx/imgui/imgui_impl_vulkan.cpp
        src/test.cpp
)

# Engine Required Includes
find_package(glfw3 3.4 REQUIRED)
find_package(OpenAL REQUIRED)
target_link_libraries(JoshEngine
        glfw
        OpenAL::OpenAL
)

# Vulkan
#[[ Remove this to switch to another API-> ]]
find_package(Vulkan REQUIRED)
find_package(glslang REQUIRED)
add_compile_definitions(GFX_API_VK)
target_link_libraries(JoshEngine
        Vulkan::Vulkan
        glslang::glslang
        glslang::SPIRV
)
#]]

# OpenGL
#[[ Uncomment this and comment out another graphics API block to switch APIs
find_package(OpenGL REQUIRED)
add_compile_definitions(GFX_API_OPENGL41)
target_link_libraries(JoshEngine
       OpenGL::GL
)
#]]