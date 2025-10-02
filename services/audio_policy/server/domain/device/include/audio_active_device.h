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
#ifndef ST_AUDIO_ACTIVE_DEVICE_H
#define ST_AUDIO_ACTIVE_DEVICE_H

#include <bitset>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include "singleton.h"
#include "audio_group_handle.h"
#include "audio_module_info.h"
#include "audio_volume_config.h"
#include "audio_system_manager.h"
#include "audio_errors.h"
#include "audio_device_manager.h"
#include "audio_affinity_manager.h"
#include "audio_a2dp_offload_flag.h"
#include "audio_a2dp_device.h"
#include "audio_iohandle_map.h"
#include "audio_connected_device.h"

namespace OHOS {
namespace AudioStandard {

using InternalDeviceType = DeviceType;

class AudioActiveDevice {
public:
    static AudioActiveDevice& GetInstance()
    {
        static AudioActiveDevice instance;
        return instance;
    }
    bool IsDirectSupportedDevice();
    void NotifyUserSelectionEventToBt(std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor,
        StreamUsage streamUsage = STREAM_USAGE_UNKNOWN);
    void NotifyUserDisSelectionEventToBt(std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor,
        bool isSameDevice = false);
    void NotifyUserSelectionEventForInput(std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor,
        SourceType sourceType = SOURCE_TYPE_INVALID);

    bool UpdateDevice(std::shared_ptr<AudioDeviceDescriptor> &desc, const AudioStreamDeviceChangeReasonExt reason,
        const std::shared_ptr<AudioRendererChangeInfo> &rendererChangeInfo);
    bool IsDeviceActive(DeviceType deviceType);
    float GetMaxAmplitude(const int32_t deviceId, const AudioInterrupt audioInterrupt);
    std::string GetActiveBtDeviceMac();
    void SetActiveBtDeviceMac(const std::string macAddress);
    void SetActiveBtInDeviceMac(const std::string macAddress);
    int32_t SetDeviceActive(DeviceType deviceType, bool active, const int32_t uid = INVALID_UID);
    int32_t SetCallDeviceActive(DeviceType deviceType, bool active, std::string address,
        const int32_t uid = INVALID_UID);
    bool GetActiveA2dpDeviceStreamInfo(DeviceType deviceType, AudioStreamInfo &streamInfo);

    void SetCurrentInputDevice(const AudioDeviceDescriptor &desc);
    const AudioDeviceDescriptor GetCurrentInputDevice();
    DeviceType GetCurrentInputDeviceType();
    std::string GetCurrentInputDeviceMacAddr();
    void SetCurrentOutputDevice(const AudioDeviceDescriptor &desc);
    const AudioDeviceDescriptor GetCurrentOutputDevice();
    DeviceType GetCurrentOutputDeviceType();
    DeviceCategory GetCurrentOutputDeviceCategory();
    std::string GetCurrentOutputDeviceNetworkId();
    std::string GetCurrentOutputDeviceMacAddr();
    void UpdateActiveDeviceRoute(InternalDeviceType deviceType, DeviceFlag deviceFlag,
        const std::string &deviceName = "", std::string networkId = LOCAL_NETWORK_ID);
    void UpdateActiveDevicesRoute(std::vector<std::pair<InternalDeviceType, DeviceFlag>> &activeDevices,
        const std::string &deviceName = "");
    bool IsDeviceInVector(std::shared_ptr<AudioDeviceDescriptor> desc,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> descs);
    void UpdateStreamDeviceMap(std::string source);
    bool IsDeviceInActiveOutputDevices(DeviceType type, bool isRemote);
    bool IsDeviceInActiveOutputDevices(std::shared_ptr<AudioDeviceDescriptor> desc);

    std::shared_ptr<AudioDeviceDescriptor> GetDeviceForVolume(StreamUsage streamUsage);
    std::shared_ptr<AudioDeviceDescriptor> GetDeviceForVolume(AudioStreamType streamType);
    std::shared_ptr<AudioDeviceDescriptor> GetDeviceForVolume();
    std::shared_ptr<AudioDeviceDescriptor> GetDeviceForVolume(int32_t appUid);
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetActiveOutputDevices();

private:
    AudioActiveDevice()
        : audioDeviceManager_(AudioDeviceManager::GetAudioDeviceManager()),
        audioAffinityManager_(AudioAffinityManager::GetAudioAffinityManager()),
        audioA2dpDevice_(AudioA2dpDevice::GetInstance()),
        audioA2dpOffloadFlag_(AudioA2dpOffloadFlag::GetInstance()),
        audioConnectedDevice_(AudioConnectedDevice::GetInstance()) {}
    ~AudioActiveDevice() {}
    void WriteOutputRouteChangeEvent(std::shared_ptr<AudioDeviceDescriptor> &desc,
        const AudioStreamDeviceChangeReason reason);
    void HandleActiveBt(DeviceType deviceType, std::string macAddress);
    void HandleNegtiveBt(DeviceType deviceType);
    void UpdateVolumeTypeDeviceMap(std::shared_ptr<AudioStreamDescriptor> desc);
    void UpdateStreamUsageDeviceMap(std::shared_ptr<AudioStreamDescriptor> desc);
    void SortDevicesByPriority(std::vector<std::shared_ptr<AudioDeviceDescriptor>> &descs);
    int32_t GetRealUid(std::shared_ptr<AudioStreamDescriptor> streamDesc);
    bool IsAvailableFrontDeviceInVector(
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> descs);

private:
    std::mutex curOutputDevice_; // lock this mutex to operate currentActiveDevice_
    AudioDeviceDescriptor currentActiveDevice_ = AudioDeviceDescriptor(DEVICE_TYPE_NONE, DEVICE_ROLE_NONE);
    std::mutex curInputDevice_; // lock this mutex to operate currentActiveInputDevice_
    AudioDeviceDescriptor currentActiveInputDevice_ = AudioDeviceDescriptor(DEVICE_TYPE_NONE, DEVICE_ROLE_NONE);
    
    std::mutex deviceForVolumeMutex_;

    std::string activeBTDevice_;
    std::string activeBTInDevice_;
    std::vector<std::shared_ptr<AudioDeviceDescriptor>> activeOutputDevices_;
    std::unordered_map<AudioVolumeType, std::vector<std::shared_ptr<AudioDeviceDescriptor>>> volumeTypeDeviceMap_;
    std::unordered_map<StreamUsage, std::vector<std::shared_ptr<AudioDeviceDescriptor>>> streamUsageDeviceMap_;
    std::shared_ptr<AudioDeviceDescriptor> defaultOutputDevice_ =
        std::make_shared<AudioDeviceDescriptor>(DEVICE_TYPE_SPEAKER, OUTPUT_DEVICE);

    AudioDeviceManager &audioDeviceManager_;
    AudioAffinityManager &audioAffinityManager_;
    AudioA2dpDevice& audioA2dpDevice_;
    AudioA2dpOffloadFlag& audioA2dpOffloadFlag_;
    AudioConnectedDevice& audioConnectedDevice_;
};

}
}

#endif
