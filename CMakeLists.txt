cmake_minimum_required(VERSION 3.27)
project(JoshEngine3_1)

set(CMAKE_CXX_STANDARD 17)

add_executable(JoshEngine3_1 src/main.cpp
        src/engine/gfx/enginegfx.cpp
        src/engine/engine.cpp)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
target_link_libraries(JoshEngine3_1 glfw OpenGL::GL GLEW::GLEW)