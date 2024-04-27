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

# Engine Required Includes
find_package(glfw3 3.4 REQUIRED)
find_package(OpenAL REQUIRED)
include_directories("${JoshEngine_SOURCE_DIR}/includes/stb")
target_link_libraries(JoshEngine
        glfw
        OpenAL::OpenAL
)

# Vulkan
#[[ Remove this to switch to another API->
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

# Metal
#[[ Uncomment this and comment out another graphics API block to switch APIs ]]
add_compile_definitions(GFX_API_MTL)
find_package(glslang REQUIRED)
include_directories("${JoshEngine_SOURCE_DIR}/includes/metal-cpp")
target_link_libraries(JoshEngine
        glslang::glslang
        glslang::SPIRV
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
)
#]]