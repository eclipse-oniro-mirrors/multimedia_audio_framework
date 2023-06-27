/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <iostream>

#include "audio_errors.h"
#include "audio_info.h"
#include "audio_system_manager.h"
#include "audio_routing_manager.h"
#include "audio_stream_manager.h"
#include "audio_manager_fuzzer.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
namespace {
    std::string g_networkId = "LocalDevice";
}
void AudioRendererStateCallbackFuzz::OnRendererStateChange(
    const std::vector<std::unique_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) {}

void AudioCapturerStateCallbackFuzz::OnCapturerStateChange(
    const std::vector<std::unique_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos) {}
const int32_t LIMITSIZE = 4;
void AudioManagerFuzzAudioFrameworkTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }

    AudioVolumeType type = *reinterpret_cast<const AudioVolumeType *>(data);
    int32_t volume = *reinterpret_cast<const int32_t *>(data);
    AudioSystemManager::GetInstance()->SetVolume(type, volume);
    AudioSystemManager::GetInstance()->GetVolume(type);
    AudioSystemManager::GetInstance()->GetMinVolume(type);
    AudioSystemManager::GetInstance()->GetMaxVolume(type);
    AudioSystemManager::GetInstance()->SetMute(type, true);
    AudioSystemManager::GetInstance()->IsStreamMute(type);
    AudioSystemManager::GetInstance()->SetRingerMode(*reinterpret_cast<const AudioRingerMode *>(data));
    AudioSystemManager::GetInstance()->SetAudioScene(*reinterpret_cast<const AudioScene *>(data));

    std::string key(reinterpret_cast<const char*>(data), size);
    std::string value(reinterpret_cast<const char*>(data), size);
    AudioSystemManager::GetInstance()->SetAudioParameter(key, value);

    std::list<std::pair<AudioInterrupt, AudioFocuState>> focusInfoList = {};
    std::pair<AudioInterrupt, AudioFocuState> focusInfo = {};
    focusInfo.first.streamUsage = *reinterpret_cast<const StreamUsage *>(data);
    focusInfo.first.contentType = *reinterpret_cast<const ContentType *>(data);
    focusInfo.first.audioFocusType.streamType = *reinterpret_cast<const AudioStreamType *>(data);
    focusInfo.first.audioFocusType.sourceType = *reinterpret_cast<const SourceType *>(data);
    focusInfo.first.audioFocusType.isPlay = *reinterpret_cast<const bool *>(data);
    focusInfo.first.sessionID = *reinterpret_cast<const int32_t *>(data);
    focusInfo.first.pauseWhenDucked = *reinterpret_cast<const bool *>(data);
    focusInfo.first.pid = *reinterpret_cast<const int32_t *>(data);
    focusInfo.first.mode = *reinterpret_cast<const InterruptMode *>(data);
    focusInfo.second = *reinterpret_cast<const AudioFocuState *>(data);
    focusInfoList.push_back(focusInfo);
    AudioSystemManager::GetInstance()->GetAudioFocusInfoList(focusInfoList);

    shared_ptr<AudioFocusInfoChangeCallbackFuzz> focusInfoChangeCallbackFuzz =
        std::make_shared<AudioFocusInfoChangeCallbackFuzz>();
    AudioSystemManager::GetInstance()->RegisterFocusInfoChangeCallback(focusInfoChangeCallbackFuzz);
    AudioSystemManager::GetInstance()->UnregisterFocusInfoChangeCallback();
}

void AudioRoutingManagerFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }

    AudioRendererInfo rendererInfo = {};
    rendererInfo.contentType = *reinterpret_cast<const ContentType *>(data);
    rendererInfo.streamUsage = *reinterpret_cast<const StreamUsage *>(data);
    rendererInfo.rendererFlags = *reinterpret_cast<const int32_t *>(data);
    std::vector<sptr<AudioDeviceDescriptor>> desc;
    shared_ptr<AudioPreferOutputDeviceChangeCallbackFuzz> preferOutputCallbackFuzz =
        std::make_shared<AudioPreferOutputDeviceChangeCallbackFuzz>();
    AudioRoutingManager::GetInstance()->GetPreferOutputDeviceForRendererInfo(rendererInfo, desc);
    AudioRoutingManager::GetInstance()->SetPreferOutputDeviceChangeCallback(rendererInfo, preferOutputCallbackFuzz);
    AudioRoutingManager::GetInstance()->UnsetPreferOutputDeviceChangeCallback();
}

void AudioStreamManagerFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }

    int32_t clientPid = *reinterpret_cast<const int32_t *>(data);
    shared_ptr<AudioRendererStateCallbackFuzz> audioRendererStateCallbackFuzz =
        std::make_shared<AudioRendererStateCallbackFuzz>();
    shared_ptr<AudioCapturerStateCallbackFuzz> audioCapturerStateCallbackFuzz =
        std::make_shared<AudioCapturerStateCallbackFuzz>();
    AudioStreamManager::GetInstance()->RegisterAudioRendererEventListener(clientPid, audioRendererStateCallbackFuzz);
    AudioStreamManager::GetInstance()->UnregisterAudioRendererEventListener(clientPid);
    AudioStreamManager::GetInstance()->RegisterAudioCapturerEventListener(clientPid, audioCapturerStateCallbackFuzz);
    AudioStreamManager::GetInstance()->UnregisterAudioCapturerEventListener(clientPid);

    std::vector<std::unique_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
    AudioStreamManager::GetInstance()->GetCurrentRendererChangeInfos(audioRendererChangeInfos);

    std::vector<std::unique_ptr<AudioCapturerChangeInfo>> audioCapturerChangeInfos;
    AudioStreamManager::GetInstance()->GetCurrentCapturerChangeInfos(audioCapturerChangeInfos);
}

void AudioGroupManagerFuzzTest(const uint8_t* data, size_t size)
{
    if ((data == nullptr) || (size < LIMITSIZE)) {
        return;
    }

    int32_t volume = *reinterpret_cast<const int32_t *>(data);
    AudioVolumeType type = *reinterpret_cast<const AudioVolumeType *>(data);
    VolumeAdjustType adjustType = *reinterpret_cast<const VolumeAdjustType *>(data);
    DeviceType device = *reinterpret_cast<const DeviceType *>(data);
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(g_networkId, infos);
    int32_t groupId = infos[0]->volumeGroupId_;
    auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
    audioGroupMngr_->IsVolumeUnadjustable();
    audioGroupMngr_->AdjustVolumeByStep(adjustType);
    audioGroupMngr_->AdjustSystemVolumeByStep(type, adjustType);
    audioGroupMngr_->GetSystemVolumeInDb(type, volume, device);
}
} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioStandard::AudioManagerFuzzAudioFrameworkTest(data, size);
    OHOS::AudioStandard::AudioRoutingManagerFuzzTest(data, size);
    OHOS::AudioStandard::AudioStreamManagerFuzzTest(data, size);
    OHOS::AudioStandard::AudioGroupManagerFuzzTest(data, size);
    return 0;
}
