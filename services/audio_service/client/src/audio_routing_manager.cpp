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
#ifndef LOG_TAG
#define LOG_TAG "AudioRoutingManager"
#endif

#include "audio_errors.h"
#include "audio_manager_proxy.h"
#include "audio_policy_manager.h"
#include "audio_common_log.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_routing_manager.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;
AudioRoutingManager *AudioRoutingManager::GetInstance()
{
    static AudioRoutingManager audioRoutingManager;
    return &audioRoutingManager;
}

int32_t AudioRoutingManager::GetCallingPid()
{
    return getpid();
}

int32_t AudioRoutingManager::SetMicStateChangeCallback(
    const std::shared_ptr<AudioManagerMicStateChangeCallback> &callback)
{
    AudioSystemManager* audioSystemManager = AudioSystemManager::GetInstance();
    std::shared_ptr<AudioGroupManager> groupManager = audioSystemManager->GetGroupManager(DEFAULT_VOLUME_GROUP_ID);
    CHECK_AND_RETURN_RET_LOG(groupManager != nullptr, ERR_INVALID_PARAM,
        "setMicrophoneMuteCallback falied, groupManager is null");
    return groupManager->SetMicStateChangeCallback(callback);
}

int32_t AudioRoutingManager::GetPreferredOutputDeviceForRendererInfo(AudioRendererInfo rendererInfo,
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc)
{
    desc = AudioPolicyManager::GetInstance().GetPreferredOutputDeviceDescriptors(rendererInfo);

    return SUCCESS;
}

int32_t AudioRoutingManager::GetPreferredInputDeviceForCapturerInfo(AudioCapturerInfo captureInfo,
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> &desc)
{
    desc = AudioPolicyManager::GetInstance().GetPreferredInputDeviceDescriptors(captureInfo);

    return SUCCESS;
}

int32_t AudioRoutingManager::SetPreferredOutputDeviceChangeCallback(AudioRendererInfo rendererInfo,
    const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback>& callback)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "callback is nullptr");

    return AudioPolicyManager::GetInstance().SetPreferredOutputDeviceChangeCallback(rendererInfo, callback);
}

int32_t AudioRoutingManager::SetPreferredInputDeviceChangeCallback(AudioCapturerInfo capturerInfo,
    const std::shared_ptr<AudioPreferredInputDeviceChangeCallback> &callback)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "callback is nullptr");

    return AudioPolicyManager::GetInstance().SetPreferredInputDeviceChangeCallback(capturerInfo, callback);
}

int32_t AudioRoutingManager::UnsetPreferredOutputDeviceChangeCallback(
    const std::shared_ptr<AudioPreferredOutputDeviceChangeCallback> &callback)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    return AudioPolicyManager::GetInstance().UnsetPreferredOutputDeviceChangeCallback(callback);
}

int32_t AudioRoutingManager::UnsetPreferredInputDeviceChangeCallback(
    const std::shared_ptr<AudioPreferredInputDeviceChangeCallback> &callback)
{
    AUDIO_INFO_LOG("Entered %{public}s", __func__);
    return AudioPolicyManager::GetInstance().UnsetPreferredInputDeviceChangeCallback(callback);
}

vector<sptr<MicrophoneDescriptor>> AudioRoutingManager::GetAvailableMicrophones()
{
    return AudioPolicyManager::GetInstance().GetAvailableMicrophones();
}

std::vector<std::shared_ptr<AudioDeviceDescriptor>> AudioRoutingManager::GetAvailableDevices(AudioDeviceUsage usage)
{
    return AudioPolicyManager::GetInstance().GetAvailableDevices(usage);
}

std::shared_ptr<AudioDeviceDescriptor> AudioRoutingManager::GetActiveBluetoothDevice()
{
    return AudioPolicyManager::GetInstance().GetActiveBluetoothDevice();
}

int32_t AudioRoutingManager::SetAudioDeviceRefinerCallback(const std::shared_ptr<AudioDeviceRefiner> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, ERR_INVALID_PARAM, "callback is nullptr");

    return AudioPolicyManager::GetInstance().SetAudioDeviceRefinerCallback(callback);
}

int32_t AudioRoutingManager::UnsetAudioDeviceRefinerCallback()
{
    return AudioPolicyManager::GetInstance().UnsetAudioDeviceRefinerCallback();
}

int32_t AudioRoutingManager::TriggerFetchDevice(AudioStreamDeviceChangeReasonExt reason)
{
    return AudioPolicyManager::GetInstance().TriggerFetchDevice(reason);
}

int32_t AudioRoutingManager::SetPreferredDevice(const PreferredType preferredType,
    const std::shared_ptr<AudioDeviceDescriptor> &desc, const int32_t uid)
{
    return AudioPolicyManager::GetInstance().SetPreferredDevice(preferredType, desc, uid);
}

void AudioRoutingManager::SaveRemoteInfo(const std::string &networkId, DeviceType deviceType)
{
    AudioPolicyManager::GetInstance().SaveRemoteInfo(networkId, deviceType);
}

int32_t AudioRoutingManager::SetDeviceConnectionStatus(const std::shared_ptr<AudioDeviceDescriptor> &desc,
    const bool isConnected)
{
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, ERR_INVALID_PARAM, "desc is nullptr");
    return AudioPolicyManager::GetInstance().SetDeviceConnectionStatus(desc, isConnected);
}
} // namespace AudioStandard
} // namespace OHOS
