cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

set(CMAKE_CXX_STANDARD 17)

add_executable(JoshEngine src/main.cpp
        src/engine/gfx/opengl/gfx_gl33.cpp
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
        src/engine/enginedebug.cpp)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(OpenAL REQUIRED)
target_link_libraries(JoshEngine glfw OpenGL::GL OpenAL::OpenAL)