/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef AUDIO_SUITE_COMMON_H
#define AUDIO_SUITE_COMMON_H

#include "audio_suite_log.h"
#include "audio_stream_info.h"

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {

static constexpr uint32_t SAMPLE_SIZE_1_BYTE = 1;
static constexpr uint32_t SAMPLE_SIZE_2_BYTE = 2;
static constexpr uint32_t SAMPLE_SIZE_3_BYTE = 3;
static constexpr uint32_t SAMPLE_SIZE_4_BYTE = 4;

class AudioSuiteRingBuffer {
public:
    AudioSuiteRingBuffer(uint32_t capacity) : capacity_(capacity), head_(0), tail_(0), size_(0)
    {
        CHECK_AND_RETURN_LOG(capacity_ != 0, "AudioSuiteRingBuffer malloc failed, capacity is zero.");
        buffer_ = new(std::nothrow) uint8_t[capacity_];
        CHECK_AND_RETURN_LOG(buffer_ != nullptr, "AudioSuiteRingBuffer malloc failed.");
    }

    ~AudioSuiteRingBuffer()
    {
        if (buffer_ != nullptr) {
            delete[] buffer_;
            buffer_ = nullptr;
        }
    }

    // 禁止拷贝和赋值
    AudioSuiteRingBuffer(const AudioSuiteRingBuffer&) = delete;
    AudioSuiteRingBuffer& operator=(const AudioSuiteRingBuffer&) = delete;

    // 存入数据
    int32_t PushData(uint8_t* byteData, uint32_t size);
    int32_t GetData(uint8_t* byteData, uint32_t size);
    int32_t ResizeBuffer(uint32_t size);
    int32_t ClearBuffer();
    bool CheckDataEnough(uint32_t size)
    {
        return size_ >= size;
    }

    uint32_t GetRestSpace()
    {
        return (capacity_ - size_);
    }

    uint32_t GetSize() const
    {
        return size_;
    }
    
private:
    uint8_t* buffer_ = nullptr;
    uint32_t capacity_;
    uint32_t head_;
    uint32_t tail_;
    uint32_t size_;
};

class AudioSuiteUtil {
public:
    static uint32_t GetSampleSize(AudioSampleFormat type)
    {
        uint32_t sampleSize = SAMPLE_SIZE_4_BYTE;
        switch (type) {
            case AudioSampleFormat::SAMPLE_U8:
                sampleSize = SAMPLE_SIZE_1_BYTE;
                break;
            case AudioSampleFormat::SAMPLE_S16LE:
                sampleSize = SAMPLE_SIZE_2_BYTE;
                break;
            case AudioSampleFormat::SAMPLE_S24LE:
                sampleSize = SAMPLE_SIZE_3_BYTE;
                break;
            case AudioSampleFormat::SAMPLE_S32LE:
            case AudioSampleFormat::SAMPLE_F32LE:
                sampleSize = SAMPLE_SIZE_4_BYTE;
                break;
            default:
                break;
        }
        return sampleSize;
    }
};
}
}
}
#endif