/*
 * @Author: Ricken
 * @Email: me@ricken.cn
 * @Date: 2026-07-06
 * @FilePath: /kk_frame/src/app/managers/audio_mgr.cc
 * @Description: ALSA 音频播放管理器
 */

#include "audio_mgr.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <vector>

#ifdef ENABLE_AUDIO
#include <alsa/asoundlib.h>
#endif

namespace {

    struct WavInfo {
        uint16_t format{ 0 };
        uint16_t channels{ 0 };
        uint32_t sampleRate{ 0 };
        uint16_t bitsPerSample{ 0 };
        uint32_t blockAlign{ 0 };
        uint64_t dataOffset{ 0 };
        uint64_t dataSize{ 0 };
    };

    template <typename T>
    bool readValue(std::ifstream& stream, T& value) {
        stream.read(reinterpret_cast<char*>(&value), sizeof(value));
        return stream.good();
    }

    bool readWavHeader(std::ifstream& stream, WavInfo& info, std::string& error) {
        char riff[4], wave[4];
        uint32_t riffSize;
        stream.read(riff, 4);
        if (!readValue(stream, riffSize)) return false;
        stream.read(wave, 4);
        if (!stream.good() || std::memcmp(riff, "RIFF", 4) || std::memcmp(wave, "WAVE", 4)) {
            error = "Not a RIFF/WAVE file";
            return false;
        }

        bool haveFormat = false;
        while (stream.good()) {
            char chunkId[4];
            uint32_t chunkSize;
            stream.read(chunkId, 4);
            if (!readValue(stream, chunkSize)) break;
            const std::streamoff chunkStart = stream.tellg();

            if (!std::memcmp(chunkId, "fmt ", 4)) {
                uint32_t byteRate;
                if (chunkSize < 16 || !readValue(stream, info.format) ||
                    !readValue(stream, info.channels) || !readValue(stream, info.sampleRate) ||
                    !readValue(stream, byteRate) || !readValue(stream, info.blockAlign) ||
                    !readValue(stream, info.bitsPerSample)) {
                    error = "Invalid WAV format chunk";
                    return false;
                }
                haveFormat = true;
            } else if (!std::memcmp(chunkId, "data", 4)) {
                if (!haveFormat) {
                    error = "WAV data chunk appears before format chunk";
                    return false;
                }
                info.dataOffset = static_cast<uint64_t>(stream.tellg());
                info.dataSize = chunkSize;
                break;
            }
            stream.seekg(chunkStart + static_cast<std::streamoff>(chunkSize + (chunkSize & 1)));
        }

        if (!info.dataOffset || !info.dataSize || info.format != 1 || !info.channels ||
            !info.sampleRate || !info.blockAlign) {
            error = "Only non-empty PCM WAV files are supported";
            return false;
        }
        return true;
    }

    void applyVolume(std::vector<char>& data, size_t size, uint16_t bits, float volume) {
        if (volume >= 0.999f) return;
        if (bits == 8) {
            for (size_t i = 0; i < size; ++i) {
                int sample = static_cast<unsigned char>(data[i]) - 128;
                data[i] = static_cast<char>(std::max(0, std::min(255, static_cast<int>(sample * volume) + 128)));
            }
        } else if (bits == 16) {
            int16_t* samples = reinterpret_cast<int16_t*>(data.data());
            for (size_t i = 0; i < size / 2; ++i) samples[i] = static_cast<int16_t>(samples[i] * volume);
        } else if (bits == 32) {
            int32_t* samples = reinterpret_cast<int32_t*>(data.data());
            for (size_t i = 0; i < size / 4; ++i) samples[i] = static_cast<int32_t>(samples[i] * volume);
        } else if (bits == 24) {
            for (size_t i = 0; i + 2 < size; i += 3) {
                int32_t sample = static_cast<unsigned char>(data[i]) |
                    (static_cast<unsigned char>(data[i + 1]) << 8) |
                    (static_cast<unsigned char>(data[i + 2]) << 16);
                if (sample & 0x800000) sample |= ~0xffffff;
                sample = static_cast<int32_t>(sample * volume);
                data[i] = sample & 0xff;
                data[i + 1] = (sample >> 8) & 0xff;
                data[i + 2] = (sample >> 16) & 0xff;
            }
        }
    }

} // namespace

AudioMgr::AudioMgr() : mThread(&AudioMgr::playbackLoop, this) { }

AudioMgr::~AudioMgr() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mExit = true;
        ++mRequestId;
    }
    mCondition.notify_all();
    if (mThread.joinable()) mThread.join();
}

bool AudioMgr::play(const std::string& filePath, bool loop) {
    if (filePath.empty() || !isSupported()) return false;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mFilePath = filePath;
        mLoop = loop;
        mLastError.clear();
        mPositionMs = 0;
        mDurationMs = 0;
        mSeekMs = -1;
        mState = State::Loading;
        ++mRequestId;
    }
    mCondition.notify_all();
    return true;
}

bool AudioMgr::pause() {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mState != State::Playing) return false;
    mState = State::Paused;
    return true;
}

bool AudioMgr::resume() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (mState != State::Paused) return false;
        mState = State::Playing;
    }
    mCondition.notify_all();
    return true;
}

void AudioMgr::stop() {
    {
        std::lock_guard<std::mutex> lock(mMutex);
        mState = State::Stopped;
        mPositionMs = 0;
        mSeekMs = -1;
        ++mRequestId;
    }
    mCondition.notify_all();
}

bool AudioMgr::seek(int64_t positionMs) {
    std::lock_guard<std::mutex> lock(mMutex);
    if (mState != State::Playing && mState != State::Paused) return false;
    mSeekMs = std::max<int64_t>(0, std::min(positionMs, mDurationMs));
    mPositionMs = mSeekMs;
    return true;
}

void AudioMgr::setLoop(bool loop) { std::lock_guard<std::mutex> lock(mMutex); mLoop = loop; }
bool AudioMgr::isLoop() const { std::lock_guard<std::mutex> lock(mMutex); return mLoop; }
void AudioMgr::setVolume(float volume) { std::lock_guard<std::mutex> lock(mMutex); mVolume = std::max(0.0f, std::min(1.0f, volume)); }
float AudioMgr::getVolume() const { std::lock_guard<std::mutex> lock(mMutex); return mVolume; }
AudioMgr::State AudioMgr::getState() const { std::lock_guard<std::mutex> lock(mMutex); return mState; }
bool AudioMgr::isPlaying() const { State state = getState(); return state == State::Playing; }
int64_t AudioMgr::getPosition() const { std::lock_guard<std::mutex> lock(mMutex); return mPositionMs; }
int64_t AudioMgr::getDuration() const { std::lock_guard<std::mutex> lock(mMutex); return mDurationMs; }
std::string AudioMgr::getCurrentFile() const { std::lock_guard<std::mutex> lock(mMutex); return mFilePath; }
std::string AudioMgr::getLastError() const { std::lock_guard<std::mutex> lock(mMutex); return mLastError; }

bool AudioMgr::isSupported() {
#ifdef ENABLE_AUDIO
    return true;
#else
    return false;
#endif
}

void AudioMgr::setErrorLocked(const std::string& error) {
    mLastError = error;
    mState = State::Error;
}

void AudioMgr::playbackLoop() {
#ifndef ENABLE_AUDIO
    return;
#else
    for (;;) {
        std::string path;
        uint64_t requestId;
        {
            std::unique_lock<std::mutex> lock(mMutex);
            mCondition.wait(lock, [this] { return mExit || mState == State::Loading; });
            if (mExit) return;
            path = mFilePath;
            requestId = mRequestId;
        }

        std::ifstream stream(path.c_str(), std::ios::binary);
        WavInfo wav;
        std::string error;
        if (!stream.is_open()) error = "Cannot open audio file: " + path;
        else if (!readWavHeader(stream, wav, error) && error.empty()) error = "Cannot read WAV header";

        snd_pcm_format_t format = SND_PCM_FORMAT_UNKNOWN;
        if (wav.bitsPerSample == 8) format = SND_PCM_FORMAT_U8;
        else if (wav.bitsPerSample == 16) format = SND_PCM_FORMAT_S16_LE;
        else if (wav.bitsPerSample == 24) format = SND_PCM_FORMAT_S24_3LE;
        else if (wav.bitsPerSample == 32) format = SND_PCM_FORMAT_S32_LE;
        if (error.empty() && format == SND_PCM_FORMAT_UNKNOWN) error = "Unsupported WAV sample width";

        snd_pcm_t* pcm = nullptr;
        if (error.empty()) {
            int result = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
            if (result < 0) error = std::string("Cannot open ALSA device: ") + snd_strerror(result);
        }
        if (error.empty()) {
            int result = snd_pcm_set_params(pcm, format, SND_PCM_ACCESS_RW_INTERLEAVED,
                wav.channels, wav.sampleRate, 1, 100000);
            if (result < 0) error = std::string("Cannot configure ALSA device: ") + snd_strerror(result);
        }

        if (!error.empty()) {
            if (pcm) snd_pcm_close(pcm);
            std::lock_guard<std::mutex> lock(mMutex);
            if (requestId == mRequestId) setErrorLocked(error);
            continue;
        }

        const uint64_t totalFrames = wav.dataSize / wav.blockAlign;
        {
            std::lock_guard<std::mutex> lock(mMutex);
            if (requestId != mRequestId) { snd_pcm_close(pcm); continue; }
            mDurationMs = static_cast<int64_t>(totalFrames * 1000 / wav.sampleRate);
            mState = State::Playing;
        }

        const size_t framesPerChunk = 2048;
        std::vector<char> buffer(framesPerChunk * wav.blockAlign);
        uint64_t framePosition = 0;
        bool pausedInAlsa = false;

        while (framePosition < totalFrames) {
            float volume;
            bool loop;
            int64_t seekMs;
            {
                std::unique_lock<std::mutex> lock(mMutex);
                if (mExit || requestId != mRequestId || mState == State::Stopped) break;
                if (mState == State::Paused) {
                    if (!pausedInAlsa) { snd_pcm_pause(pcm, 1); pausedInAlsa = true; }
                    mCondition.wait(lock, [this, requestId] {
                        return mExit || requestId != mRequestId || mState != State::Paused;
                    });
                    continue;
                }
                if (pausedInAlsa) { snd_pcm_pause(pcm, 0); pausedInAlsa = false; }
                seekMs = mSeekMs;
                mSeekMs = -1;
                volume = mVolume;
                loop = mLoop;
            }

            if (seekMs >= 0) {
                framePosition = std::min<uint64_t>(totalFrames,
                    static_cast<uint64_t>(seekMs) * wav.sampleRate / 1000);
                stream.clear();
                stream.seekg(static_cast<std::streamoff>(wav.dataOffset + framePosition * wav.blockAlign));
                snd_pcm_drop(pcm);
                snd_pcm_prepare(pcm);
                continue;
            }

            size_t frames = static_cast<size_t>(std::min<uint64_t>(framesPerChunk, totalFrames - framePosition));
            size_t bytes = frames * wav.blockAlign;
            stream.read(buffer.data(), bytes);
            size_t bytesRead = static_cast<size_t>(stream.gcount());
            frames = bytesRead / wav.blockAlign;
            if (!frames) break;
            applyVolume(buffer, frames * wav.blockAlign, wav.bitsPerSample, volume);

            size_t written = 0;
            while (written < frames) {
                snd_pcm_sframes_t result = snd_pcm_writei(pcm,
                    buffer.data() + written * wav.blockAlign, frames - written);
                if (result < 0) result = snd_pcm_recover(pcm, static_cast<int>(result), 1);
                if (result < 0) {
                    error = std::string("ALSA playback failed: ") + snd_strerror(static_cast<int>(result));
                    break;
                }
                written += static_cast<size_t>(result);
            }
            if (!error.empty()) break;
            framePosition += frames;
            {
                std::lock_guard<std::mutex> lock(mMutex);
                if (requestId == mRequestId)
                    mPositionMs = static_cast<int64_t>(framePosition * 1000 / wav.sampleRate);
            }

            if (framePosition >= totalFrames && loop) {
                framePosition = 0;
                stream.clear();
                stream.seekg(static_cast<std::streamoff>(wav.dataOffset));
            }
        }

        snd_pcm_drop(pcm);
        snd_pcm_close(pcm);
        std::lock_guard<std::mutex> lock(mMutex);
        if (requestId == mRequestId) {
            if (!error.empty()) setErrorLocked(error);
            else if (mState != State::Stopped) {
                mPositionMs = mDurationMs;
                mState = State::Completed;
            }
        }
    }
#endif
}
