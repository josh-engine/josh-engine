//
// Created by Ember Lee on 3/22/24.
//

#include "audioutil.h"
#ifndef AUDIO_DISABLE
#include <iostream>
#include <al.h>
#include <alc.h>
#include <stb_vorbis.h>
#include <unordered_map>
#include <thread>
#include <queue>
#endif

#ifndef AUDIO_DISABLE
ALCdevice* alDevice;
ALCcontext* context;
std::unordered_map<std::string, unsigned int> audioMap;
#endif
glm::vec3 lpos;

void setMasterVolume(float volume) {
#ifndef AUDIO_DISABLE
    alListenerf(AL_GAIN, volume);
#endif
}

glm::vec3 getListenerPos() {
    return lpos;
}

void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec) {
#ifndef AUDIO_DISABLE
    lpos = position;
    alListener3f(AL_POSITION, position.x, position.y, position.z);
    alListener3f(AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    ALfloat orientation[] = {lookVec.x, lookVec.y, lookVec.z, upVec.x, upVec.y, upVec.z};
    alListenerfv(AL_ORIENTATION, orientation);
#endif
}

void initAudio() {
#ifndef AUDIO_DISABLE
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
#endif
}

unsigned int oggToBuffer(const std::string& filePath) {
#ifndef AUDIO_DISABLE
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
#else
    return 0;
#endif
}
// If are not using MSVC
#ifndef _MSC_VER
Sound::Sound(glm::vec3 pos, glm::vec3 vel, const std::string &filePath, bool loop, float halfVolumeDistance, float min, float max, float gain) {
    // If we are using MSVC
#else
    Sound::Sound(vec3_MSVC pos, vec3_MSVC vel, const std::string &filePath, bool loop, float halfVolumeDistance, float min, float max, float gain) {
#endif
#ifndef AUDIO_DISABLE
    isLooping = loop;
    position = pos;
    velocity = vel;
    isPaused = false;

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
    alSourceQueueBuffers(sourceID, 1, &bufferID);
#endif
}

void Sound::updateSource() const {
#ifndef AUDIO_DISABLE
    alSource3f(sourceID, AL_POSITION, position.x, position.y, position.z);
    alSource3f(sourceID, AL_VELOCITY, velocity.x, velocity.y, velocity.z);
    alSourcei(sourceID, AL_LOOPING, isLooping);
#endif
}

void Sound::play() {
#ifndef AUDIO_DISABLE
    alSourcePlay(sourceID);
    isPaused = false;
#endif
}

void Sound::stop() const {
#ifndef AUDIO_DISABLE
    alSourceStop(sourceID);
#endif
}

void Sound::pause() {
#ifndef AUDIO_DISABLE
    alSourcePause(sourceID);
    isPaused = true;
#endif
}

void Sound::togglePaused() {
#ifndef AUDIO_DISABLE
    if (isPaused) play();
    else pause();
#endif
}

bool Sound::isPlaying() const {
#ifndef AUDIO_DISABLE
    int result;
    alGetSourcei(sourceID, AL_SOURCE_STATE, &result);
    return result == AL_PLAYING;
#else
    return false;
#endif
}

void Sound::deleteSource() const {
#ifndef AUDIO_DISABLE
    alDeleteSources((ALuint)1, &sourceID);
#endif
}

void Sound::setGain(float gain) const {
#ifndef AUDIO_DISABLE
    alSourcef(sourceID, AL_GAIN, gain);
#endif
}

void threadMan(MusicTrack* musicTrack); // For my own sanity it's below MusicTrack's constructor and destructor

MusicTrack::MusicTrack() {
#ifndef AUDIO_DISABLE
    this->commandThread = std::make_unique<std::thread>(threadMan, this);
#endif
}

void MusicTrack::sendCommand(MusicTrackCommand command) {
    this->command = command;       //  Set command
    commandPresent = true;        //   Say "we have a thing"
    while (commandPresent) {}    //    Block until thread ack
}

MusicTrack::~MusicTrack() {
    sendCommand({CLOSE_SOURCE, 0, static_cast<uint8_t>(0)});
    this->commandThread->join();
}

void threadMan(MusicTrack* musicTrack) {
    while (context == nullptr) {std::this_thread::sleep_for(std::chrono::milliseconds(1));} // Wait for OpenAL init
    alcMakeContextCurrent(context);
    musicTrack->isPlaying = false;
    alGenSources((ALuint)1, &musicTrack->sourceID);
    alSourcef(musicTrack->sourceID, AL_PITCH, 1);
    alSourcef(musicTrack->sourceID, AL_GAIN, 1.0f);
    alSourcef(musicTrack->sourceID, AL_MAX_GAIN, 1.0f);
    alSourcef(musicTrack->sourceID, AL_MIN_GAIN, 0.0f);
    alSourcei(musicTrack->sourceID, AL_LOOPING, false);

    std::queue<MusicTrackCommand> commandQueue{};
    std::queue<uint8_t> unqueueQueue{};
    bool unqueueRequest = false;
    while (true) {
        ALenum res;
        alGetSourcei(musicTrack->sourceID, AL_SOURCE_STATE, &res);
        if (unqueueRequest && res != AL_PLAYING && musicTrack->isPlaying) {
            while (!unqueueQueue.empty()) {
                alSourceUnqueueBuffers(musicTrack->sourceID, 1, &musicTrack->bufferIDs[unqueueQueue.front()]);
                unqueueQueue.pop();
            }
            unqueueRequest = false;
            alSourcei(musicTrack->sourceID, AL_LOOPING, true);
            alSourcePlay(musicTrack->sourceID);
        }
        if (musicTrack->commandPresent) {
            musicTrack->commandPresent = false;
            if (musicTrack->command.t != CLOSE_SOURCE) {
                commandQueue.push(musicTrack->command);
                musicTrack->command.t = NOP; // Sometimes duplicate commands are issued. This prevents that.
            } else {
                return; // Be free from your enslavement
            }
        } else if (!commandQueue.empty()) {
            // Process commands because yes
            MusicTrackCommand command = commandQueue.front();
            bool skipPop = false;
            alGetError(); // Clear error
            if (command.executeTime <= UNIX_CURRENT_TIME_MS) {
                switch (command.t) {
                    case PLAY:
                        std::cout << "play" << std::endl;
                        alSourcePlay(musicTrack->sourceID);
                        musicTrack->isPlaying = true;
                        break;
                    case STOP:
                        std::cout << "stop" << std::endl;
                        alSourceStop(musicTrack->sourceID);
                        musicTrack->isPlaying = false;
                        break;
                    case QUEUE:
                        std::cout << "queue" << std::endl;
                        alSourceQueueBuffers(musicTrack->sourceID, 1, &musicTrack->bufferIDs[command.idx]);
                        break;
                    case UNQUEUE:
                        std::cout << "unqueue" << std::endl;
                        if (!unqueueRequest) alSourcei(musicTrack->sourceID, AL_LOOPING, false);
                        unqueueRequest = true;
                        unqueueQueue.push(command.idx);
                        break;
                    case SET_VOLUME:
                        std::cout << "setvol" << std::endl;
                        alSourcef(musicTrack->sourceID, AL_GAIN, command.vol);
                        break;
                    case SET_BUF:
                        std::cout << "setbuf" << std::endl;
                        musicTrack->bufferIDs[command.idx] = command.bufID;
                        break;
                    case NOP: // Just in case
                        std::cout << "nop" << std::endl;
                        break;
                    case CLOSE_SOURCE:
                        std::cout
                                << "Ember owes Noah 3 dollars. \nPlease create a PR on the JoshEngine GitHub with a screenshot of the log attached."
                                << std::endl;
                        break;
                }
                ALint err = alGetError();
                if (err == 0 && !skipPop) {
                    commandQueue.pop();
                } else {
                    std::cerr << "COMMAND: " << command.t << " ERR: " << err << std::endl;
                }
            }
        }
    }
}

void MusicTrack::play() {
    sendCommand(MusicTrackCommand(PLAY, UNIX_CURRENT_TIME_MS, static_cast<uint8_t>(0)));
}

void MusicTrack::stop() {
    sendCommand(MusicTrackCommand(STOP, UNIX_CURRENT_TIME_MS, static_cast<uint8_t>(0)));
}

void MusicTrack::queue(uint8_t idx) {
    sendCommand(MusicTrackCommand(QUEUE, UNIX_CURRENT_TIME_MS, idx));
}

void MusicTrack::unqueue(uint8_t idx){
    sendCommand(MusicTrackCommand(UNQUEUE, UNIX_CURRENT_TIME_MS, idx));
}

void MusicTrack::setVolume(float volume) {
    sendCommand(MusicTrackCommand(SET_VOLUME, UNIX_CURRENT_TIME_MS, volume));
}

void MusicTrack::setBuffer(uint8_t idx, unsigned int buf) {
    sendCommand(MusicTrackCommand(SET_BUF, UNIX_CURRENT_TIME_MS, idx, buf));
}