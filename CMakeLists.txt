cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

set(CMAKE_CXX_STANDARD 20)

# Engine Required Includes
find_package(glfw3 3.3 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(glm REQUIRED)
include_directories("${JoshEngine_SOURCE_DIR}/includes/stb")
set(JoshEngine_libraries
        glm::glm
        glfw
        OpenAL::OpenAL
)
set(JoshEngine_sources src/main.cpp
        src/test.cpp
        src/engine/engine.cpp
        src/engine/sound/engineaudio.cpp
        src/engine/gfx/modelutil.cpp
        src/engine/gfx/imgui/imgui.cpp
        src/engine/gfx/imgui/imgui_demo.cpp
        src/engine/gfx/imgui/imgui_draw.cpp
        src/engine/gfx/imgui/imgui_tables.cpp
        src/engine/gfx/imgui/imgui_widgets.cpp
        src/engine/gfx/imgui/imgui_impl_glfw.cpp
        src/engine/enginedebug.cpp
        src/engine/gfx/renderable.cpp
)

option(JE_API_VK "Use Vulkan Graphics API" ON)
option(JE_API_GL "Use OpenGL Graphics API" OFF)
if (APPLE)
    option(JE_API_MTL "Use Metal Graphics API (Mac only)" OFF)
    add_compile_definitions(PLATFORM_MAC)
elseif (UNIX)
    add_compile_definitions(PLATFORM_UNIX)
endif()
# Vulkan
if (JE_API_VK)
    find_package(Vulkan REQUIRED)
    find_package(glslang REQUIRED)
    add_compile_definitions(GFX_API_VK)
    set(JoshEngine_libraries ${JoshEngine_libraries}
            Vulkan::Vulkan
            glslang::glslang
            glslang::SPIRV
    )
    set(JoshEngine_sources ${JoshEngine_sources}
            src/engine/gfx/vk/gfx_vk.cpp
            src/engine/gfx/imgui/imgui_impl_vulkan.cpp
    )
endif()

# OpenGL
if (JE_API_GL)
    find_package(OpenGL REQUIRED)
    find_package(glslang REQUIRED)
    add_compile_definitions(GFX_API_OPENGL41)
    set(JoshEngine_libraries ${JoshEngine_libraries}
            OpenGL::GL
            glslang::glslang
            glslang::SPIRV
    )
    set(JoshEngine_sources ${JoshEngine_sources}
            src/engine/gfx/opengl/gfx_gl41.cpp
            src/engine/gfx/imgui/imgui_impl_opengl3.cpp
            src/engine/gfx/spirv/cross/spirv_glsl.cpp
            src/engine/gfx/spirv/cross/spirv_parser.cpp
            src/engine/gfx/spirv/cross/spirv_reflect.cpp
            src/engine/gfx/spirv/cross/spirv_cross_util.cpp
            src/engine/gfx/spirv/cross/spirv_cross_parsed_ir.cpp
            src/engine/gfx/spirv/cross/spirv_cross_c.cpp
            src/engine/gfx/spirv/cross/spirv_cross.cpp
            src/engine/gfx/spirv/cross/spirv_cpp.cpp
            src/engine/gfx/spirv/cross/spirv_cfg.cpp
    )
endif()

# Metal
if (JE_API_MTL)
    find_package(glslang REQUIRED)
    include_directories("${JoshEngine_SOURCE_DIR}/includes/metal-cpp")
    add_compile_definitions(GFX_API_MTL)
    set(JoshEngine_libraries ${JoshEngine_libraries}
            glslang::glslang
            glslang::SPIRV
            "-framework Metal"
            "-framework Foundation"
            "-framework QuartzCore"
    )
    set(JoshEngine_sources ${JoshEngine_sources}
            src/engine/gfx/mtl/gfx_mtl.mm
            src/engine/gfx/mtl/mtl_impl.cpp
            src/engine/gfx/imgui/imgui_impl_metal.mm
            src/engine/gfx/spirv/cross/spirv_msl.cpp
            src/engine/gfx/spirv/cross/spirv_glsl.cpp
            src/engine/gfx/spirv/cross/spirv_hlsl.cpp
            src/engine/gfx/spirv/cross/spirv_parser.cpp
            src/engine/gfx/spirv/cross/spirv_reflect.cpp
            src/engine/gfx/spirv/cross/spirv_cross_util.cpp
            src/engine/gfx/spirv/cross/spirv_cross_parsed_ir.cpp
            src/engine/gfx/spirv/cross/spirv_cross_c.cpp
            src/engine/gfx/spirv/cross/spirv_cross.cpp
            src/engine/gfx/spirv/cross/spirv_cpp.cpp
            src/engine/gfx/spirv/cross/spirv_cfg.cpp
    )
endif()

add_executable(JoshEngine ${JoshEngine_sources})
target_link_libraries(JoshEngine ${JoshEngine_libraries})