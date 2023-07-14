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

#ifndef I_POLICY_PROVIDER_H
#define I_POLICY_PROVIDER_H

#include <memory>
#include <vector>

#include "audio_info.h"
#include "audio_shared_memory.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static const std::vector<std::pair<AudioStreamType, DeviceType>> g_volumeIndexVector = {
        {STREAM_MUSIC, DEVICE_TYPE_SPEAKER},
        {STREAM_MUSIC, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_MUSIC, DEVICE_TYPE_USB_HEADSET},
        {STREAM_MUSIC, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_RING, DEVICE_TYPE_SPEAKER},
        {STREAM_RING, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_RING, DEVICE_TYPE_USB_HEADSET},
        {STREAM_RING, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_SPEAKER},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_USB_HEADSET},
        {STREAM_VOICE_ASSISTANT, DEVICE_TYPE_WIRED_HEADPHONES},
        {STREAM_SYSTEM, DEVICE_TYPE_SPEAKER},
        {STREAM_SYSTEM, DEVICE_TYPE_WIRED_HEADSET},
        {STREAM_SYSTEM, DEVICE_TYPE_USB_HEADSET},
        {STREAM_SYSTEM, DEVICE_TYPE_WIRED_HEADPHONES}};
}
class IPolicyProvider {
public:
    virtual int32_t GetProcessDeviceInfo(const AudioProcessConfig &config, DeviceInfo &deviceInfo) = 0;

    virtual int32_t InitSharedVolume(std::shared_ptr<AudioSharedMemory> &buffer) = 0;

    // virtual int32_t GetVolume(AudioStreamType streamType, DeviceType deviceType, Volume &vol) = 0;

    // virtual int32_t SetVolume(AudioStreamType streamType, DeviceType deviceType, Volume vol) = 0;

    virtual ~IPolicyProvider() = default;

    static bool GetVolumeIndex(AudioStreamType streamType, DeviceType deviceType, size_t &index)
    {
        bool isFind = false;
        for (size_t tempIndex = 0; tempIndex < g_volumeIndexVector.size(); tempIndex++) {
            if (g_volumeIndexVector[tempIndex].first == streamType &&
                g_volumeIndexVector[tempIndex].second == deviceType) {
                isFind = true;
                index = tempIndex;
                break;
            }
        }
        return isFind;
    };
    static size_t GetVolumeVectorSize()
    {
        return g_volumeIndexVector.size();
    };
};
} // namespace AudioStandard
} // namespace OHOS
#endif // I_POLICY_PROVIDER_H