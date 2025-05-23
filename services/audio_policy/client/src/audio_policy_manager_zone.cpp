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
#ifndef LOG_TAG
#define LOG_TAG "AudioPolicyManagerZone"
#endif

#include "audio_policy_manager.h"
#include "audio_policy_proxy.h"
#include "audio_errors.h"
#include "audio_server_death_recipient.h"
#include "audio_policy_log.h"
#include "audio_utils.h"
#include "audio_zone_client.h"

namespace OHOS {
namespace AudioStandard {
int32_t AudioPolicyManager::RegisterAudioZoneClient(const sptr<IRemoteObject> &object)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->RegisterAudioZoneClient(object);
}

int32_t AudioPolicyManager::CreateAudioZone(const std::string &name, const AudioZoneContext &context)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->CreateAudioZone(name, context);
}

void AudioPolicyManager::ReleaseAudioZone(int32_t zoneId)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_LOG(gsp != nullptr, "audio policy manager proxy is NULL.");

    gsp->ReleaseAudioZone(zoneId);
}

const std::vector<std::shared_ptr<AudioZoneDescriptor>> AudioPolicyManager::GetAllAudioZone()
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    std::vector<std::shared_ptr<AudioZoneDescriptor>> zoneDescriptors;
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, zoneDescriptors, "audio policy manager proxy is NULL.");

    return gsp->GetAllAudioZone();
}

const std::shared_ptr<AudioZoneDescriptor> AudioPolicyManager::GetAudioZone(int32_t zoneId)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, nullptr, "audio policy manager proxy is NULL.");

    return gsp->GetAudioZone(zoneId);
}

int32_t AudioPolicyManager::BindDeviceToAudioZone(int32_t zoneId,
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->BindDeviceToAudioZone(zoneId, devices);
}

int32_t AudioPolicyManager::UnBindDeviceToAudioZone(int32_t zoneId,
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->UnBindDeviceToAudioZone(zoneId, devices);
}

int32_t AudioPolicyManager::EnableAudioZoneReport(bool enable)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->EnableAudioZoneReport(enable);
}

int32_t AudioPolicyManager::EnableAudioZoneChangeReport(int32_t zoneId, bool enable)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->EnableAudioZoneChangeReport(zoneId, enable);
}

int32_t AudioPolicyManager::AddUidToAudioZone(int32_t zoneId, int32_t uid)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->AddUidToAudioZone(zoneId, uid);
}

int32_t AudioPolicyManager::RemoveUidFromAudioZone(int32_t zoneId, int32_t uid)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->RemoveUidFromAudioZone(zoneId, uid);
}

int32_t AudioPolicyManager::EnableSystemVolumeProxy(int32_t zoneId, bool enable)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->EnableSystemVolumeProxy(zoneId, enable);
}

std::list<std::pair<AudioInterrupt, AudioFocuState>> AudioPolicyManager::GetAudioInterruptForZone(int32_t zoneId)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, {}, "audio policy manager proxy is NULL.");

    return gsp->GetAudioInterruptForZone(zoneId);
}

std::list<std::pair<AudioInterrupt, AudioFocuState>> AudioPolicyManager::GetAudioInterruptForZone(
    int32_t zoneId, const std::string &deviceTag)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, {}, "audio policy manager proxy is NULL.");

    return gsp->GetAudioInterruptForZone(zoneId, deviceTag);
}

int32_t AudioPolicyManager::EnableAudioZoneInterruptReport(int32_t zoneId, const std::string &deviceTag, bool enable)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->EnableAudioZoneInterruptReport(zoneId, deviceTag, enable);
}

int32_t AudioPolicyManager::InjectInterruptToAudioZone(int32_t zoneId,
    const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->InjectInterruptToAudioZone(zoneId, interrupts);
}

int32_t AudioPolicyManager::InjectInterruptToAudioZone(int32_t zoneId, const std::string &deviceTag,
    const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts)
{
    const sptr<IAudioPolicy> gsp = GetAudioPolicyManagerProxy();
    CHECK_AND_RETURN_RET_LOG(gsp != nullptr, ERROR, "audio policy manager proxy is NULL.");

    return gsp->InjectInterruptToAudioZone(zoneId, deviceTag, interrupts);
}
} // namespace AudioStandard
} // namespace OHOS
