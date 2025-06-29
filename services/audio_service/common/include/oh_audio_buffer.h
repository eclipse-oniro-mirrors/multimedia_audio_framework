/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OH_AUDIO_BUFFER_H
#define OH_AUDIO_BUFFER_H

#include <atomic>
#include <string>

#include "message_parcel.h"

#include "audio_info.h"
#include "audio_shared_memory.h"

namespace OHOS {
namespace AudioStandard {
const size_t BASIC_SYNC_INFO_SIZE = 8; // sizeof(uint32_t) * 2
// client or server.
enum AudioBufferHolder : uint32_t {
    // normal stream, Client buffer created when readFromParcel
    AUDIO_CLIENT = 0,
    // normal stream, Server buffer shared with Client
    AUDIO_SERVER_SHARED,
    // normal stream, Server buffer shared with hdi
    AUDIO_SERVER_ONLY,
    // Server buffer shared with hdi and has sync info
    AUDIO_SERVER_ONLY_WITH_SYNC,
    // Independent stream
    AUDIO_SERVER_INDEPENDENT
};

enum StreamStatus : uint32_t {
    STREAM_IDEL = 0,
    STREAM_STARTING,
    STREAM_RUNNING,
    STREAM_PAUSING,
    STREAM_PAUSED,
    STREAM_STOPPING,
    STREAM_STOPPED,
    STREAM_RELEASED,
    STREAM_STAND_BY,
    STREAM_INVALID
};

/**
 * totalSizeInFrame = spanCount * spanSizeInFrame
 *
 * 0 <= write - base < 2 * totalSize
 * 0 <= read - base < 1 * totalSize
 * 0 <= write - read < 1 * totalSize
 */
struct BasicBufferInfo {
    uint32_t totalSizeInFrame;
    uint32_t spanSizeInFrame;
    uint32_t byteSizePerFrame;

    std::atomic<uint32_t> futexObj;

    std::atomic<StreamStatus> streamStatus;

    // basic read/write postion
    std::atomic<uint64_t> basePosInFrame;
    std::atomic<uint64_t> curWriteFrame;
    std::atomic<uint64_t> curReadFrame;

    std::atomic<uint32_t> underrunCount;

    std::atomic<uint64_t> handlePos;
    std::atomic<int64_t> handleTime;

    std::atomic<uint64_t> position;
    std::atomic<uint64_t> timeStamp;

    std::atomic<float> streamVolume;
    std::atomic<float> duckFactor;
    std::atomic<float> muteFactor;
    std::atomic<RestoreStatus> restoreStatus = NO_NEED_FOR_RESTORE;
    std::atomic<bool> isNeedStop = false;

    RestoreInfo restoreInfo;
};

enum SpanStatus : uint32_t {
    SPAN_IDEL = 0,
    SPAN_WRITTING,
    SPAN_WRITE_DONE,
    SPAN_READING,
    SPAN_READ_DONE,
    SPAN_INVALID
};

// one span represents a collection of audio sampling data for a short period of time
struct SpanInfo {
    std::atomic<SpanStatus> spanStatus;
    uint64_t offsetInFrame = 0;

    int64_t writeStartTime;
    int64_t writeDoneTime;

    int64_t readStartTime;
    int64_t readDoneTime;

    // volume info for each span
    bool isMute;
    int32_t volumeStart;
    int32_t volumeEnd;
};

class OHAudioBuffer {
public:
    static const int INVALID_BUFFER_FD = -1;
    OHAudioBuffer(AudioBufferHolder bufferHolder, uint32_t totalSizeInFrame, uint32_t spanSizeInFrame,
        uint32_t byteSizePerFrame);
    ~OHAudioBuffer();

    // create OHAudioBuffer locally or remotely
    static std::shared_ptr<OHAudioBuffer> CreateFromLocal(uint32_t totalSizeInFrame, uint32_t spanSizeInFrame,
        uint32_t byteSizePerFrame);
    static std::shared_ptr<OHAudioBuffer> CreateFromRemote(uint32_t totalSizeInFrame, uint32_t spanSizeInFrame,
        uint32_t byteSizePerFrame, AudioBufferHolder holder, int dataFd, int infoFd = INVALID_BUFFER_FD);

    // for ipc.
    static int32_t WriteToParcel(const std::shared_ptr<OHAudioBuffer> &buffer, MessageParcel &parcel);
    static std::shared_ptr<OHAudioBuffer> ReadFromParcel(MessageParcel &parcel);

    AudioBufferHolder GetBufferHolder();

    int32_t GetSizeParameter(uint32_t &totalSizeInFrame, uint32_t &spanSizeInFrame, uint32_t &byteSizePerFrame);

    std::atomic<StreamStatus> *GetStreamStatus();

    uint32_t GetUnderrunCount();

    bool SetUnderrunCount(uint32_t count);

    bool GetHandleInfo(uint64_t &frames, int64_t &nanoTime);

    void SetHandleInfo(uint64_t frames, int64_t nanoTime);

    float GetStreamVolume();
    bool SetStreamVolume(float streamVolume);

    float GetDuckFactor();
    bool SetDuckFactor(float duckFactor);

    float GetMuteFactor();
    bool SetMuteFactor(float muteFactor);

    int32_t GetAvailableDataFrames();

    int32_t ResetCurReadWritePos(uint64_t readFrame, uint64_t writeFrame);

    uint64_t GetCurWriteFrame();
    uint64_t GetCurReadFrame();

    int32_t SetCurWriteFrame(uint64_t writeFrame);
    int32_t SetCurReadFrame(uint64_t readFrame);

    uint32_t GetSessionId();
    int32_t SetSessionId(uint32_t sessionId);

    int32_t GetWriteBuffer(uint64_t writePosInFrame, BufferDesc &bufferDesc);

    int32_t GetReadbuffer(uint64_t readPosInFrame, BufferDesc &bufferDesc);

    int32_t GetBufferByFrame(uint64_t posInFrame, BufferDesc &bufferDesc);

    SpanInfo *GetSpanInfo(uint64_t posInFrame);
    SpanInfo *GetSpanInfoByIndex(uint32_t spanIndex);

    uint32_t GetSpanCount();

    int64_t GetLastWrittenTime();
    void SetLastWrittenTime(int64_t time);

    // hdi use one span as one frame
    uint32_t GetSyncWriteFrame();
    bool SetSyncWriteFrame(uint32_t writeFrame);
    uint32_t GetSyncReadFrame();
    bool SetSyncReadFrame(uint32_t readFrame);

    std::atomic<uint32_t> *GetFutex();
    uint8_t *GetDataBase();
    size_t GetDataSize();
    RestoreStatus CheckRestoreStatus();
    RestoreStatus SetRestoreStatus(RestoreStatus restoreStatus);
    void GetRestoreInfo(RestoreInfo &restoreInfo);
    void SetRestoreInfo(RestoreInfo restoreInfo);

    void GetTimeStampInfo(uint64_t &position, uint64_t &timeStamp);
    void SetTimeStampInfo(uint64_t position, uint64_t timeStamp);

    void SetStopFlag(bool isNeedStop);
    bool GetStopFlag() const;

private:
    int32_t Init(int dataFd, int infoFd);
    int32_t SizeCheck();
    void InitBasicBufferInfo();

    uint32_t sessionId_ = 0;
    AudioBufferHolder bufferHolder_;
    uint32_t totalSizeInFrame_;
    uint32_t spanSizeInFrame_;
    uint32_t byteSizePerFrame_;

    // available only in single process
    int64_t lastWrittenTime_ = 0;

    // calculated in advance
    size_t totalSizeInByte_ = 0;
    size_t spanSizeInByte_ = 0;
    uint32_t spanConut_ = 0;

    // for render or capturer
    AudioMode audioMode_;

    // for StatusInfo buffer
    std::shared_ptr<AudioSharedMemory> statusInfoMem_ = nullptr;
    BasicBufferInfo *basicBufferInfo_ = nullptr;
    SpanInfo *spanInfoList_ = nullptr;

    // for audio data buffer
    std::shared_ptr<AudioSharedMemory> dataMem_ = nullptr;
    uint8_t *dataBase_ = nullptr;
    volatile uint32_t *syncReadFrame_ = nullptr;
    volatile uint32_t *syncWriteFrame_ = nullptr;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // OH_AUDIO_BUFFER_H
