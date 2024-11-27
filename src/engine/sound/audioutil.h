//
// Created by Ember Lee on 3/22/24.
//

#ifndef JOSHENGINE_AUDIOUTIL_H
#define JOSHENGINE_AUDIOUTIL_H
#include <glm/glm.hpp>
#include <string>
#include <thread>

#define UNIX_CURRENT_TIME_MS static_cast<uint64_t>(duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count())

typedef glm::vec<3, float, (glm::qualifier)3> vec3_MSVC;

namespace JE { namespace Audio {
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

enum MusicTrackCommandType {
    PLAY,
    STOP,
    QUEUE,
    UNQUEUE,
    SET_VOLUME,
    SET_BUF,
    CLOSE_SOURCE,
    NOP // Just in case
};

struct MusicTrackCommand {
    MusicTrackCommandType t;
    uint64_t executeTime;
    uint8_t idx;
    union {
        uint32_t bufID;
        float vol;
    };

    MusicTrackCommand() {
        this->t = NOP;
    }

    MusicTrackCommand(uint64_t time) {
        this->t = NOP;
        this->executeTime = time;
    }

    MusicTrackCommand(MusicTrackCommandType type, uint64_t time, uint8_t idx) {
        this->t = type;
        this->executeTime = time;
        this->idx = idx;
    }

    MusicTrackCommand(MusicTrackCommandType type, uint64_t time, uint8_t idx, uint32_t buf) {
        this->t = type;
        this->executeTime = time;
        this->idx = idx;
        this->bufID = buf;
    }

    MusicTrackCommand(MusicTrackCommandType type, uint64_t time, float vol) {
        this->t = type;
        this->executeTime = time;
        this->vol = vol;
    }
};

class MusicTrack {
public:
    unsigned int sourceID{};
    unsigned int bufferIDs[256]{};
    uint8_t bufCount{};
    bool isPlaying{};
    MusicTrackCommand command{};
    std::atomic<bool> commandPresent{};

    MusicTrack();
    ~MusicTrack();

    void play();
    void queue(uint8_t idx);
    void unqueue(uint8_t idx);
    void queue(uint8_t idx, uint64_t msOff);
    void unqueue(uint8_t idx, uint64_t msOff);
    void waitMS(uint64_t msOff);
    void stop();
    void setVolume(float volume);
    void setBuffer(uint8_t idx, unsigned int buf);
    void sendCommand(MusicTrackCommand command);

#ifndef AUDIO_DISABLE
private:
    std::unique_ptr<std::thread> commandThread{};
#endif
};

void init();
void updateListener(glm::vec3 position, glm::vec3 velocity, glm::vec3 lookVec, glm::vec3 upVec);
}}
#endif //JOSHENGINE_AUDIOUTIL_H