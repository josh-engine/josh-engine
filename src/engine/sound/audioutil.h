//
// Created by Ember Lee on 3/22/24.
//

#ifndef JOSHENGINE_AUDIOUTIL_H
#define JOSHENGINE_AUDIOUTIL_H
#include <glm/glm.hpp>
#include <string>

typedef glm::vec<3, float, (glm::qualifier)3> vec3_MSVC;

void setMasterVolume(float volume);
unsigned int oggToBuffer(const std::string& filePath);

class Sound {
public:
    unsigned int sourceID{};
    unsigned int bufferID{};
    glm::vec3 position{};
    glm::vec3 velocity{};
    bool isLooping;
    bool isPaused;
    // If we are not using MSVC
#ifndef _MSC_VER
    Sound(glm::vec3 pos, glm::vec3 vel, const std::string& filePath, bool loop, float halfVolumeDistance, float min, float max, float gain);
    // If we are using MSVC
#else
    Sound(vec3_MSVC pos, vec3_MSVC vel, const std::string& filePath, bool loop, float halfVolumeDistance, float min, float max, float gain);
#endif
    void updateSource() const;
    void play();
    void stop() const;
    void pause();
    void togglePaused();
    void deleteSource() const;
    void setGain(float gain) const;

    [[nodiscard]] bool isPlaying() const;

};

class MusicTrack {
public:
    unsigned int sourceID{};
    unsigned int bufferIDs[256]{};
    uint8_t bufCount{};
    bool isPlaying;

    MusicTrack();

    void play();
    void queue(uint8_t idx);
    void stop();
    void setVolume(float volume);
    void setBuffer(uint8_t idx, unsigned int buf);
};

void initAudio();
void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec);

#endif //JOSHENGINE_AUDIOUTIL_H
