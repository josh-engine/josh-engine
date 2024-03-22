//
// Created by Ember Lee on 3/22/24.
//

#include <iostream>
#include <al.h>
#include <alc.h>
#include <glm/glm.hpp>
#include "../../stb/stb_vorbis.c"

ALCdevice* device;
ALCcontext* context;
glm::vec3 lpos;

glm::vec3 getListenerPos(){
    return lpos;
}

void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec){
    lpos = position;
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALfloat orientation[] = {lookVec.x, lookVec.y, lookVec.z, upVec.x, upVec.y, upVec.z};
    alListenerfv(AL_ORIENTATION, orientation);
}

void initAudio(){
    device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to initiate OpenAL device!" << std::endl;
        exit(1);
    }

    // Clear error stack just in case the user wants to check for errors on their own.
    // TODO: add easy check error function (use ALUT? or maybe figure out how to get error string without something like that)
    alGetError();

    context = alcCreateContext(device, NULL);
    if (!alcMakeContextCurrent(context)) {
        std::cerr << "Failed to create OpenAL context!" << std::endl;
        exit(1);
    }

    alDistanceModel(AL_INVERSE_DISTANCE);

}

ALuint oggToBuffer(std::string filePath, bool stereo){
    int channels, sampleRate, samples;
    short* data;
    samples = stb_vorbis_decode_filename(filePath.c_str(), &channels, &sampleRate, &data);
    ALuint buffer;
    alGenBuffers(1, &buffer);
    if (stereo) alBufferData(buffer, AL_FORMAT_STEREO16, data, samples*2*sizeof(short), sampleRate);
    else alBufferData(buffer, AL_FORMAT_MONO16, data, samples*2*sizeof(short), sampleRate*2);
    return buffer;
}