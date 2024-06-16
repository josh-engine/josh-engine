//
// Created by Ember Lee on 3/24/24.
//
#include "engineconfig.h"
#ifdef DEBUG_ENABLED
#include <string>
#include "enginedebug.h"
#include "engine.h"
#include "gfx/imgui/imgui.h"
#include <unordered_map>

#ifdef GFX_API_VK
#include "gfx/vk/gfx_vk.h"
bool vulkanMemoryView;
#endif

bool statView, gmObjView, texturesView, buffersView, graphicsView;
std::string selectedGameObject;
std::string selectedFunction;
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

inline std::string sizeFormat(size_t s) {
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

    ImGui::Checkbox("Stats", &statView);
    ImGui::Checkbox("GameObjects Editor", &gmObjView);
    ImGui::Checkbox("Show Graphics Menus", &graphicsView);
    if (graphicsView) {
        ImGui::Checkbox("Textures", &texturesView);
        ImGui::Checkbox("Uniform Memory Info", &buffersView);
#ifdef GFX_API_VK
        ImGui::Checkbox("vkAlloc Allocation View", &vulkanMemoryView);
#endif
    }

    ImGui::End();



    if (statView) {
        ImGui::Begin("Stats");

        ImGui::Text("Frame time: %ims (~%i fps)", static_cast<int>(getFrameTime()), static_cast<int>(1/(getFrameTime()/1000)));
        ImGui::Text("Renderables: %zu", getRenderableCount());

        ImGui::End();
    }

    if (gmObjView) {
        ImGui::Begin("GameObjects");

        std::unordered_map<std::string, GameObject>* gameObjects = getGameObjects();

        if (ImGui::BeginCombo("GameObject", selectedGameObject.c_str(), 0)) {
            for (const auto& object: *gameObjects) {
                if (ImGui::Selectable(object.first.c_str(), selectedGameObject == object.first))
                    selectedGameObject = object.first;
            }
            ImGui::EndCombo();
        }

        if (!selectedGameObject.empty()) {
            GameObject* gmObj = &gameObjects->at(selectedGameObject);

            if (ImGui::Button("Duplicate GameObject", {ImGui::GetWindowSize().x-20, ImGui::GetTextLineHeight()+5})){
                GameObject copy = gameObjects->at(selectedGameObject);
                gameObjects->insert({selectedGameObject + " (copy)", copy});
            }

            ImGui::Text("Renderable count: %zu", gmObj->renderables.size());

            ImGui::Text("Transform");
            ImGui::Indent();
            ImGui::InputFloat3("Position", &gmObj->transform.position[0]);
            ImGui::InputFloat3("Rotation", &gmObj->transform.rotation[0]);
            ImGui::InputFloat3("Scale",    &gmObj->transform.scale[0]   );
            ImGui::Unindent();

            ImGui::Text("Functions");
            ImGui::Indent();

            if (gmObj->onUpdate.empty())
                ImGui::Text("Empty");
            else {
                for (auto function: gmObj->onUpdate) {
                    if (functionNameMap.find(reinterpret_cast<void*>(function)) == functionNameMap.end()) {
                        ImGui::TextColored({0.75f, 0.75f, 0.75f, 1.0f}, "Function at %lx", (unsigned long) function);
                    } else {
                        ImGui::Text("%s", functionNameMap.at(reinterpret_cast<void*>(function)).c_str());
                    }
                }
                if (ImGui::BeginCombo("Remove", selectedFunction.c_str(), 0)) {
                    for (int i = 0; i < gmObj->onUpdate.size(); i++) {
                        std::string str;
                        if (functionNameMap.find(reinterpret_cast<void*>(gmObj->onUpdate[i])) == functionNameMap.end()) {
                            str = "Function at %lx";
                        } else {
                            str = functionNameMap.at(reinterpret_cast<void*>(gmObj->onUpdate[i]));
                        }
                        if (ImGui::Selectable(str.c_str())){
                            gmObj->onUpdate.erase(gmObj->onUpdate.begin() + i);
                            break;
                        }
                    }
                    ImGui::EndCombo();
                }
            }
            if (ImGui::BeginCombo("Add", selectedFunction.c_str(), 0)) {
                for (const auto& fn : functionNameMap) {
                    if (ImGui::Selectable(fn.second.c_str())) {
                        gmObj->onUpdate.push_back(reinterpret_cast<void (*)(double dt, GameObject* g)>(fn.first));
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Unindent();
        }

        ImGui::End();
    }

    if (texturesView && graphicsView) {
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

    if (buffersView && graphicsView) {
        ImGui::Begin("Uniform Buffers");
        for (size_t i = 0; i < getBufCount(); i++) {
            ImGui::Text("Buffer %zu", i);
            ImGui::Indent();
            JEUniformBufferReference_VK ref = getBuf(i);
            for (int j = 0; j < MAX_FRAMES_IN_FLIGHT; j++) {
                JEAllocation_VK alloc = (*ref.alloc)[j];
                ImGui::Text("Frame %i", j);
                ImGui::Indent();
                ImGui::Text("Allocation is %s", sizeFormat(alloc.size).c_str());
                ImGui::Text("Stored in block %i", alloc.memoryRefID);
                ImGui::Text("Map pointer is 0x%lx", (unsigned long) (*ref.map)[j]);
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        ImGui::End();
    }

#ifdef GFX_API_VK
    if (vulkanMemoryView && graphicsView) {
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