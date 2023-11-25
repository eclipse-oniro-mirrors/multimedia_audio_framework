/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef BLUETOOTH_DEVICE_MANAGER_H
#define BLUETOOTH_DEVICE_MANAGER_H

#include <mutex>
#include "bluetooth_hfp_ag.h"
#include "bluetooth_device_utils.h"
#include "audio_info.h"
#include "audio_errors.h"
#include "audio_log.h"
#include "audio_system_manager.h"
#include "idevice_status_observer.h"

namespace OHOS {
namespace Bluetooth {

int32_t RegisterDeviceObserver(AudioStandard::IDeviceStatusObserver &observer);
void UnregisterDeviceObserver();
void SendUserSelectionEvent(AudioStandard::DeviceType devType, const std::string &macAddress, int32_t eventType);

class MediaBluetoothDeviceManager {
public:
    MediaBluetoothDeviceManager() = default;
    virtual ~MediaBluetoothDeviceManager() = default;
    static void SetMediaStack(const BluetoothRemoteDevice &device, int action);
    static void HandleConnectDevice(const BluetoothRemoteDevice &device);
    static void HandleDisconnectDevice(const BluetoothRemoteDevice &device);
    static void HandleWearDevice(const BluetoothRemoteDevice &device);
    static void HandleUnwearDevice(const BluetoothRemoteDevice &device);
    static void HandleEnableDevice(const BluetoothRemoteDevice &device);
    static void HandleDisableDevice();
    static void HandleWearEnable(const BluetoothRemoteDevice &device);
    static void HandleWearDisable(const BluetoothRemoteDevice &device);
    static void AddDeviceInConfigVector(const BluetoothRemoteDevice &device,
        std::vector<BluetoothRemoteDevice> &deviceVector);
    static void RemoveDeviceInConfigVector(const BluetoothRemoteDevice &device,
        std::vector<BluetoothRemoteDevice> &deviceVector);
    static void NotifyToUpdateAudioDevice(const BluetoothRemoteDevice &device,
        AudioStandard::DeviceCategory category, DeviceStatus deviceStatus);
    static bool IsA2dpBluetoothDeviceExist(const std::string& macAddress);
    static int32_t GetConnectedA2dpBluetoothDevice(const std::string& macAddress, BluetoothRemoteDevice &device);
    static void UpdateA2dpDeviceConfiguration(const BluetoothRemoteDevice &device,
        const AudioStandard::AudioStreamInfo &streamInfo);
    static void ClearAllA2dpBluetoothDevice();

private:
    static std::map<std::string, BluetoothRemoteDevice> a2dpBluetoothDeviceMap_;
    static std::map<std::string, BluetoothDeviceAction> wearDetectionStateMap_;
    static std::vector<BluetoothRemoteDevice> privacyDevices_;
    static std::vector<BluetoothRemoteDevice> commonDevices_;
    static std::vector<BluetoothRemoteDevice> negativeDevices_;
};

class HfpBluetoothDeviceManager {
public:
    HfpBluetoothDeviceManager() = default;
    virtual ~HfpBluetoothDeviceManager() = default;
    static void OnHfpStackChanged(const BluetoothRemoteDevice &device, bool isConnected);
};
} // namespace Bluetooth
} // namespace OHOS

#endif // BLUETOOTH_DEVICE_MANAGER_H