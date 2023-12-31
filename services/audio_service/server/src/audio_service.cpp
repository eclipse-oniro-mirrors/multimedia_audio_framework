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

#include "audio_service.h"

#include <thread>

#include "audio_errors.h"
#include "audio_log.h"
#include "remote_audio_renderer_sink.h"
#include "policy_handler.h"

namespace OHOS {
namespace AudioStandard {
AudioService *AudioService::GetInstance()
{
    static AudioService AudioService;

    return &AudioService;
}

AudioService::AudioService()
{
    AUDIO_INFO_LOG("AudioService()");
}

AudioService::~AudioService()
{
    AUDIO_INFO_LOG("~AudioService()");
}

int32_t AudioService::OnProcessRelease(IAudioProcessStream *process)
{
    std::lock_guard<std::mutex> lock(processListMutex_);
    bool isFind = false;
    int32_t ret = ERROR;
    auto paired = linkedPairedList_.begin();
    std::string endpointName;
    bool needRelease = false;
    while (paired != linkedPairedList_.end()) {
        if ((*paired).first == process) {
            ret = UnlinkProcessToEndpoint((*paired).first, (*paired).second);
            linkedPairedList_.erase(paired);
            isFind = true;
            if ((*paired).second->GetStatus() == AudioEndpoint::EndpointStatus::UNLINKED) {
                needRelease = true;
                endpointName = (*paired).second->GetEndpointName();
            }
            break;
        } else {
            paired++;
        }
    }
    if (isFind) {
        AUDIO_INFO_LOG("OnProcessRelease find and release process result %{public}d", ret);
    } else {
        AUDIO_WARNING_LOG("OnProcessRelease can not find target process");
    }

    if (needRelease) {
        AUDIO_INFO_LOG("OnProcessRelease find endpoint unlink, call delay release.");
        int32_t delayTime = 10000;
        std::thread releaseEndpointThread(&AudioService::DelayCallReleaseEndpoint, this, endpointName, delayTime);
        releaseEndpointThread.detach();
    }

    return SUCCESS;
}

sptr<AudioProcessInServer> AudioService::GetAudioProcess(const AudioProcessConfig &config)
{
    AUDIO_INFO_LOG("GetAudioProcess dump %{public}s", ProcessConfig::DumpProcessConfig(config).c_str());

    DeviceInfo deviceInfo = GetDeviceInfoForProcess(config);
    std::shared_ptr<AudioEndpoint> audioEndpoint = GetAudioEndpointForDevice(deviceInfo);
    CHECK_AND_RETURN_RET_LOG(audioEndpoint != nullptr, nullptr, "no endpoint found for the process");

    uint32_t totalSizeInframe = 0;
    uint32_t spanSizeInframe = 0;
    audioEndpoint->GetPreferBufferInfo(totalSizeInframe, spanSizeInframe);

    std::lock_guard<std::mutex> lock(processListMutex_);
    sptr<AudioProcessInServer> process = AudioProcessInServer::Create(config, this);
    CHECK_AND_RETURN_RET_LOG(process != nullptr, nullptr, "AudioProcessInServer create failed.");

    int32_t ret = process->ConfigProcessBuffer(totalSizeInframe, spanSizeInframe);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "ConfigProcessBuffer failed");

    ret = LinkProcessToEndpoint(process, audioEndpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, nullptr, "LinkProcessToEndpoint failed");

    linkedPairedList_.push_back(std::make_pair(process, audioEndpoint));
    return process;
}

int32_t AudioService::LinkProcessToEndpoint(sptr<AudioProcessInServer> process,
    std::shared_ptr<AudioEndpoint> endpoint)
{
    int32_t ret = endpoint->LinkProcessStream(process);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "LinkProcessStream failed");

    ret = process->AddProcessStatusListener(endpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "AddProcessStatusListener failed");

    releaseEndpointCV_.notify_all();
    return SUCCESS;
}

int32_t AudioService::UnlinkProcessToEndpoint(sptr<AudioProcessInServer> process,
    std::shared_ptr<AudioEndpoint> endpoint)
{
    int32_t ret = endpoint->UnlinkProcessStream(process);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "UnlinkProcessStream failed");

    ret = process->RemoveProcessStatusListener(endpoint);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "RemoveProcessStatusListener failed");

    return SUCCESS;
}

void AudioService::DelayCallReleaseEndpoint(std::string endpointName, int32_t delayInMs)
{
    AUDIO_INFO_LOG("Delay release endpoint [%{public}s] start.", endpointName.c_str());
    if (!endpointList_.count(endpointName)) {
        AUDIO_ERR_LOG("Find no such endpoint: %{public}s", endpointName.c_str());
        return;
    }
    std::unique_lock<std::mutex> lock(releaseEndpointMutex_);
    releaseEndpointCV_.wait_for(lock, std::chrono::milliseconds(delayInMs), [this, endpointName] {
        if (endpointName == reusingEndpoint_) {
            AUDIO_INFO_LOG("Delay release endpoint break when reuse: %{public}s", endpointName.c_str());
            return true;
        }
        AUDIO_INFO_LOG("Wake up but keep release endpoint %{public}s in delay", endpointName.c_str());
        return false;
    });
    // todo: add break operation
    std::shared_ptr<AudioEndpoint> temp = nullptr;
    if (endpointList_.find(endpointName) == endpointList_.end() || endpointList_[endpointName] == nullptr) {
        AUDIO_ERR_LOG("Endpoint %{public}s not available, stop call release", endpointName.c_str());
        return;
    }
    temp = endpointList_[endpointName];
    if (temp->GetStatus() == AudioEndpoint::EndpointStatus::UNLINKED) {
        AUDIO_INFO_LOG("%{public}s not in use anymore, call release!", endpointName.c_str());
        temp->Release();
        temp = nullptr;
        endpointList_.erase(endpointName);
        return;
    }
    AUDIO_WARNING_LOG("%{public}s is not unlinked, stop call release", endpointName.c_str());
    return;
}

DeviceInfo AudioService::GetDeviceInfoForProcess(const AudioProcessConfig &config)
{
    // send the config to AudioPolicyServera and get the device info.
    DeviceInfo deviceInfo;
    bool ret = PolicyHandler::GetInstance().GetProcessDeviceInfo(config, deviceInfo);
    if (ret) {
        AUDIO_INFO_LOG("Get DeviceInfo from policy server success, deviceType is%{public}d", deviceInfo.deviceType);
        return deviceInfo;
    } else {
        AUDIO_WARNING_LOG("GetProcessDeviceInfo from audio policy server failed!");
    }

    if (config.audioMode == AUDIO_MODE_RECORD) {
        deviceInfo.deviceId = 1;
        if (config.isRemote) {
            deviceInfo.networkId = "remote_mmap_dmic";
        } else {
            deviceInfo.networkId = LOCAL_NETWORK_ID;
        }
        deviceInfo.deviceRole = INPUT_DEVICE;
        deviceInfo.deviceType = DEVICE_TYPE_MIC;
    } else {
        deviceInfo.deviceId = 6; // 6 for test
        if (config.isRemote) {
            deviceInfo.networkId = REMOTE_NETWORK_ID;
        } else {
            deviceInfo.networkId = LOCAL_NETWORK_ID;
        }
        deviceInfo.deviceRole = OUTPUT_DEVICE;
        deviceInfo.deviceType = DEVICE_TYPE_SPEAKER;
    }
    AudioStreamInfo targetStreamInfo = {SAMPLE_RATE_48000, ENCODING_PCM, SAMPLE_S16LE, STEREO}; // note: read from xml
    deviceInfo.audioStreamInfo = targetStreamInfo;
    deviceInfo.deviceName = "mmap_device";
    return deviceInfo;
}

std::shared_ptr<AudioEndpoint> AudioService::GetAudioEndpointForDevice(DeviceInfo deviceInfo)
{
    // temp method to get deivce key
    std::string deviceKey = deviceInfo.networkId + std::to_string(deviceInfo.deviceId);
    if (endpointList_.find(deviceKey) != endpointList_.end()) {
        AUDIO_INFO_LOG("AudioService find endpoint already exist for deviceKey:%{public}s", deviceKey.c_str());
        reusingEndpoint_ = deviceKey;
        return endpointList_[deviceKey];
    }
    std::shared_ptr<AudioEndpoint> endpoint = AudioEndpoint::GetInstance(AudioEndpoint::EndpointType::TYPE_MMAP,
        deviceInfo);
    if (endpoint == nullptr) {
        AUDIO_ERR_LOG("Find no endpoint for the process");
        return nullptr;
    }
    endpointList_[deviceKey] = endpoint;
    return endpoint;
}

void AudioService::Dump(std::stringstream &dumpStringStream)
{
    AUDIO_INFO_LOG("AudioService dump begin");
    // dump process
    for (auto paired : linkedPairedList_) {
        paired.first->Dump(dumpStringStream);
    }
    // dump endpoint
    for (auto item : endpointList_) {
        dumpStringStream << std::endl << "Endpoint device id:" << item.first << std::endl;
        item.second->Dump(dumpStringStream);
    }
    PolicyHandler::GetInstance().Dump(dumpStringStream);
}
} // namespace AudioStandard
} // namespace OHOS
