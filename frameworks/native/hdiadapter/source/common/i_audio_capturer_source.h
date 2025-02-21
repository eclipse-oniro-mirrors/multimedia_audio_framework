/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#ifndef I_AUDIO_CAPTURER_SOURCE_H
#define I_AUDIO_CAPTURER_SOURCE_H

#include <cstdint>

#include "audio_info.h"
#include "audio_hdiadapter_info.h"

namespace OHOS {
namespace AudioStandard {
typedef struct {
    const char *adapterName;
    uint32_t openMicSpeaker;
    HdiAdapterFormat format;
    uint32_t sampleRate;
    uint32_t channel;
    float volume;
    uint32_t bufferSize;
    bool isBigEndian;
    const char *filePath;
    const char *deviceNetworkId;
    int32_t deviceType;
    int32_t sourceType;
    uint64_t channelLayout;
} IAudioSourceAttr;

class IAudioSourceCallback {
public:
    virtual void OnWakeupClose() = 0;
    virtual void OnCapturerState(bool isActive) = 0;
    virtual void OnAudioSourceParamChange(const std::string &netWorkId, const AudioParamKey key,
        const std::string &condition, const std::string &value) = 0;
};

class IAudioCapturerSource {
public:
    static IAudioCapturerSource *GetInstance(const char *deviceClass, const char *deviceNetworkId,
           const SourceType sourceType = SourceType::SOURCE_TYPE_MIC, const char *sourceName = "Built_in_wakeup");
    static void GetAllInstance(std::vector<IAudioCapturerSource *> &allInstance);
    virtual ~IAudioCapturerSource() = default;

    virtual int32_t Init(const IAudioSourceAttr &attr) = 0;
    virtual bool IsInited(void) = 0;
    virtual void DeInit(void) = 0;

    virtual int32_t Start(void) = 0;
    virtual int32_t Stop(void) = 0;
    virtual int32_t Flush(void) = 0;
    virtual int32_t Reset(void) = 0;
    virtual int32_t Pause(void) = 0;
    virtual int32_t Resume(void) = 0;

    virtual int32_t CaptureFrame(char *frame, uint64_t requestBytes, uint64_t &replyBytes) = 0;

    virtual int32_t SetVolume(float left, float right) = 0;
    virtual int32_t GetVolume(float &left, float &right) = 0;
    virtual int32_t SetMute(bool isMute) = 0;
    virtual int32_t GetMute(bool &isMute) = 0;
    virtual int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) = 0;
    virtual int32_t SetInputRoute(DeviceType deviceType) = 0;
    virtual uint64_t GetTransactionId() = 0;
    virtual int32_t GetPresentationPosition(uint64_t& frames, int64_t& timeSec, int64_t& timeNanoSec) = 0;

    virtual void RegisterWakeupCloseCallback(IAudioSourceCallback *callback) = 0;
    virtual void RegisterAudioCapturerSourceCallback(IAudioSourceCallback *callback) = 0;
    virtual void RegisterParameterCallback(IAudioSourceCallback *callback) = 0;

    virtual int32_t Preload(const std::string &usbInfoStr)
    {
        return 0;
    }
};

class IMmapAudioCapturerSource : public IAudioCapturerSource {
public:
    IMmapAudioCapturerSource() = default;
    virtual ~IMmapAudioCapturerSource() = default;
    virtual int32_t GetMmapBufferInfo(int &fd, uint32_t &totalSizeInframe, uint32_t &spanSizeInframe,
        uint32_t &byteSizePerFrame) = 0;
    virtual int32_t GetMmapHandlePosition(uint64_t &frames, int64_t &timeSec, int64_t &timeNanoSec) = 0;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // AUDIO_CAPTURER_SOURCE_H
