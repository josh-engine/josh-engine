//
// Created by Ember Lee on 3/22/24.
//

#ifndef JOSHENGINE_ENGINEAUDIO_H
#define JOSHENGINE_ENGINEAUDIO_H
#include <glm/glm.hpp>
#include <al.h>
#include <string>

void setVolume(float volume);
ALuint oggToBuffer(const std::string& filePath);

class Sound {
public:
    ALuint sourceID{};
    ALuint bufferID{};
    glm::vec3 position{};
    glm::vec3 velocity{};
    bool isLooping;
    Sound(glm::vec3 pos, glm::vec3 vel, const std::string& filePath, bool loop, float halfVolumeDistance, float min, float max, float gain) {
        isLooping = loop;
        position = pos;
        velocity = vel;

        // Create audio source
        alGenSources((ALuint)1, &sourceID);
        alSourcef(sourceID, AL_PITCH, 1);
        alSourcef(sourceID, AL_GAIN, gain);
        alSourcef(sourceID, AL_MAX_GAIN, max);
        alSourcef(sourceID, AL_MIN_GAIN, min);
        alSourcef(sourceID, AL_REFERENCE_DISTANCE, halfVolumeDistance);
        updateSource();

        // Fill buffer
        bufferID = oggToBuffer(filePath);
    }
    void updateSource() const {
        alSource3f(sourceID, AL_POSITION, position.x, position.y, position.z);
        alSource3f(sourceID, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
        alSourcei(sourceID, AL_LOOPING, isLooping);
    }
    void play() {
        alSourceQueueBuffers(sourceID, 1, &bufferID);
        alSourcePlay(sourceID);
    }
    void stop() const {
        alSourceStop(sourceID);
    }
};

void initAudio();
void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec);

#endif //JOSHENGINE_ENGINEAUDIO_H
