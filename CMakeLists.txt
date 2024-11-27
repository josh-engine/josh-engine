cmake_minimum_required(VERSION 3.27)
project(JoshEngine)

option(JE_API_VK "Use Vulkan Graphics API" ON)
option(JE_OPENAL "Compile with audio support" ON)

set(CMAKE_CXX_STANDARD 20)

# Engine Required Includes
find_package(glfw3 3.3 REQUIRED)
find_package(OpenAL REQUIRED)
find_package(glm REQUIRED)

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

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG_ENABLED)
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

if (JE_OPENAL)
    set(JoshEngine_libraries ${JoshEngine_libraries}
            OpenAL::OpenAL
    )
else()
    add_compile_definitions(AUDIO_DISABLE)
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
target_link_libraries(JoshEngine ${JoshEngine_libraries})