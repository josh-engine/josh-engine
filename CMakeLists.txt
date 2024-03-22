cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

set(CMAKE_CXX_STANDARD 17)

add_executable(JoshEngine src/main.cpp
        src/engine/gfx/enginegfx.cpp
        src/engine/engine.cpp
        src/engine/sound/engineaudio.cpp)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenAL REQUIRED)
target_link_libraries(JoshEngine glfw OpenGL::GL GLEW::GLEW OpenAL::OpenAL)