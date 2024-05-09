//
// Created by Ember Lee on 3/24/24.
//
#include "engineconfig.h"
#ifdef DEBUG_ENABLED
#include <string>
#include <bit>
#include "enginedebug.h"
#include "engine.h"
#include "gfx/imgui/imgui.h"
#include <unordered_map>

#ifdef GFX_API_VK
#include "gfx/vk/gfx_vk.h"
bool vulkanMemoryView;
#endif

bool statView, gmObjView, texturesView, buffersView;
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

inline constexpr std::string sizeFormat(size_t s) {
    if (s > 1024*1024) {
        return std::to_string((s/1024)/1024) + " MiB";
    } else if (s > 1024) {
        return std::to_string((s/1024)) + " KiB";
    } else {
        return std::to_string(s) + " bytes";
    }
}

void setupImGuiWindow() {
#ifdef DEBUG_ENABLED
    ImGui::Begin("Debug Menu");

    ImGui::Text("JoshEngine %s", ENGINE_VERSION_STRING);
//#if defined(GFX_API_VK)
    // No more other graphics APIs... sadge. If/when DX12 will add this feature back.
    // ImGui::Text("Running on Vulkan");
//#endif

    ImGui::Checkbox("Stats View", &statView);
    ImGui::Checkbox("GameObjects View", &gmObjView);
    ImGui::Checkbox("Textures View", &texturesView);
    ImGui::Checkbox("Uniforms View", &buffersView);
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
            for (const auto &object: gameObjects) {
                if (ImGui::Selectable(object.first.c_str(), selectedGameObject == object.first))
                    selectedGameObject = object.first;
            }
            ImGui::EndCombo();
        }

        if (!selectedGameObject.empty()) {
            GameObject gmObj = gameObjects.at(selectedGameObject);

            ImGui::Text("Transform");
            ImGui::Indent();
            ImGui::InputFloat3("Position", &gmObj.transform.position[0]);
            ImGui::InputFloat3("Rotation", &gmObj.transform.rotation[0]);
            ImGui::InputFloat3("Scale",    &gmObj.transform.scale[0]   );
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
        std::unordered_map<std::string, unsigned int> textures = getTexs();

        if (ImGui::BeginCombo("Texture", selectedTexture.c_str(), 0)) {
            for (const auto& object: textures) {
                if (ImGui::Selectable(object.first.c_str(), selectedTexture == object.first))
                    selectedTexture = object.first;
            }
            ImGui::EndCombo();
        }
        if (!selectedTexture.empty()) {
            ImTextureID texture_id = getTex(textures.at(selectedTexture));
            ImGui::Image(texture_id, {ImGui::GetWindowSize().x-20, ImGui::GetWindowSize().y-60});
        }
        ImGui::End();
    }

    if (buffersView) {
        ImGui::Begin("Uniform Buffers");
        for (size_t i = 0; i < getBufCount(); i++) {
            ImGui::Text("Buffer %zu", i);
            ImGui::Indent();
            JEBufferReference_VK ref = getBuf(i);
            for (int i1 = 0; i1 < MAX_FRAMES_IN_FLIGHT; i1++) {
                JEAllocation_VK ac = (*ref.alloc)[i1];
                ImGui::Text("Frame %i", i1);
                ImGui::Indent();
                ImGui::Text("Allocation is %s", sizeFormat(ac.size).c_str());
                ImGui::Text("Stored in block of size %s and type %i", sizeFormat(ac.memoryRef->size).c_str(), ac.memoryRef->type);
                ImGui::Text("Map pointer is 0x%lx", std::bit_cast<unsigned long>((*ref.map)[i1]));
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        ImGui::End();
    }

#ifdef GFX_API_VK
    if (vulkanMemoryView) {
        ImGui::Begin("vkAlloc Memory Viewer");
        std::vector<JEMemoryBlock_VK> mem = getMemory();
        ImGui::Text("Minimum alloc: %s", sizeFormat(NEW_BLOCK_MIN_SIZE).c_str());
        int i = 0;
        for (auto const& block : mem) {
            ImGui::Text("Block %i - Memory Type %i", i, block.type);
            std::string count = sizeFormat(block.top) +"/" + sizeFormat(block.size);
            ImGui::ProgressBar((static_cast<float>(block.top))/static_cast<float>(block.size), ImVec2(-FLT_MIN, 0), count.c_str());
            i++;
        }
        ImGui::End();
    }
#endif //GFX_API_VK

#endif //DEBUG_ENABLED
}