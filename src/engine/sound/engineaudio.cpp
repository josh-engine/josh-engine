//
// Created by Ember Lee on 3/22/24.
//

#include "engineaudio.h"
#include <iostream>
#include <map>
#include <alc.h>
#include "../../stb/stb_vorbis.c"

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

std::unordered_map<std::string, ALuint> audioMap;

ALuint oggToBuffer(const std::string& filePath) {
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