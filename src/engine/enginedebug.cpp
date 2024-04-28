//
// Created by Ember Lee on 3/24/24.
//
#include "engineconfig.h"
#include <string>
#ifdef DEBUG_ENABLED
#include "enginedebug.h"
#include "engine.h"
#include "gfx/imgui/imgui.h"
#include <unordered_map>
#include <string>
#ifdef GFX_API_VK
#include "gfx/vk/gfx_vk.h"
bool vulkanMemoryView;
#endif

bool statView, gmObjView, texturesView;
std::string selectedGameObject;
std::string selectedTexture;
std::unordered_map<void*, std::string> functionNameMap;
#endif //DEBUG_ENABLED


void initDebugTools() {
#ifdef DEBUG_ENABLED
    putImGuiCall(&setupImGuiWindow);
#endif //DEBUG_ENABLED
}

void registerFunctionToDebug(const std::string& name, void* function) {
#ifdef DEBUG_ENABLED
    functionNameMap.insert({function, name});
#endif //DEBUG_ENABLED
}

void setupImGuiWindow() {
#ifdef DEBUG_ENABLED
    ImGui::Begin("Debug Menu");

    ImGui::Text("JoshEngine %s", ENGINE_VERSION_STRING);
#ifdef GFX_API_OPENGL41
    ImGui::Text("Running on OpenGL 4.1");
#elif defined(GFX_API_VK)
    ImGui::Text("Running on Vulkan");
#elif defined(GFX_API_MTL)
    ImGui::Text("Running on Metal");
#endif

    ImGui::Checkbox("Stats View", &statView);
    ImGui::Checkbox("GameObjects View", &gmObjView);
    ImGui::Checkbox("Textures View", &texturesView);
#ifdef GFX_API_VK
    ImGui::Checkbox("vkAlloc View", &vulkanMemoryView);
#endif

    ImGui::End();

    if (statView) {
        ImGui::Begin("Stats");

        ImGui::Text("Frame time: %ims (~%i fps)", static_cast<int>(getFrameTime()), static_cast<int>(1/(getFrameTime()/1000)));
        ImGui::Text("Renderables: %i", getRenderableCount());

        ImGui::End();
    }

    if (gmObjView) {
        ImGui::Begin("GameObjects");

        std::unordered_map<std::string, GameObject> gameObjects = getGameObjects();

        if (ImGui::BeginCombo("GameObject", selectedGameObject.c_str(), 0)) {
            for (const auto& object: gameObjects) {
                if (ImGui::Selectable(object.first.c_str(), selectedGameObject == object.first))
                    selectedGameObject = object.first;
            }
            ImGui::EndCombo();
        }

        if (!selectedGameObject.empty()) {
            GameObject gmObj = gameObjects.at(selectedGameObject);

            ImGui::Text("Transform");
            ImGui::Indent();
            ImGui::TextColored({0.5f, 1.0f, 0.5f, 1.0f}, "Position");
            ImGui::TextColored({0.5f, 1.0f, 0.5f, 1.0f}, "X: %f, Y: %f, Z: %f", gmObj.transform.position.x, gmObj.transform.position.y, gmObj.transform.position.z);
            ImGui::TextColored({1.0f, 0.5f, 0.5f, 1.0f}, "Rotation");
            ImGui::TextColored({1.0f, 0.5f, 0.5f, 1.0f}, "X: %f, Y: %f, Z: %f", gmObj.transform.rotation.x, gmObj.transform.rotation.y, gmObj.transform.rotation.z);
            ImGui::TextColored({0.5f, 0.5f, 1.0f, 1.0f}, "Scale");
            ImGui::TextColored({0.5f, 0.5f, 1.0f, 1.0f}, "X: %f, Y: %f, Z: %f", gmObj.transform.scale.x, gmObj.transform.scale.y, gmObj.transform.scale.z);
            ImGui::Unindent();

            ImGui::Text("Functions");
            ImGui::Indent();
            if (gmObj.onUpdate.empty())
                ImGui::Text("Empty");
            else {
                for (auto function: gmObj.onUpdate) {
                    if (functionNameMap.find(reinterpret_cast<void*>(function)) == functionNameMap.end()) {
                        ImGui::TextColored({0.75f, 0.75f, 0.75f, 1.0f}, "Function at %lx", (unsigned long) function);
                    } else {
                        ImGui::Text("%s", functionNameMap.at(reinterpret_cast<void*>(function)).c_str());
                    }
                }
            }
            ImGui::Unindent();

            ImGui::Text("Renderable count: %zu", gmObj.renderables.size());
        }

        ImGui::End();
    }


    if (texturesView) {
        ImGui::Begin("Textures");
#if defined(GFX_API_OPENGL41)
        std::unordered_map<std::string, unsigned int> textures = getTexs();

        if (ImGui::BeginCombo("Texture", selectedTexture.c_str(), 0)) {
            for (const auto& object: textures) {
                if (ImGui::Selectable(object.first.c_str(), selectedTexture == object.first))
                    selectedTexture = object.first;
            }
            ImGui::EndCombo();
        }
        if (!selectedTexture.empty()) {
#ifdef GFX_API_OPENGL41
            ImGui::Image((void*)(intptr_t)textures.at(selectedTexture), {ImGui::GetWindowSize().x-20, ImGui::GetWindowSize().y-60});
#endif
        }

#else
        ImGui::Text("Texture view is only supported in OpenGL.");
#endif
        ImGui::End();
    }
#ifdef GFX_API_VK
    if (vulkanMemoryView) {
        ImGui::Begin("vkAlloc Memory Viewer");
        std::vector<JEMemoryBlock_VK> mem = getMemory();
        ImGui::Text("Minimum alloc: %i MiB", (NEW_BLOCK_MIN_SIZE/1024)/1024);
        int i = 0;
        for (auto const& block : mem) {
            ImGui::Text("Block %i - Memory Type %i", i, block.type);
            std::string count = std::to_string((block.top/1024)/1024) + " MiB/" + std::to_string(((block.size)/1024)/1024) + " MiB";
            ImGui::ProgressBar((static_cast<float>(block.top))/static_cast<float>(block.size), ImVec2(-FLT_MIN, 0), count.c_str());
            i++;
        }
        ImGui::End();
    }
#endif //GFX_API_VK
#endif //DEBUG_ENABLED
}