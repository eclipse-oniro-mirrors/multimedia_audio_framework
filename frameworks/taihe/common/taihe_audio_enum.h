/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef TAIHE_AUDIO_ENUM_H
#define TAIHE_AUDIO_ENUM_H

#include "ohos.multimedia.audio.proj.hpp"
#include "ohos.multimedia.audio.impl.hpp"
#include "taihe/runtime.hpp"
#include "audio_log.h"
#include "audio_effect.h"

namespace ANI::Audio {
using namespace taihe;
using namespace ohos::multimedia::audio;
class TaiheAudioEnum {
public:
    enum CapturerType {
        TYPE_INVALID = -1,
        TYPE_MIC = 0,
        TYPE_VOICE_RECOGNITION = 1,
        TYPE_WAKEUP = 3,
        TYPE_VOICE_CALL = 4,
        TYPE_PLAYBACK_CAPTURE = 2,
        TYPE_COMMUNICATION = 7,
        TYPE_MESSAGE = 10,
        TYPE_REMOTE_CAST = 11,
        TYPE_VOICE_TRANSCRIPTION = 12,
        TYPE_CAMCORDER = 13,
        TYPE_UNPROCESSED = 14,
        TYPE_LIVE = 17
    };

    enum AudioJsVolumeType {
        VOLUMETYPE_DEFAULT = -1,
        VOICE_CALL = 0,
        RINGTONE = 2,
        MEDIA = 3,
        ALARM = 4,
        ACCESSIBILITY = 5,
        SYSTEM = 6,
        VOICE_ASSISTANT = 9,
        ULTRASONIC = 10,
        NOTIFICATION = 11,
        NAVIGATION = 12,
        VOLUMETYPE_MAX,
        ALL = 100
    };

    enum AudioRingMode {
        RINGER_MODE_SILENT = 0,
        RINGER_MODE_VIBRATE,
        RINGER_MODE_NORMAL
    };

    enum InterruptMode {
        SHARE_MODE = 0,
        INDEPENDENT_MODE = 1
    };

    enum AudioDataCallbackResult {
        CALLBACK_RESULT_INVALID = -1,
        CALLBACK_RESULT_VALID = 0,
    };

    static bool IsLegalInputArgumentInterruptMode(int32_t interruptMode);
    static bool IsLegalInputArgumentAudioEffectMode(int32_t audioEffectMode);
    static bool IsLegalInputArgumentChannelBlendMode(int32_t blendMode);
    static bool IsLegalCapturerType(int32_t type);
    static bool IsLegalInputArgumentVolType(int32_t inputType);
    static bool IsLegalInputArgumentRingMode(int32_t ringMode);
    static OHOS::AudioStandard::AudioVolumeType GetNativeAudioVolumeType(int32_t volumeType);
    static OHOS::AudioStandard::AudioRingerMode GetNativeAudioRingerMode(int32_t ringMode);
    static OHOS::AudioStandard::InterruptMode GetNativeInterruptMode(int32_t interruptMode);
    static AudioVolumeType GetJsAudioVolumeType(OHOS::AudioStandard::AudioStreamType volumeType);
    static AudioVolumeType GetJsAudioVolumeTypeMore(OHOS::AudioStandard::AudioStreamType volumeType);
    static bool IsLegalInputArgumentDeviceFlag(int32_t deviceFlag);
    static bool IsLegalInputArgumentActiveDeviceType(int32_t activeDeviceFlag);
    static bool IsLegalInputArgumentCommunicationDeviceType(int32_t communicationDeviceType);
    static bool IsLegalInputArgumentDeviceType(int32_t deviceType);
    static bool IsLegalInputArgumentDefaultOutputDeviceType(int32_t deviceType);
    static bool IsLegalInputArgumentVolumeAdjustType(int32_t adjustType);
    static bool IsLegalInputArgumentVolumeMode(int32_t volumeMode);
    static bool IsLegalInputArgumentStreamUsage(int32_t streamUsage);
    static bool IsValidSourceType(int32_t intValue);
    static bool IsLegalDeviceUsage(int32_t usage);
    static bool IsLegalInputArgumentSpatializationSceneType(int32_t spatializationSceneType);

    static ConnectType ToTaiheConnectType(OHOS::AudioStandard::ConnectType type);
    static DeviceRole ToTaiheDeviceRole(OHOS::AudioStandard::DeviceRole type);
    static DeviceType ToTaiheDeviceType(OHOS::AudioStandard::DeviceType type);
    static AudioEncodingType ToTaiheAudioEncodingType(OHOS::AudioStandard::AudioEncodingType type);
    static AudioState ToTaiheAudioState(OHOS::AudioStandard::RendererState state);
    static AudioState ToTaiheAudioState(OHOS::AudioStandard::CapturerState state);
    static StreamUsage ToTaiheStreamUsage(OHOS::AudioStandard::StreamUsage usage);
    static SourceType ToTaiheSourceType(OHOS::AudioStandard::SourceType type);
    static EffectFlag ToTaiheEffectFlag(OHOS::AudioStandard::EffectFlag flag);
    static AudioScene ToTaiheAudioScene(OHOS::AudioStandard::AudioScene scene);
    static InterruptType ToTaiheInterruptType(OHOS::AudioStandard::InterruptType type);
    static InterruptHint ToTaiheInterruptHint(OHOS::AudioStandard::InterruptHint hint);
    static AudioVolumeMode ToTaiheAudioVolumeMode(OHOS::AudioStandard::AudioVolumeMode mode);
    static DeviceChangeType ToTaiheDeviceChangeType(OHOS::AudioStandard::DeviceChangeType type);
    static AudioSessionDeactivatedReason ToTaiheSessionDeactiveReason(
        OHOS::AudioStandard::AudioSessionDeactiveReason reason);
    static ohos::multimedia::audio::AsrNoiseSuppressionMode ToTaiheAsrNoiseSuppressionMode(
        ::AsrNoiseSuppressionMode mode);
    static ohos::multimedia::audio::AsrAecMode ToTaiheAsrAecMode(::AsrAecMode mode);
    static ohos::multimedia::audio::AsrWhisperDetectionMode ToTaiheAsrWhisperDetectionMode(
        ::AsrWhisperDetectionMode mode);
    static InterruptForceType ToTaiheInterruptForceType(OHOS::AudioStandard::InterruptForceType type);
    static AudioSpatializationSceneType ToTaiheAudioSpatializationSceneType(
        OHOS::AudioStandard::AudioSpatializationSceneType type);
    static AudioVolumeType ToTaiheAudioVolumeType(TaiheAudioEnum::AudioJsVolumeType type);
    static ohos::multimedia::audio::AudioRingMode ToTaiheAudioRingMode(OHOS::AudioStandard::AudioRingerMode mode);
    static AudioEffectMode ToTaiheAudioEffectMode(OHOS::AudioStandard::AudioEffectMode mode);
    static AudioStreamDeviceChangeReason ToTaiheAudioStreamDeviceChangeReason(
        OHOS::AudioStandard::AudioStreamDeviceChangeReason reason);
    static AudioChannelLayout ToTaiheAudioChannelLayout(OHOS::AudioStandard::AudioChannelLayout layout);
    static DeviceBlockStatus ToTaiheDeviceBlockStatus(OHOS::AudioStandard::DeviceBlockStatus status);

    static const std::map<std::string, int32_t> deviceTypeMap;
};
} // namespace ANI::Audio

#endif // TAIHE_AUDIO_ENUM_H
