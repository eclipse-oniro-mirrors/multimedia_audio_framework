/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "audio_errors.h"
#include "audio_manager_proxy.h"
#include "audio_policy_manager.h"
#include "audio_log.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "audio_utils.h"

#include "audio_group_manager.h"

namespace OHOS {
namespace AudioStandard {
static sptr<IStandardAudioService> g_sProxy = nullptr;
AudioGroupManager::AudioGroupManager(int32_t groupId) : groupId_(groupId)
{
}
AudioGroupManager::~AudioGroupManager()
{
    AUDIO_DEBUG_LOG("AudioGroupManager start");
    if (cbClientId_ != -1) {
        UnsetRingerModeCallback(cbClientId_);
    }
}

int32_t AudioGroupManager::SetVolume(AudioVolumeType volumeType, int32_t volume)
{
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string condition = "EVENT_TYPE=1;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE="
            + std::to_string(volumeType) + ";";
        std::string value = std::to_string(volume);
        g_sProxy->SetAudioParameter(netWorkId_, AudioParamKey::VOLUME, condition, value);
        return SUCCESS;
    }

    AUDIO_DEBUG_LOG("AudioSystemManager SetVolume volumeType=%{public}d ", volumeType);

    /* Validate and return INVALID_PARAMS error */
    if ((volume < MIN_VOLUME_LEVEL) || (volume > MAX_VOLUME_LEVEL)) {
        AUDIO_ERR_LOG("Invalid Volume Input!");
        return ERR_INVALID_PARAM;
    }

    switch (volumeType) {
        case STREAM_MUSIC:
        case STREAM_RING:
        case STREAM_NOTIFICATION:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
        case STREAM_ALARM:
        case STREAM_ACCESSIBILITY:
        case STREAM_ULTRASONIC:
        case STREAM_ALL:
            break;
        default:
            AUDIO_ERR_LOG("SetVolume volumeType=%{public}d not supported", volumeType);
            return ERR_NOT_SUPPORTED;
    }

    /* Call Audio Policy SetSystemVolumeLevel */
    AudioStreamType StreamVolType = (AudioStreamType)volumeType;
    return AudioPolicyManager::GetInstance().SetSystemVolumeLevel(StreamVolType, volume, API_9);
}

int32_t AudioGroupManager::GetVolume(AudioVolumeType volumeType)
{
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string condition = "EVENT_TYPE=1;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE="
            + std::to_string(volumeType) + ";";
        std::string value = g_sProxy->GetAudioParameter(netWorkId_, AudioParamKey::VOLUME, condition);
        if (value.empty()) {
            AUDIO_ERR_LOG("[AudioGroupManger]: invalid value %{public}s", value.c_str());
            return 0;
        }
        return std::stoi(value);
    }
    switch (volumeType) {
        case STREAM_MUSIC:
        case STREAM_RING:
        case STREAM_NOTIFICATION:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
        case STREAM_ALARM:
        case STREAM_ACCESSIBILITY:
            break;
        case STREAM_ULTRASONIC:
        case STREAM_ALL:
            if (!PermissionUtil::VerifySelfPermission()) {
                AUDIO_ERR_LOG("GetVolume: No system permission");
                return ERR_PERMISSION_DENIED;
            }
            break;
        default:
            AUDIO_ERR_LOG("GetVolume volumeType=%{public}d not supported", volumeType);
            return ERR_NOT_SUPPORTED;
    }

    AudioStreamType StreamVolType = (AudioStreamType)volumeType;
    return AudioPolicyManager::GetInstance().GetSystemVolumeLevel(StreamVolType);
}

int32_t AudioGroupManager::GetMaxVolume(AudioVolumeType volumeType)
{
    if (!IsAlived()) {
        CHECK_AND_RETURN_RET_LOG(g_sProxy != nullptr, ERR_OPERATION_FAILED, "GetMaxVolume service unavailable");
    }
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string condition = "EVENT_TYPE=3;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE=" +
            std::to_string(volumeType) + ";";
        std::string value = g_sProxy->GetAudioParameter(netWorkId_, AudioParamKey::VOLUME, condition);
        if (value.empty()) {
            AUDIO_ERR_LOG("[AudioGroupManger]: invalid value %{public}s", value.c_str());
            return 0;
        }
        return std::stoi(value);
    }

    if (volumeType == STREAM_ALL) {
        if (!PermissionUtil::VerifySelfPermission()) {
            AUDIO_ERR_LOG("GetMaxVolume: No system permission");
            return ERR_PERMISSION_DENIED;
        }
    }

    if (volumeType == STREAM_ULTRASONIC) {
        if (!PermissionUtil::VerifySelfPermission()) {
            AUDIO_ERR_LOG("GetMaxVolume: STREAM_ULTRASONIC No system permission");
            return ERR_PERMISSION_DENIED;
        }
    }
    return AudioPolicyManager::GetInstance().GetMaxVolumeLevel(volumeType);
}

int32_t AudioGroupManager::GetMinVolume(AudioVolumeType volumeType)
{
    if (!IsAlived()) {
        CHECK_AND_RETURN_RET_LOG(g_sProxy != nullptr, ERR_OPERATION_FAILED, "GetMinVolume service unavailable");
    }
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string condition = "EVENT_TYPE=2;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE" +
            std::to_string(volumeType) + ";";
        std::string value = g_sProxy->GetAudioParameter(netWorkId_, AudioParamKey::VOLUME, condition);
        if (value.empty()) {
            AUDIO_ERR_LOG("[AudioGroupManger]: invalid value %{public}s", value.c_str());
            return 0;
        }
        return std::stoi(value);
    }

    if (volumeType == STREAM_ALL) {
        if (!PermissionUtil::VerifySelfPermission()) {
            AUDIO_ERR_LOG("GetMinVolume: No system permission");
            return ERR_PERMISSION_DENIED;
        }
    }

    if (volumeType == STREAM_ULTRASONIC) {
        if (!PermissionUtil::VerifySelfPermission()) {
            AUDIO_ERR_LOG("GetMinVolume: STREAM_ULTRASONIC No system permission");
            return ERR_PERMISSION_DENIED;
        }
    }
    return AudioPolicyManager::GetInstance().GetMinVolumeLevel(volumeType);
}

int32_t AudioGroupManager::SetMute(AudioVolumeType volumeType, bool mute)
{
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string conditon = "EVENT_TYPE=4;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE="
            + std::to_string(volumeType) + ";";
        std::string value = mute ? "1" : "0";
        g_sProxy->SetAudioParameter(netWorkId_, AudioParamKey::VOLUME, conditon, value);
        return SUCCESS;
    }

    AUDIO_DEBUG_LOG("AudioSystemManager SetMute for volumeType=%{public}d", volumeType);
    switch (volumeType) {
        case STREAM_MUSIC:
        case STREAM_RING:
        case STREAM_NOTIFICATION:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
        case STREAM_ALARM:
        case STREAM_ACCESSIBILITY:
        case STREAM_ULTRASONIC:
        case STREAM_ALL:
            break;
        default:
            AUDIO_ERR_LOG("SetMute volumeType=%{public}d not supported", volumeType);
            return ERR_NOT_SUPPORTED;
    }

    /* Call Audio Policy SetStreamMute */
    AudioStreamType StreamVolType = (AudioStreamType)volumeType;
    return AudioPolicyManager::GetInstance().SetStreamMute(StreamVolType, mute, API_9);
}

int32_t AudioGroupManager::IsStreamMute(AudioVolumeType volumeType, bool &isMute)
{
    AUDIO_DEBUG_LOG("AudioSystemManager::GetMute Client");
    if (connectType_ == CONNECT_TYPE_DISTRIBUTED) {
        std::string condition = "EVENT_TYPE=4;VOLUME_GROUP_ID=" + std::to_string(groupId_) + ";AUDIO_VOLUME_TYPE="
            + std::to_string(volumeType) + ";";
        std::string ret = g_sProxy->GetAudioParameter(netWorkId_, AudioParamKey::VOLUME, condition);

        return ret == "1" ? true : false;
    }

    switch (volumeType) {
        case STREAM_MUSIC:
        case STREAM_RING:
        case STREAM_NOTIFICATION:
        case STREAM_VOICE_CALL:
        case STREAM_VOICE_ASSISTANT:
        case STREAM_ALARM:
        case STREAM_ACCESSIBILITY:
            break;
        case STREAM_ULTRASONIC:
        case STREAM_ALL:
            if (!PermissionUtil::VerifySelfPermission()) {
                AUDIO_ERR_LOG("IsStreamMute: No system permission");
                return ERR_PERMISSION_DENIED;
            }
            break;
        default:
            AUDIO_ERR_LOG("IsStreamMute volumeType=%{public}d not supported", volumeType);
            return false;
    }

    AudioStreamType StreamVolType = (AudioStreamType)volumeType;
    isMute = AudioPolicyManager::GetInstance().GetStreamMute(StreamVolType);
    return SUCCESS;
}

int32_t AudioGroupManager::Init()
{
    // init networkId_
    std::string netWorkId;
    int32_t ret = AudioPolicyManager::GetInstance().GetNetworkIdByGroupId(groupId_, netWorkId);
    if (ret == SUCCESS) {
        netWorkId_ = netWorkId;
        connectType_ = netWorkId_ == LOCAL_NETWORK_ID ? CONNECT_TYPE_LOCAL : CONNECT_TYPE_DISTRIBUTED;
        AUDIO_INFO_LOG("AudioGroupManager::init set networkId %{public}s.", netWorkId_.c_str());
    } else {
        AUDIO_ERR_LOG("AudioGroupManager::init failed, has no valid group");
        return ERROR;
    }

    // init g_sProxy
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        AUDIO_ERR_LOG("AudioSystemManager::init failed");
        return ERROR;
    }

    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    if (object == nullptr) {
        AUDIO_DEBUG_LOG("AudioSystemManager::object is NULL.");
        return ERROR;
    }
    g_sProxy = iface_cast<IStandardAudioService>(object);
    if (g_sProxy == nullptr) {
        AUDIO_DEBUG_LOG("AudioSystemManager::init g_sProxy is NULL.");
        return ERROR;
    } else {
        AUDIO_DEBUG_LOG("AudioSystemManager::init g_sProxy is assigned.");
        return SUCCESS;
    }
}

bool AudioGroupManager::IsAlived()
{
    if (g_sProxy == nullptr) {
        Init();
    }

    return (g_sProxy != nullptr) ? true : false;
}

int32_t AudioGroupManager::GetGroupId()
{
    return groupId_;
}

int32_t AudioGroupManager::SetRingerModeCallback(const int32_t clientId,
    const std::shared_ptr<AudioRingerModeCallback> &callback)
{
    if (callback == nullptr) {
        AUDIO_ERR_LOG("AudioSystemManager: callback is nullptr");
        return ERR_INVALID_PARAM;
    }

    cbClientId_ = clientId;

    return AudioPolicyManager::GetInstance().SetRingerModeCallback(clientId, callback, API_9);
}

int32_t AudioGroupManager::UnsetRingerModeCallback(const int32_t clientId) const
{
    return AudioPolicyManager::GetInstance().UnsetRingerModeCallback(clientId);
}

int32_t AudioGroupManager::SetRingerMode(AudioRingerMode ringMode) const
{
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::SetRingerMode is not supported for local device.");
        return ERROR;
    }
    /* Call Audio Policy SetRingerMode */
    return AudioPolicyManager::GetInstance().SetRingerMode(ringMode, API_9);
}

AudioRingerMode AudioGroupManager::GetRingerMode() const
{
    /* Call Audio Policy GetRingerMode */
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::SetRingerMode is not supported for local device.");
        return AudioRingerMode::RINGER_MODE_NORMAL;
    }
    return (AudioPolicyManager::GetInstance().GetRingerMode());
}

int32_t AudioGroupManager::SetMicrophoneMute(bool isMute)
{
    /* Call Audio Policy GetRingerMode */
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::SetRingerMode is not supported for local device.");
        return ERROR;
    }
    return AudioPolicyManager::GetInstance().SetMicrophoneMuteAudioConfig(isMute);
}

bool AudioGroupManager::IsMicrophoneMute(API_VERSION api_v)
{
    /* Call Audio Policy GetRingerMode */
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::SetRingerMode is not supported for local device.");
        return false;
    }
    return AudioPolicyManager::GetInstance().IsMicrophoneMute(api_v);
}

int32_t AudioGroupManager::SetMicStateChangeCallback(
    const std::shared_ptr<AudioManagerMicStateChangeCallback> &callback)
{
    AUDIO_INFO_LOG("Entered AudioRoutingManager::%{public}s", __func__);
    if (callback == nullptr) {
        AUDIO_ERR_LOG("setMicrophoneMuteCallback::callback is null");
        return ERR_INVALID_PARAM;
    }
    int32_t clientId = static_cast<int32_t>(getpid());
    return AudioPolicyManager::GetInstance().SetMicStateChangeCallback(clientId, callback);
}

bool AudioGroupManager::IsVolumeUnadjustable()
{
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::IsVolumeUnadjustable is only supported for LOCAL_NETWORK_ID.");
        return ERROR;
    }
    return AudioPolicyManager::GetInstance().IsVolumeUnadjustable();
}

int32_t AudioGroupManager::AdjustVolumeByStep(VolumeAdjustType adjustType)
{
    return AudioPolicyManager::GetInstance().AdjustVolumeByStep(adjustType);
}

int32_t AudioGroupManager::AdjustSystemVolumeByStep(AudioVolumeType volumeType, VolumeAdjustType adjustType)
{
    return AudioPolicyManager::GetInstance().AdjustSystemVolumeByStep(volumeType, adjustType);
}

float AudioGroupManager::GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType)
{
    /* Call Audio Policy GetSystemVolumeInDb */
    if (netWorkId_ != LOCAL_NETWORK_ID) {
        AUDIO_ERR_LOG("AudioGroupManager::GetSystemVolumeInDb is only supported for LOCAL_NETWORK_ID.");
        return ERROR;
    }
    return AudioPolicyManager::GetInstance().GetSystemVolumeInDb(volumeType, volumeLevel, deviceType);
}
} // namespace AudioStandard
} // namespace OHOS
