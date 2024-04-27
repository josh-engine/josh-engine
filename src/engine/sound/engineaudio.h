//
// Created by Ember Lee on 3/22/24.
//

#ifndef JOSHENGINE_ENGINEAUDIO_H
#define JOSHENGINE_ENGINEAUDIO_H
#include <glm/glm.hpp>
#include <string>

void setVolume(float volume);
unsigned int oggToBuffer(const std::string& filePath);

class Sound {
public:
    unsigned int sourceID{};
    unsigned int bufferID{};
    glm::vec3 position{};
    glm::vec3 velocity{};
    bool isLooping;
    Sound(glm::vec3 pos, glm::vec3 vel, const std::string& filePath, bool loop, float halfVolumeDistance, float min, float max, float gain);
    void updateSource() const;
    void play();
    void stop() const;
};

void initAudio();
void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec);

#endif //JOSHENGINE_ENGINEAUDIO_H
