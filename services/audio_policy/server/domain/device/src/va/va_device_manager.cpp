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
#define LOG_TAG "VADeviceManager"
#endif

#include "va_device_manager.h"
#include "audio_policy_log.h"
#include "audio_module_info.h"

namespace OHOS {
namespace AudioStandard {

static const std::string VA_DEVICE_INFO_NAME = "Virtual Audio";
static const std::string VA_INPUT_PIPE_INFO_NAME = "va_input";

VADeviceManager &VADeviceManager::GetInstance()
{
    static VADeviceManager instance;
    return instance;
}

std::shared_ptr<AudioDeviceDescriptor> VADeviceManager::ConvertVADeviceToDescriptor(
    const std::shared_ptr<VADevice> &vaDevice)
{
    if (vaDevice == nullptr) {
        return nullptr;
    }
    auto desc = std::make_shared<AudioDeviceDescriptor>();
    auto config = vaDevice->configuration_;
    desc->deviceName_ = config.name_;
    desc->displayName_ = config.name_;
    switch (config.type_) {
        case VA_DEVICE_TYPE_NONE:
            desc->deviceType_ = DEVICE_TYPE_NONE;
            break;
        case VA_DEVICE_TYPE_BT_SPP:
            desc->deviceType_ = DEVICE_TYPE_BT_SPP;
            break;
        default:
            desc->deviceType_ = DEVICE_TYPE_NONE;
            break;
    }
    switch (config.role_) {
        case VA_DEVICE_ROLE_IN:
            desc->deviceRole_ = INPUT_DEVICE;
            break;
        case VA_DEVICE_ROLE_OUT:
            desc->deviceRole_ = OUTPUT_DEVICE;
            break;
        default:
            desc->deviceRole_ = DEVICE_ROLE_NONE;
            break;
    }
    desc->macAddress_ = config.address_;

    for (const auto &streamProp : config.properties_) {
        std::shared_ptr<DeviceStreamInfo> streamInfo = ConvertVAStreamPropertyToInfo(streamProp);
        desc->audioStreamInfo_.push_back(*streamInfo);
    }
    return desc;
}

std::shared_ptr<DeviceStreamInfo> VADeviceManager::ConvertVAStreamPropertyToInfo(
    const VAAudioStreamProperty &vaStreamProperty)
{
    std::shared_ptr<DeviceStreamInfo> streamInfo = std::make_shared<DeviceStreamInfo>();
    streamInfo->encoding = vaStreamProperty.encoding_;
    streamInfo->format = vaStreamProperty.sampleFormat_;
    streamInfo->channelLayout.insert(vaStreamProperty.channelLayout_);
    streamInfo->samplingRate.insert(static_cast<AudioSamplingRate>(vaStreamProperty.sampleRate_));
    return streamInfo;
}

void VADeviceManager::OnDevicesConnected(
    const std::shared_ptr<VADevice> &vaDevice,
    const sptr<IVADeviceController> &controller)
{
    CHECK_AND_RETURN_LOG(vaDevice != nullptr && controller != nullptr, "invalid parameter: null pointer detected");
    AUDIO_INFO_LOG("va device manager connecting to device: {\"name\":\"%{public}s\", \"type\":\"%{public}d\"}",
                   vaDevice->configuration_.name_.c_str(), vaDevice->configuration_.type_);
    std::shared_ptr<AudioDeviceDescriptor> descriptor = ConvertVADeviceToDescriptor(vaDevice);
    connectedVADeviceMap_[vaDevice->configuration_.address_] = controller;

    if (!IsVAAdapterRegistered()) {
        RegisterVAAdapterToMap();
    }
    AddVAStreamPropToMap(vaDevice->configuration_.properties_);
    ReorganizePolicyConfig();
    AudioCoreService::GetCoreService()->GetEventEntry()->OnDeviceStatusUpdated(*descriptor, true);
}

void VADeviceManager::OnDevicesDisconnected(const std::shared_ptr<VADevice> &vaDevice)
{
    CHECK_AND_RETURN_LOG(vaDevice != nullptr, "invalid parameter: null pointer detected");
    std::shared_ptr<AudioDeviceDescriptor> descriptor = ConvertVADeviceToDescriptor(vaDevice);
    AudioCoreService::GetCoreService()->GetEventEntry()->OnDeviceStatusUpdated(*descriptor, false);
    connectedVADeviceMap_.erase(vaDevice->configuration_.address_);
    if (connectedVADeviceMap_.size() <= 0) {
        UnregisterVAAdapterFromMap();
        ReorganizePolicyConfig();
    }
}

void VADeviceManager::GetDeviceController(const std::string macAddr, sptr<IRemoteObject> &controller)
{
    auto it = connectedVADeviceMap_.find(macAddr);
    if (it == connectedVADeviceMap_.end()) {
        controller = nullptr;
    } else {
        sptr<IVADeviceController> vaController = it->second;
        if (vaController == nullptr) {
            controller = nullptr;
        } else {
            controller = vaController->AsObject();
        }
    }
}

void VADeviceManager::RegisterVAAdapterToMap()
{
    AudioPolicyConfigData &config = AudioPolicyConfigData::GetInstance();
    PolicyAdapterInfo adapterInfo{};
    std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = std::make_shared<PolicyAdapterInfo>(adapterInfo);
    adapterInfoPtr->adapterName = ADAPTER_TYPE_VA;
    config.adapterInfoMap.insert({adapterInfoPtr->GetTypeEnum(), adapterInfoPtr});

    std::shared_ptr<AdapterPipeInfo> pipeInfo = std::make_shared<AdapterPipeInfo>();
    pipeInfo->adapterInfo_ = adapterInfoPtr;
    pipeInfo->name_ = VA_INPUT_PIPE_INFO_NAME;
    pipeInfo->role_ = PIPE_ROLE_INPUT;
    pipeInfo->supportDevices_.push_back(VA_DEVICE_INFO_NAME);

    AdapterDeviceInfo deviceInfo{};
    deviceInfo.adapterInfo_ = adapterInfoPtr;
    deviceInfo.name_ = VA_DEVICE_INFO_NAME;
    deviceInfo.type_ = DEVICE_TYPE_BT_SPP;
    deviceInfo.role_ = INPUT_DEVICE;
    deviceInfo.supportPipes_.push_back(VA_INPUT_PIPE_INFO_NAME);
    deviceInfo.supportPipeMap_[AudioFlag::AUDIO_INPUT_FLAG_NORMAL] = pipeInfo;

    adapterInfoPtr->deviceInfos.push_back(make_shared<AdapterDeviceInfo>(deviceInfo));

    PaPropInfo paProp = {};
    paProp.lib_ = "libmodule-hdi-source.z.so";
    paProp.role_ = "source";
    paProp.moduleName_ = VIRTUAL_AUDIO;

    pipeInfo->paProp_ = std::move(paProp);

    adapterInfoPtr->pipeInfos.push_back(pipeInfo);
}

void VADeviceManager::UnregisterVAAdapterFromMap()
{
    AudioPolicyConfigData &config = AudioPolicyConfigData::GetInstance();
    config.adapterInfoMap.erase(AudioAdapterType::TYPE_VA);
    std::pair<DeviceType, DeviceRole> deviceMapKey = std::make_pair(DEVICE_TYPE_BT_SPP, INPUT_DEVICE);
    config.deviceInfoMap.erase(deviceMapKey);
}

void VADeviceManager::AddVAStreamPropToMap(std::list<VAAudioStreamProperty> properties)
{
    AudioPolicyConfigData &config = AudioPolicyConfigData::GetInstance();
    auto it = config.adapterInfoMap.find(AudioAdapterType::TYPE_VA);
    CHECK_AND_RETURN_LOG(it != config.adapterInfoMap.end(), "va adapter not found");
    std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = it->second;
    CHECK_AND_RETURN_LOG(adapterInfoPtr != nullptr, "va adapter is null pointer");
    CHECK_AND_RETURN_LOG(adapterInfoPtr->pipeInfos.size() > 0, "va pipe info not found");
    std::shared_ptr<AdapterPipeInfo> pipeInfoPtr = adapterInfoPtr->pipeInfos.front();

    for (auto vaStreamProp : properties) {
        std::shared_ptr<PipeStreamPropInfo> pipeStreamPropInfo =
            ConvertVADeviceStreamPropertyToPipeStreamPropInfo(vaStreamProp);
        CHECK_AND_RETURN_LOG(pipeStreamPropInfo != nullptr, "pipeStreamPropInfo is null");
        pipeStreamPropInfo->pipeInfo_ = pipeInfoPtr;
        pipeStreamPropInfo->supportDevices_.push_back(VA_DEVICE_INFO_NAME);
        pipeInfoPtr->streamPropInfos_.push_back(pipeStreamPropInfo);
    }
}

void VADeviceManager::ReorganizePolicyConfig()
{
    AudioPolicyConfigData::GetInstance().Reorganize();
}

bool VADeviceManager::IsVAAdapterRegistered()
{
    AudioPolicyConfigData &config = AudioPolicyConfigData::GetInstance();
    return config.adapterInfoMap.find(AudioAdapterType::TYPE_VA) != config.adapterInfoMap.end();
}

bool VADeviceManager::IsDeviceInfoMapContainsVA(std::pair<DeviceType, DeviceRole> deviceMapKey)
{
    AudioPolicyConfigData &config = AudioPolicyConfigData::GetInstance();
    return config.deviceInfoMap.find(deviceMapKey) != config.deviceInfoMap.end();
}

std::shared_ptr<PipeStreamPropInfo> VADeviceManager::ConvertVADeviceStreamPropertyToPipeStreamPropInfo(
    const VAAudioStreamProperty &vaStreamProperty)
{
    std::shared_ptr<PipeStreamPropInfo> pipeStreamInfo = std::make_shared<PipeStreamPropInfo>();
    pipeStreamInfo->format_ = vaStreamProperty.sampleFormat_;
    pipeStreamInfo->sampleRate_ = vaStreamProperty.sampleRate_;
    pipeStreamInfo->channelLayout_ = vaStreamProperty.channelLayout_;
    pipeStreamInfo->channels_ = AudioDefinitionPolicyUtils::ConvertLayoutToAudioChannel(pipeStreamInfo->channelLayout_);
    pipeStreamInfo->bufferSize_ = CalculateBufferSize(vaStreamProperty);
    return pipeStreamInfo;
}

int32_t VADeviceManager::CalculateBufferSize(const VAAudioStreamProperty &vaStreamProperty)
{
    const int32_t cyclesPerSecond = 50;
    if (vaStreamProperty.samplesPerCycle_ >
            static_cast<int32_t>(AudioSamplingRate::SAMPLE_RATE_192000) / cyclesPerSecond ||
            vaStreamProperty.samplesPerCycle_ < 0) {
        AUDIO_ERR_LOG("invalid samplesPerCycle");
        return 0;
    }
    int32_t channels = AudioDefinitionPolicyUtils::ConvertLayoutToAudioChannel(vaStreamProperty.channelLayout_);
    int32_t samplesPerCycle_ = vaStreamProperty.samplesPerCycle_;
    int32_t bitWidth = 2;
    int32_t bufferSize = channels * samplesPerCycle_ * bitWidth;
    return bufferSize;
}
}  // namespace VirtualAudioDevice
}  // namespace OHOS