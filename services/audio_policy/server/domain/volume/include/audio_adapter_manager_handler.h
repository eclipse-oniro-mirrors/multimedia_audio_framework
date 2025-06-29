/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef AUDIO_ADAPTER_MANAGER_HANDLER_H
#define AUDIO_ADAPTER_MANAGER_HANDLER_H

#include <mutex>
#include "singleton.h"
#include "event_handler.h"
#include "event_runner.h"

#include "audio_policy_log.h"
#include "audio_system_manager.h"
#include "audio_policy_client.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

class AudioAdapterManagerHandler : public AppExecFwk::EventHandler {
public:
    AudioAdapterManagerHandler();
    ~AudioAdapterManagerHandler();

    void ReleaseEventRunner();

    enum EventAdapterManagerServerCmd  {
        DATABASE_UPDATE,
        VOLUME_DATABASE_SAVE,
        STREAM_MUTE_STATUS_UPDATE,
        RINGER_MODE_UPDATE,
    };

    struct VolumeDataEvent {
        VolumeDataEvent() = delete;
        VolumeDataEvent(const DeviceType &deviceType, const AudioStreamType &streamType, const int32_t &volumeLevel,
            const std::string &networkId)
            : deviceType_(deviceType), streamType_(streamType), volumeLevel_(volumeLevel), networkId_(networkId)
        {}
        DeviceType deviceType_;
        AudioStreamType streamType_;
        int32_t volumeLevel_;
        std::string networkId_;
    };

    struct StreamMuteStatusEvent {
        StreamMuteStatusEvent() = delete;
        StreamMuteStatusEvent(const AudioStreamType &streamType, const bool &mute, const StreamUsage &streamUsage,
            const DeviceType &deviceType)
            : streamType_(streamType), mute_(mute), streamUsage_(streamUsage), deviceType_(deviceType)
        {}
        AudioStreamType streamType_;
        bool mute_;
        StreamUsage streamUsage_;
        DeviceType deviceType_;
    };

    struct RingerModeEvent {
        RingerModeEvent() = delete;
        RingerModeEvent(const AudioRingerMode &ringerMode)
            : ringerMode_(ringerMode)
        {}
        AudioRingerMode ringerMode_;
    };

    bool SendKvDataUpdate(const bool &isFirstBoot);
    bool SendSaveVolume(const DeviceType &deviceType, const AudioStreamType &streamType, const int32_t &volumeLevel,
        std::string networkId = "LocalDevice");
    bool SendStreamMuteStatusUpdate(const AudioStreamType &streamType, const bool &mute,
        const StreamUsage &streamUsage, const DeviceType &deviceType = DEVICE_TYPE_NONE);
    bool SendRingerModeUpdate(const AudioRingerMode &ringerMode);

protected:
    void ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event) override;

private:
    /* Handle Event*/
    void HandleUpdateKvDataEvent(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleVolumeDataBaseSave(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleUpdateStreamMuteStatus(const AppExecFwk::InnerEvent::Pointer &event);
    void HandleUpdateRingerMode(const AppExecFwk::InnerEvent::Pointer &event);

    std::mutex runnerMutex_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_ADAPTER_MANAGER_HANDLER_H
