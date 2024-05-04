//
// Created by Ember Lee on 3/22/24.
//

#include "engineaudio.h"
#include <iostream>
#include <map>
#include <al.h>
#include <alc.h>
#include <stb_vorbis.h>
#include <unordered_map>

ALCdevice* alDevice;
ALCcontext* context;
glm::vec3 lpos;

void setVolume(float volume) {
    alListenerf(AL_GAIN, volume);
}

glm::vec3 getListenerPos() {
    return lpos;
}

void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec) {
    lpos = position;
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALfloat orientation[] = {lookVec.x, lookVec.y, lookVec.z, upVec.x, upVec.y, upVec.z};
    alListenerfv(AL_ORIENTATION, orientation);
}

void initAudio() {
    alDevice = alcOpenDevice(nullptr);
    if (!alDevice) {
        std::cerr << "Failed to initiate OpenAL device!" << std::endl;
        exit(1);
    }

    // Clear error stack just in case the user wants to check for errors on their own.
    // TODO: add easy check error function (use ALUT? or maybe figure out how to get error string without something like that)
    alGetError();

    context = alcCreateContext(alDevice, nullptr);
    if (!alcMakeContextCurrent(context)) {
        std::cerr << "Failed to create OpenAL context!" << std::endl;
        exit(1);
    }

    alDistanceModel(AL_INVERSE_DISTANCE);
}

std::unordered_map<std::string, unsigned int> audioMap;

unsigned int oggToBuffer(const std::string& filePath) {
    if (audioMap.contains(filePath)) return audioMap.at(filePath);
    int channels, sampleRate, samples;
    short* data;
    int error;
    stb_vorbis *v = stb_vorbis_open_filename(filePath.c_str(), &error, nullptr);
    stb_vorbis_info info = stb_vorbis_get_info(v);
    samples = stb_vorbis_decode_filename(filePath.c_str(), &channels, &sampleRate, &data);
    ALuint buffer;
    alGenBuffers(1, &buffer);                 //      needless cast so the compiler will stop yelling at me
    if (info.channels > 1) alBufferData(buffer, AL_FORMAT_STEREO16, data, samples*2*static_cast<int>(sizeof(short)), sampleRate);
    else alBufferData(buffer, AL_FORMAT_MONO16, data, samples*static_cast<int>(sizeof(short)), sampleRate);
    audioMap.insert({filePath, buffer});
    return buffer;
}
// If are not using MVSC
#ifndef _MSC_VER
Sound::Sound(glm::vec3 pos, glm::vec3 vel, const std::string &filePath, bool loop, float halfVolumeDistance, float min, float max, float gain) {
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
// If we are using MVSC
#else
Sound::Sound(vec3_mvsc pos, vec3_mvsc vel, const std::string &filePath, bool loop, float halfVolumeDistance, float min, float max, float gain) {
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
#endif

void Sound::updateSource() const {
    alSource3f(sourceID, AL_POSITION, position.x, position.y, position.z);
    alSource3f(sourceID, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    alSourcei(sourceID, AL_LOOPING, isLooping);
}

void Sound::play() {
    alSourceQueueBuffers(sourceID, 1, &bufferID);
    alSourcePlay(sourceID);
}

void Sound::stop() const {
    alSourceStop(sourceID);
}