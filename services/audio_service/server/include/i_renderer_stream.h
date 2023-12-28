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

#ifndef I_RENDERER_STREAM_H
#define I_RENDERER_STREAM_H

#include "i_stream.h"
#include "audio_info.h"
#include "audio_stream_info.h"

namespace OHOS {
namespace AudioStandard {
class IWriteCallback {
public:
    virtual int32_t OnWriteData(size_t length) = 0;
};

class IRendererStream : public IStream {
public:
    virtual ~IRendererStream() = default;
    virtual int32_t GetStreamFramesWritten(uint64_t &framesWritten) = 0;
    virtual int32_t GetCurrentTimeStamp(uint64_t &timeStamp) = 0;
    virtual int32_t GetLatency(uint64_t &latency) = 0;
    virtual int32_t SetRate(int32_t rate) = 0;
    virtual int32_t SetLowPowerVolume(float volume) = 0;
    virtual int32_t GetLowPowerVolume(float &volume) = 0;
    virtual int32_t SetAudioEffectMode(int32_t effectMode) = 0;
    virtual int32_t GetAudioEffectMode(int32_t &effectMode) = 0;
    virtual int32_t SetPrivacyType(int32_t privacyType) = 0;
    virtual int32_t GetPrivacyType(int32_t &privacyType) = 0;

    virtual void RegisterWriteCallback(const std::weak_ptr<IWriteCallback> &callback) = 0;
    virtual int32_t GetMinimumBufferSize(size_t &minBufferSize) const = 0;
    virtual void GetByteSizePerFrame(size_t &byteSizePerFrame) const = 0;
    virtual void GetSpanSizePerFrame(size_t &spanSizeInFrame) const = 0;
    virtual void AbortCallback(int32_t abortTimes) = 0;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // I_RENDERER_STREAM_H