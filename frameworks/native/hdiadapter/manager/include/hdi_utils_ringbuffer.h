/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef HDI_UTILS_RINGBUFFER_H
#define HDI_UTILS_RINGBUFFER_H

#include <mutex>
#include <string>

namespace OHOS {
namespace AudioStandard {

enum RingBufferState {
    RINGBUFFER_EMPTY = 0,
    RINGBUFFER_FULL,
    RINGBUFFER_HALFFULL
};

typedef struct RingBuffer {
    uint8_t *data; // BYTE
    int length;
} RingBuffer;

class HdiRingBuffer {
public:
    HdiRingBuffer();
    ~HdiRingBuffer();

    void Init(const int32_t sampleRate, const int32_t channelCount, const int32_t formatBytes,
        const int32_t onceFrameNum = 2, const int32_t maxFrameNum = 5);
    // read
    RingBuffer AcquireOutputBuffer();
    int32_t ReleaseOutputBuffer(RingBuffer &item);

    // write
    RingBuffer DequeueInputBuffer();
    int32_t EnqueueInputBuffer(RingBuffer &item);

private:
    enum RingBufferState GetRingBufferStatus();
    int32_t GetRingBufferDataLen();
    void AddWriteIndex(const int32_t &length);
    void AddReadIndex(const int32_t &length);

private:
    RingBuffer ringBuffer_;
    RingBuffer outputBuffer_;
    RingBuffer inputBuffer_;
    int32_t readIndex_;
    bool readFull_;
    int32_t writeIndex_;
    bool writeFull_;
    int32_t maxBufferSize_;
    int32_t perFrameLength_;
    std::mutex mtx_;
};

} // namespace AudioStandard
} // namespace OHOS
#endif // HDI_UTILS_RINGBUFFER_H
