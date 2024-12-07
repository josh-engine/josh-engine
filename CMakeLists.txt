cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

option(JE_API_VK "Use Vulkan Graphics API" ON)
option(JE_API_VK "Use WebGPU Graphics API" OFF)
option(JE_OPENAL "Compile with audio support" ON)

set(CMAKE_CXX_STANDARD 20)

# Require for GLM
include(FetchContent)

# Engine Required Includes
if (NOT EMSCRIPTEN)
find_package(glfw3 3.3 REQUIRED)
find_package(OpenAL REQUIRED)
endif()

FetchContent_Declare(
        glm
        GIT_REPOSITORY	https://github.com/g-truc/glm.git
        GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)

# Global definitions should probably be defined before compilation
add_compile_definitions(GLM_FORCE_DEFAULT_ALIGNED_GENTYPES)
add_compile_definitions(GLM_ENABLE_EXPERIMENTAL)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

include_directories("${JoshEngine_SOURCE_DIR}/includes/stb")
set(JoshEngine_libraries
        glm::glm
        glfw
)
set(JoshEngine_sources
        src/engine/gfx/renderable.cpp
        src/engine/sound/audioutil.cpp
        src/engine/gfx/modelutil.cpp
        src/engine/ui/imgui/imgui.cpp
        src/engine/ui/imgui/imgui_demo.cpp
        src/engine/ui/imgui/imgui_draw.cpp
        src/engine/ui/imgui/imgui_tables.cpp
        src/engine/ui/imgui/imgui_widgets.cpp
        src/engine/ui/imgui/imgui_impl_glfw.cpp
        src/engine/ui/je/uiutil.cpp
        src/engine/debug/debugutil.cpp
        src/engine/jbd/bundleutil.cpp
        src/engine/engine.cpp
        src/main.cpp
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT EMSCRIPTEN)
    add_compile_definitions(DEBUG_ENABLED)
    if (XCODE)
        set_target_properties(JoshEngine PROPERTIES
                XCODE_GENERATE_SCHEME ON
                XCODE_SCHEME_ENABLE_GPU_FRAME_CAPTURE_MODE "Metal"
        )
    endif()
endif()

if (APPLE)
    add_compile_definitions(PLATFORM_MAC)
elseif (UNIX)
    add_compile_definitions(PLATFORM_UNIX)
endif()

# Vulkan
if (JE_API_VK)
    find_package(Vulkan REQUIRED)
    find_package(glslang REQUIRED)
    add_compile_definitions(GFX_API_VK)
    add_compile_definitions(GLFW_INCLUDE_VULKAN)
    set(JoshEngine_libraries ${JoshEngine_libraries}
            Vulkan::Vulkan
            glslang::glslang
            glslang::SPIRV
    )
    set(JoshEngine_sources ${JoshEngine_sources}
            src/engine/gfx/vk/gfx_vk.cpp
            src/engine/ui/imgui/imgui_impl_vulkan.cpp
    )
endif()

# WebGPU
if (JE_API_WEBGPU)
    if (NOT EMSCRIPTEN)
        if (WEBGPU_BACKEND STREQUAL "WGPU")
            message("JoshEngine detected WGPU for WebGPU backend.")
            add_compile_definitions(IMGUI_IMPL_WEBGPU_BACKEND_WGPU)
        elseif (WEBGPU_BACKEND STREQUAL "DAWN")
            message(SEND_ERROR "JoshEngine detected Dawn for WebGPU backend. If you're determined enough, you can make it work, but this was written with WGPU in mind.")
            add_compile_definitions(IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
        else()
            message(SEND_ERROR "JoshEngine detected no WebGPU backend! Please set -DWEBGPU_BACKEND=WGPU in your cmake invocation arguments.")
        endif()
    endif()
    add_subdirectory(webgpu)
    add_subdirectory(glfw3webgpu)
    add_compile_definitions(GFX_API_WEBGPU)
    add_compile_definitions(GLFW_INCLUDE_NONE)
    set(JoshEngine_sources ${JoshEngine_sources}
            src/engine/ui/imgui/imgui_impl_wgpu.cpp
            src/engine/gfx/wgpu/gfx_wgpu.cpp
    )

    set(JoshEngine_libraries ${JoshEngine_libraries}
            webgpu
            glfw3webgpu
    )
endif()

if (NOT EMSCRIPTEN)
    if (JE_OPENAL)
        set(JoshEngine_libraries ${JoshEngine_libraries}
                OpenAL::OpenAL
        )
    else()
        add_compile_definitions(AUDIO_DISABLE)
    endif()
endif()

# Please put your game's sources under Game_src to stay organized!
# You don't have to though, I'm not the police.

set(Game_src
        src/test.cpp
        src/gamephysicslib.cpp)
set(JoshEngine_sources ${JoshEngine_sources}
                       ${Game_src}
)

add_executable(JoshEngine ${JoshEngine_sources})
target_link_libraries(JoshEngine PRIVATE ${JoshEngine_libraries})

if (JE_API_WEBGPU)
    target_copy_webgpu_binaries(JoshEngine)
endif()
if (EMSCRIPTEN)
    set(SHELL_FILE je_shell.html)

    set_target_properties(JoshEngine PROPERTIES SUFFIX ".html")
    target_link_options(JoshEngine PRIVATE
            -sASYNCIFY
            -sUSE_GLFW=3
            -sUSE_WEBGPU
            --preload-file "${JoshEngine_SOURCE_DIR}/engineRuntime@/"
            -sALLOW_MEMORY_GROWTH
            -sNO_DISABLE_EXCEPTION_CATCHING
            -lopenal
            --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    )
    set_property(
            TARGET JoshEngine
            PROPERTY LINK_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${SHELL_FILE}"
    )
endif()