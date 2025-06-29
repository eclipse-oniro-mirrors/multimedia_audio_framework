/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#include "audio_device_descriptor.h"

#include <cinttypes>
#include "audio_service_log.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {
constexpr int32_t API_VERSION_18 = 18;

const std::map<DeviceType, std::string> deviceTypeStringMap = {
    {DEVICE_TYPE_INVALID, "INVALID"},
    {DEVICE_TYPE_EARPIECE, "EARPIECE"},
    {DEVICE_TYPE_SPEAKER, "SPEAKER"},
    {DEVICE_TYPE_WIRED_HEADSET, "WIRED_HEADSET"},
    {DEVICE_TYPE_WIRED_HEADPHONES, "WIRED_HEADPHONES"},
    {DEVICE_TYPE_BLUETOOTH_SCO, "BLUETOOTH_SCO"},
    {DEVICE_TYPE_BLUETOOTH_A2DP, "BLUETOOTH_A2DP"},
    {DEVICE_TYPE_BLUETOOTH_A2DP_IN, "BLUETOOTH_A2DP_IN"},
    {DEVICE_TYPE_NEARLINK, "NEARLINK"},
    {DEVICE_TYPE_NEARLINK_IN, "NEARLINK_IN"},
    {DEVICE_TYPE_MIC, "MIC"},
    {DEVICE_TYPE_WAKEUP, "WAKEUP"},
    {DEVICE_TYPE_USB_HEADSET, "USB_HEADSET"},
    {DEVICE_TYPE_DP, "DP"},
    {DEVICE_TYPE_REMOTE_CAST, "REMOTE_CAST"},
    {DEVICE_TYPE_USB_DEVICE, "USB_DEVICE"},
    {DEVICE_TYPE_ACCESSORY, "ACCESSORY"},
    {DEVICE_TYPE_REMOTE_DAUDIO, "REMOTE_DAUDIO"},
    {DEVICE_TYPE_HDMI, "HDMI"},
    {DEVICE_TYPE_LINE_DIGITAL, "LINE_DIGITAL"},
    {DEVICE_TYPE_FILE_SINK, "FILE_SINK"},
    {DEVICE_TYPE_FILE_SOURCE, "FILE_SOURCE"},
    {DEVICE_TYPE_EXTERN_CABLE, "EXTERN_CABLE"},
    {DEVICE_TYPE_DEFAULT, "DEFAULT"},
    {DEVICE_TYPE_USB_ARM_HEADSET, "USB_ARM_HEADSET"}
};

static const char *DeviceTypeToString(DeviceType type)
{
    if (deviceTypeStringMap.count(type) != 0) {
        return deviceTypeStringMap.at(type).c_str();
    }
    return "UNKNOWN";
}

AudioDeviceDescriptor::AudioDeviceDescriptor(int32_t descriptorType)
    : AudioDeviceDescriptor(DeviceType::DEVICE_TYPE_NONE, DeviceRole::DEVICE_ROLE_NONE)
{
    descriptorType_ = descriptorType;
    if (descriptorType_ == DEVICE_INFO) {
        deviceType_ = DeviceType(0);
        deviceRole_ = DeviceRole(0);
        networkId_ = "";
    }
}

AudioDeviceDescriptor::AudioDeviceDescriptor(DeviceType type, DeviceRole role)
    : deviceType_(type), deviceRole_(role)
{
    deviceId_ = 0;
    audioStreamInfo_ = {};
    channelMasks_ = 0;
    channelIndexMasks_ = 0;
    deviceName_ = "";
    macAddress_ = "";
    volumeGroupId_ = 0;
    interruptGroupId_ = 0;
    networkId_ = LOCAL_NETWORK_ID;
    displayName_ = "";
    deviceCategory_ = CATEGORY_DEFAULT;
    connectTimeStamp_ = 0;
    connectState_ = CONNECTED;
    pairDeviceDescriptor_ = nullptr;
    isScoRealConnected_ = false;
    isEnable_ = true;
    exceptionFlag_ = false;
    isLowLatencyDevice_ = false;
    a2dpOffloadFlag_ = 0;
    descriptorType_ = AUDIO_DEVICE_DESCRIPTOR;
    spatializationSupported_ = false;
}

AudioDeviceDescriptor::AudioDeviceDescriptor(DeviceType type, DeviceRole role, int32_t interruptGroupId,
    int32_t volumeGroupId, std::string networkId)
    : deviceType_(type), deviceRole_(role), interruptGroupId_(interruptGroupId), volumeGroupId_(volumeGroupId),
    networkId_(networkId)
{
    deviceId_ = 0;
    audioStreamInfo_ = {};
    channelMasks_ = 0;
    channelIndexMasks_ = 0;
    deviceName_ = "";
    macAddress_ = "";
    displayName_ = "";
    deviceCategory_ = CATEGORY_DEFAULT;
    connectTimeStamp_ = 0;
    connectState_ = CONNECTED;
    pairDeviceDescriptor_ = nullptr;
    isScoRealConnected_ = false;
    isEnable_ = true;
    exceptionFlag_ = false;
    isLowLatencyDevice_ = false;
    a2dpOffloadFlag_ = 0;
    descriptorType_ = AUDIO_DEVICE_DESCRIPTOR;
    spatializationSupported_ = false;
}

AudioDeviceDescriptor::AudioDeviceDescriptor(const AudioDeviceDescriptor &deviceDescriptor)
{
    deviceId_ = deviceDescriptor.deviceId_;
    deviceName_ = deviceDescriptor.deviceName_;
    macAddress_ = deviceDescriptor.macAddress_;
    deviceType_ = deviceDescriptor.deviceType_;
    deviceRole_ = deviceDescriptor.deviceRole_;
    audioStreamInfo_.channels = deviceDescriptor.audioStreamInfo_.channels;
    audioStreamInfo_.encoding = deviceDescriptor.audioStreamInfo_.encoding;
    audioStreamInfo_.format = deviceDescriptor.audioStreamInfo_.format;
    audioStreamInfo_.samplingRate = deviceDescriptor.audioStreamInfo_.samplingRate;
    channelMasks_ = deviceDescriptor.channelMasks_;
    channelIndexMasks_ = deviceDescriptor.channelIndexMasks_;
    volumeGroupId_ = deviceDescriptor.volumeGroupId_;
    interruptGroupId_ = deviceDescriptor.interruptGroupId_;
    networkId_ = deviceDescriptor.networkId_;
    dmDeviceType_ = deviceDescriptor.dmDeviceType_;
    displayName_ = deviceDescriptor.displayName_;
    deviceCategory_ = deviceDescriptor.deviceCategory_;
    connectTimeStamp_ = deviceDescriptor.connectTimeStamp_;
    connectState_ = deviceDescriptor.connectState_;
    pairDeviceDescriptor_ = deviceDescriptor.pairDeviceDescriptor_;
    isScoRealConnected_ = deviceDescriptor.isScoRealConnected_;
    isEnable_ = deviceDescriptor.isEnable_;
    exceptionFlag_ = deviceDescriptor.exceptionFlag_;
    // DeviceInfo
    isLowLatencyDevice_ = deviceDescriptor.isLowLatencyDevice_;
    a2dpOffloadFlag_ = deviceDescriptor.a2dpOffloadFlag_;
    // Other
    descriptorType_ = deviceDescriptor.descriptorType_;
    hasPair_ = deviceDescriptor.hasPair_;
    spatializationSupported_ = deviceDescriptor.spatializationSupported_;
}

AudioDeviceDescriptor::AudioDeviceDescriptor(const std::shared_ptr<AudioDeviceDescriptor> &deviceDescriptor)
{
    CHECK_AND_RETURN_LOG(deviceDescriptor != nullptr, "Error input parameter");
    deviceId_ = deviceDescriptor->deviceId_;
    deviceName_ = deviceDescriptor->deviceName_;
    macAddress_ = deviceDescriptor->macAddress_;
    deviceType_ = deviceDescriptor->deviceType_;
    deviceRole_ = deviceDescriptor->deviceRole_;
    audioStreamInfo_.channels = deviceDescriptor->audioStreamInfo_.channels;
    audioStreamInfo_.encoding = deviceDescriptor->audioStreamInfo_.encoding;
    audioStreamInfo_.format = deviceDescriptor->audioStreamInfo_.format;
    audioStreamInfo_.samplingRate = deviceDescriptor->audioStreamInfo_.samplingRate;
    channelMasks_ = deviceDescriptor->channelMasks_;
    channelIndexMasks_ = deviceDescriptor->channelIndexMasks_;
    volumeGroupId_ = deviceDescriptor->volumeGroupId_;
    interruptGroupId_ = deviceDescriptor->interruptGroupId_;
    networkId_ = deviceDescriptor->networkId_;
    dmDeviceType_ = deviceDescriptor->dmDeviceType_;
    displayName_ = deviceDescriptor->displayName_;
    deviceCategory_ = deviceDescriptor->deviceCategory_;
    connectTimeStamp_ = deviceDescriptor->connectTimeStamp_;
    connectState_ = deviceDescriptor->connectState_;
    pairDeviceDescriptor_ = deviceDescriptor->pairDeviceDescriptor_;
    isScoRealConnected_ = deviceDescriptor->isScoRealConnected_;
    isEnable_ = deviceDescriptor->isEnable_;
    exceptionFlag_ = deviceDescriptor->exceptionFlag_;
    // DeviceInfo
    isLowLatencyDevice_ = deviceDescriptor->isLowLatencyDevice_;
    a2dpOffloadFlag_ = deviceDescriptor->a2dpOffloadFlag_;
    // Other
    descriptorType_ = deviceDescriptor->descriptorType_;
    hasPair_ = deviceDescriptor->hasPair_;
    spatializationSupported_ = deviceDescriptor->spatializationSupported_;
}

AudioDeviceDescriptor::~AudioDeviceDescriptor()
{
    pairDeviceDescriptor_ = nullptr;
}

DeviceType AudioDeviceDescriptor::getType() const
{
    return deviceType_;
}

DeviceRole AudioDeviceDescriptor::getRole() const
{
    return deviceRole_;
}

DeviceCategory AudioDeviceDescriptor::GetDeviceCategory() const
{
    return deviceCategory_;
}

bool AudioDeviceDescriptor::IsAudioDeviceDescriptor() const
{
    return descriptorType_ == AUDIO_DEVICE_DESCRIPTOR;
}

bool AudioDeviceDescriptor::Marshalling(Parcel &parcel) const
{
    return Marshalling(parcel, 0);
}

bool AudioDeviceDescriptor::Marshalling(Parcel &parcel, int32_t apiVersion) const
{
    if (IsAudioDeviceDescriptor()) {
        return MarshallingToDeviceDescriptor(parcel, apiVersion);
    }

    return MarshallingToDeviceInfo(parcel);
}

bool AudioDeviceDescriptor::MarshallingToDeviceDescriptor(Parcel &parcel, int32_t apiVersion) const
{
    parcel.WriteInt32(MapInternalToExternalDeviceType(apiVersion));
    parcel.WriteInt32(deviceRole_);
    parcel.WriteInt32(deviceId_);
    audioStreamInfo_.Marshalling(parcel);
    parcel.WriteInt32(channelMasks_);
    parcel.WriteInt32(channelIndexMasks_);
    parcel.WriteString(deviceName_);
    parcel.WriteString(macAddress_);
    parcel.WriteInt32(interruptGroupId_);
    parcel.WriteInt32(volumeGroupId_);
    parcel.WriteString(networkId_);
    parcel.WriteUint16(dmDeviceType_);
    parcel.WriteString(displayName_);
    parcel.WriteInt32(deviceCategory_);
    parcel.WriteInt32(connectState_);
    parcel.WriteBool(spatializationSupported_);
    parcel.WriteInt32(mediaVolume_);
    parcel.WriteInt32(callVolume_);
    return true;
}

bool AudioDeviceDescriptor::MarshallingToDeviceInfo(Parcel &parcel) const
{
    return parcel.WriteInt32(static_cast<int32_t>(deviceType_)) &&
        parcel.WriteInt32(static_cast<int32_t>(deviceRole_)) &&
        parcel.WriteInt32(deviceId_) &&
        parcel.WriteInt32(channelMasks_) &&
        parcel.WriteInt32(channelIndexMasks_) &&
        parcel.WriteString(deviceName_) &&
        parcel.WriteString(macAddress_) &&
        audioStreamInfo_.Marshalling(parcel) &&
        parcel.WriteString(networkId_) &&
        parcel.WriteUint16(dmDeviceType_) &&
        parcel.WriteString(displayName_) &&
        parcel.WriteInt32(interruptGroupId_) &&
        parcel.WriteInt32(volumeGroupId_) &&
        parcel.WriteBool(isLowLatencyDevice_) &&
        parcel.WriteInt32(a2dpOffloadFlag_) &&
        parcel.WriteInt32(static_cast<int32_t>(deviceCategory_)) &&
        parcel.WriteBool(spatializationSupported_);
}

bool AudioDeviceDescriptor::Marshalling(Parcel &parcel, bool hasBTPermission, bool hasSystemPermission,
    int32_t apiVersion) const
{
    return MarshallingToDeviceInfo(parcel, hasBTPermission, hasSystemPermission, apiVersion);
}

bool AudioDeviceDescriptor::MarshallingToDeviceInfo(Parcel &parcel, bool hasBTPermission, bool hasSystemPermission,
    int32_t apiVersion) const
{
    DeviceType devType = deviceType_;
    int32_t devId = deviceId_;
    DeviceStreamInfo streamInfo = audioStreamInfo_;

    // If api target version < 11 && does not set deviceType, fix api compatibility.
    if (apiVersion < API_11 && (deviceType_ == DEVICE_TYPE_NONE || deviceType_ == DEVICE_TYPE_INVALID)) {
        // DeviceType use speaker or mic instead.
        if (deviceRole_ == OUTPUT_DEVICE) {
            devType = DEVICE_TYPE_SPEAKER;
            devId = 1; // 1 default speaker device id.
        } else if (deviceRole_ == INPUT_DEVICE) {
            devType = DEVICE_TYPE_MIC;
            devId = 2; // 2 default mic device id.
        }

        //If does not set sampleRates use SAMPLE_RATE_44100 instead.
        if (streamInfo.samplingRate.empty()) {
            streamInfo.samplingRate.insert(SAMPLE_RATE_44100);
        }
        // If does not set channelCounts use STEREO instead.
        if (streamInfo.channels.empty()) {
            streamInfo.channels.insert(STEREO);
        }
    }

    return parcel.WriteInt32(static_cast<int32_t>(devType)) &&
        parcel.WriteInt32(static_cast<int32_t>(deviceRole_)) &&
        parcel.WriteInt32(devId) &&
        parcel.WriteInt32(channelMasks_) &&
        parcel.WriteInt32(channelIndexMasks_) &&
        parcel.WriteString((!hasBTPermission && (deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP ||
            deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO)) ? "" : deviceName_) &&
        parcel.WriteString((!hasBTPermission && (deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP ||
            deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO)) ? "" : macAddress_) &&
        streamInfo.Marshalling(parcel) &&
        parcel.WriteString(hasSystemPermission ? networkId_ : "") &&
        parcel.WriteUint16(dmDeviceType_) &&
        parcel.WriteString(displayName_) &&
        parcel.WriteInt32(hasSystemPermission ? interruptGroupId_ : INVALID_GROUP_ID) &&
        parcel.WriteInt32(hasSystemPermission ? volumeGroupId_ : INVALID_GROUP_ID) &&
        parcel.WriteBool(isLowLatencyDevice_) &&
        parcel.WriteInt32(a2dpOffloadFlag_) &&
        parcel.WriteInt32(static_cast<int32_t>(deviceCategory_)) &&
        parcel.WriteBool(spatializationSupported_);
}

void AudioDeviceDescriptor::Unmarshalling(Parcel &parcel)
{
    return UnmarshallingToDeviceInfo(parcel);
}

std::shared_ptr<AudioDeviceDescriptor> AudioDeviceDescriptor::UnmarshallingPtr(Parcel &parcel)
{
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    if (audioDeviceDescriptor == nullptr) {
        return nullptr;
    }

    audioDeviceDescriptor->UnmarshallingToDeviceDescriptor(parcel);
    return audioDeviceDescriptor;
}

void AudioDeviceDescriptor::UnmarshallingToDeviceDescriptor(Parcel &parcel)
{
    deviceType_ = static_cast<DeviceType>(parcel.ReadInt32());
    deviceRole_ = static_cast<DeviceRole>(parcel.ReadInt32());
    deviceId_ = parcel.ReadInt32();
    audioStreamInfo_.Unmarshalling(parcel);
    channelMasks_ = parcel.ReadInt32();
    channelIndexMasks_ = parcel.ReadInt32();
    deviceName_ = parcel.ReadString();
    macAddress_ = parcel.ReadString();
    interruptGroupId_ = parcel.ReadInt32();
    volumeGroupId_ = parcel.ReadInt32();
    networkId_ = parcel.ReadString();
    dmDeviceType_ = parcel.ReadUint16();
    displayName_ = parcel.ReadString();
    deviceCategory_ = static_cast<DeviceCategory>(parcel.ReadInt32());
    connectState_ = static_cast<ConnectState>(parcel.ReadInt32());
    spatializationSupported_ = parcel.ReadBool();
    mediaVolume_ = parcel.ReadInt32();
    callVolume_ = parcel.ReadInt32();
}

void AudioDeviceDescriptor::UnmarshallingToDeviceInfo(Parcel &parcel)
{
    deviceType_ = static_cast<DeviceType>(parcel.ReadInt32());
    deviceRole_ = static_cast<DeviceRole>(parcel.ReadInt32());
    deviceId_ = parcel.ReadInt32();
    channelMasks_ = parcel.ReadInt32();
    channelIndexMasks_ = parcel.ReadInt32();
    deviceName_ = parcel.ReadString();
    macAddress_ = parcel.ReadString();
    audioStreamInfo_.Unmarshalling(parcel);
    networkId_ = parcel.ReadString();
    dmDeviceType_ = parcel.ReadUint16();
    displayName_ = parcel.ReadString();
    interruptGroupId_ = parcel.ReadInt32();
    volumeGroupId_ = parcel.ReadInt32();
    isLowLatencyDevice_ = parcel.ReadBool();
    a2dpOffloadFlag_ = parcel.ReadInt32();
    deviceCategory_ = static_cast<DeviceCategory>(parcel.ReadInt32());
    spatializationSupported_ = parcel.ReadBool();
}

void AudioDeviceDescriptor::SetDeviceInfo(std::string deviceName, std::string macAddress)
{
    deviceName_ = deviceName;
    macAddress_ = macAddress;
}

void AudioDeviceDescriptor::SetDeviceCapability(const DeviceStreamInfo &audioStreamInfo, int32_t channelMask,
    int32_t channelIndexMasks)
{
    audioStreamInfo_.channels = audioStreamInfo.channels;
    audioStreamInfo_.encoding = audioStreamInfo.encoding;
    audioStreamInfo_.format = audioStreamInfo.format;
    audioStreamInfo_.samplingRate = audioStreamInfo.samplingRate;
    channelMasks_ = channelMask;
    channelIndexMasks_ = channelIndexMasks;
}

bool AudioDeviceDescriptor::IsSameDeviceDesc(const AudioDeviceDescriptor &deviceDescriptor) const
{
    return deviceDescriptor.deviceType_ == deviceType_ &&
        deviceDescriptor.macAddress_ == macAddress_ &&
        deviceDescriptor.networkId_ == networkId_ &&
        (!IsUsb(deviceType_) || deviceDescriptor.deviceRole_ == deviceRole_);
}

bool AudioDeviceDescriptor::IsSameDeviceDescPtr(std::shared_ptr<AudioDeviceDescriptor> deviceDescriptor) const
{
    return deviceDescriptor->deviceType_ == deviceType_ &&
        deviceDescriptor->macAddress_ == macAddress_ &&
        deviceDescriptor->networkId_ == networkId_ &&
        (!IsUsb(deviceType_) || deviceDescriptor->deviceRole_ == deviceRole_);
}

bool AudioDeviceDescriptor::IsSameDeviceInfo(const AudioDeviceDescriptor &deviceInfo) const
{
    return deviceType_ == deviceInfo.deviceType_ &&
        deviceRole_ == deviceInfo.deviceRole_ &&
        macAddress_ == deviceInfo.macAddress_ &&
        networkId_ == deviceInfo.networkId_;
}

bool AudioDeviceDescriptor::IsPairedDeviceDesc(const AudioDeviceDescriptor &deviceDescriptor) const
{
    return ((deviceDescriptor.deviceRole_ == INPUT_DEVICE && deviceRole_ == OUTPUT_DEVICE) ||
        (deviceDescriptor.deviceRole_ == OUTPUT_DEVICE && deviceRole_ == INPUT_DEVICE)) &&
        deviceDescriptor.deviceType_ == deviceType_ &&
        deviceDescriptor.macAddress_ == macAddress_ &&
        deviceDescriptor.networkId_ == networkId_;
}

bool AudioDeviceDescriptor::IsDistributedSpeaker() const
{
    return deviceType_ == DEVICE_TYPE_SPEAKER && networkId_ != "LocalDevice";
}

void AudioDeviceDescriptor::Dump(std::string &dumpString)
{
    AppendFormat(dumpString, "      - device %d: role %s type %d (%s) name: %s\n",
        deviceId_, IsOutput() ? "Output" : "Input",
        deviceType_, DeviceTypeToString(deviceType_), deviceName_.c_str());
}

std::string AudioDeviceDescriptor::GetDeviceTypeString()
{
    return std::string(DeviceTypeToString(deviceType_));
}

DeviceType AudioDeviceDescriptor::MapInternalToExternalDeviceType(int32_t apiVersion) const
{
    switch (deviceType_) {
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
            if (!hasPair_ && apiVersion >= API_VERSION_18) {
#ifdef DETECT_SOUNDBOX
                return DEVICE_TYPE_USB_DEVICE;
#else
                if (deviceRole_ == INPUT_DEVICE) {
                    return DEVICE_TYPE_USB_DEVICE;
                }
#endif
            }
            return DEVICE_TYPE_USB_HEADSET;
        case DEVICE_TYPE_BLUETOOTH_A2DP_IN:
            return DEVICE_TYPE_BLUETOOTH_A2DP;
        case DEVICE_TYPE_NEARLINK_IN:
            return DEVICE_TYPE_NEARLINK;
        default:
            return deviceType_;
    }
}
} // AudioStandard
} // namespace OHOS
