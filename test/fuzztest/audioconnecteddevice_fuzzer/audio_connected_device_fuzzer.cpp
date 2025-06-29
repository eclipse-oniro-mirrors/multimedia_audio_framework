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

#include <iostream>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include "audio_info.h"
#include "audio_policy_server.h"
#include "audio_policy_service.h"
#include "audio_device_info.h"
#include "audio_utils.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "access_token.h"
#include "audio_channel_blend.h"
#include "volume_ramp.h"
#include "audio_speed.h"

#include "audio_policy_utils.h"
#include "audio_stream_descriptor.h"
#include "audio_limiter_manager.h"
#include "dfx_msg_manager.h"
#include "hpae_manager.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
using namespace std;

static const uint8_t* RAW_DATA = nullptr;
static size_t g_dataSize = 0;
static size_t g_pos;
const size_t THRESHOLD = 10;
const uint8_t TESTSIZE = 11;
static int32_t NUM_2 = 2;

typedef void (*TestFuncs)();

template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (RAW_DATA == nullptr || objectSize > g_dataSize - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, RAW_DATA + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

template<class T>
uint32_t GetArrLength(T& arr)
{
    if (arr == nullptr) {
        AUDIO_INFO_LOG("%{public}s: The array length is equal to 0", __func__);
        return 0;
    }
    return sizeof(arr) / sizeof(arr[0]);
}

vector<DeviceType> DeviceTypeVec = {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_INVALID,
    DEVICE_TYPE_EARPIECE,
    DEVICE_TYPE_SPEAKER,
    DEVICE_TYPE_WIRED_HEADSET,
    DEVICE_TYPE_WIRED_HEADPHONES,
    DEVICE_TYPE_BLUETOOTH_SCO,
    DEVICE_TYPE_BLUETOOTH_A2DP,
    DEVICE_TYPE_BLUETOOTH_A2DP_IN,
    DEVICE_TYPE_MIC,
    DEVICE_TYPE_WAKEUP,
    DEVICE_TYPE_USB_HEADSET,
    DEVICE_TYPE_DP,
    DEVICE_TYPE_REMOTE_CAST,
    DEVICE_TYPE_USB_DEVICE,
    DEVICE_TYPE_ACCESSORY,
    DEVICE_TYPE_REMOTE_DAUDIO,
    DEVICE_TYPE_HDMI,
    DEVICE_TYPE_LINE_DIGITAL,
    DEVICE_TYPE_NEARLINK,
    DEVICE_TYPE_NEARLINK_IN,
    DEVICE_TYPE_FILE_SINK,
    DEVICE_TYPE_FILE_SOURCE,
    DEVICE_TYPE_EXTERN_CABLE,
    DEVICE_TYPE_DEFAULT,
    DEVICE_TYPE_USB_ARM_HEADSET,
    DEVICE_TYPE_MAX,
};

vector<DeviceRole> DeviceRoleVec = {
    DEVICE_ROLE_NONE,
    INPUT_DEVICE,
    OUTPUT_DEVICE,
    DEVICE_ROLE_MAX,
};

void IsConnectedOutputDeviceFuzzTest()
{
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    DeviceType deviceType = DeviceTypeVec[deviceTypeCount];
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    DeviceRole deviceRole = DeviceRoleVec[deviceRoleCount];
    auto desc = make_shared<AudioDeviceDescriptor>(deviceType, deviceRole);
    AudioConnectedDevice::GetInstance().connectedDevices_.push_back(desc);
    AudioConnectedDevice::GetInstance().IsConnectedOutputDevice(desc);
}

void CheckExistOutputDeviceFuzzTest()
{
    std::string macAddress = "test";
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    audioConnectedDevice->CheckExistOutputDevice(DeviceTypeVec[deviceTypeCount], macAddress);
}

void CheckExistInputDeviceFuzzTest()
{
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    audioConnectedDevice->CheckExistInputDevice(DeviceTypeVec[deviceTypeCount]);
}

void GetConnectedDeviceByTypeFuzzTest()
{
    string networkId = "test";
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc->networkId_ = networkId;
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    audioConnectedDevice->GetConnectedDeviceByType(networkId, DeviceTypeVec[deviceTypeCount]);
}

void UpdateConnectDeviceFuzzTest()
{
    string macAddress = "macAddress";
    string deviceName = "deviceName";
    AudioStreamInfo streamInfo;
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    audioConnectedDevice->UpdateConnectDevice(DeviceTypeVec[deviceTypeCount], macAddress, deviceName, streamInfo);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc->macAddress_ = macAddress;
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    audioConnectedDevice->UpdateConnectDevice(DeviceTypeVec[deviceTypeCount], macAddress, deviceName, streamInfo);
}

void GetUsbDeviceDescriptorFuzzTest()
{
    std::string address = "test";
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    audioConnectedDevice->GetUsbDeviceDescriptor(address, DeviceRoleVec[deviceRoleCount]);
}

void UpdateSpatializationSupportedFuzzTest()
{
    string macAddress = "test";
    bool spatializationSupported = GetData<uint8_t>() % NUM_2;
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    string encryAddress =
        AudioSpatializationService::GetAudioSpatializationService().GetSha256EncryptAddress(macAddress);
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc1 = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc1->macAddress_ = macAddress;
    desc1->spatializationSupported_ = spatializationSupported;
    audioConnectedDevice->connectedDevices_.push_back(desc1);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc2 = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc2->macAddress_ = macAddress;
    audioConnectedDevice->connectedDevices_.push_back(desc2);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc3 = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc3->macAddress_ = macAddress;
    desc3->spatializationSupported_ = spatializationSupported;
    audioConnectedDevice->connectedDevices_.push_back(desc3);
    deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc4 = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc4->macAddress_ = macAddress;
    audioConnectedDevice->connectedDevices_.push_back(desc4);
    audioConnectedDevice->UpdateSpatializationSupported(encryAddress, spatializationSupported);
}

void CheckDeviceConnectedFuzzTest()
{
    std::string selectedDevice = "test";
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc->networkId_ = selectedDevice;
    audioConnectedDevice->connectedDevices_.push_back(desc);
    audioConnectedDevice->CheckDeviceConnected(selectedDevice);
}

void HasArmFuzzTest()
{
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    audioConnectedDevice->HasArm(DeviceRoleVec[deviceRoleCount]);
}

void HasHifiFuzzTest()
{
    auto audioConnectedDevice = std::make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    bool result = audioConnectedDevice->HasHifi(DeviceRoleVec[deviceRoleCount]);
}

void IsArmDeviceFuzzTest()
{
    string address = "test";
    auto audioConnectedDevice = make_shared<AudioConnectedDevice>();
    uint32_t deviceTypeCount = GetData<uint32_t>() % DeviceTypeVec.size();
    uint32_t deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    auto desc = make_shared<AudioDeviceDescriptor>(DeviceTypeVec[deviceTypeCount], DeviceRoleVec[deviceRoleCount]);
    desc->macAddress_ = address;
    audioConnectedDevice->connectedDevices_.push_back(desc);
    deviceRoleCount = GetData<uint32_t>() % DeviceRoleVec.size();
    bool result = audioConnectedDevice->IsArmDevice(address, DeviceRoleVec[deviceRoleCount]);
}

TestFuncs g_testFuncs[TESTSIZE] = {
    IsConnectedOutputDeviceFuzzTest,
    CheckExistOutputDeviceFuzzTest,
    CheckExistInputDeviceFuzzTest,
    GetConnectedDeviceByTypeFuzzTest,
    UpdateConnectDeviceFuzzTest,
    GetUsbDeviceDescriptorFuzzTest,
    UpdateSpatializationSupportedFuzzTest,
    CheckDeviceConnectedFuzzTest,

    HasArmFuzzTest,
    HasHifiFuzzTest,
    IsArmDeviceFuzzTest,
};

bool FuzzTest(const uint8_t* rawData, size_t size)
{
    if (rawData == nullptr) {
        return false;
    }

    // initialize data
    RAW_DATA = rawData;
    g_dataSize = size;
    g_pos = 0;

    uint32_t code = GetData<uint32_t>();
    uint32_t len = GetArrLength(g_testFuncs);
    if (len > 0) {
        g_testFuncs[code % len]();
    } else {
        AUDIO_INFO_LOG("%{public}s: The len length is equal to 0", __func__);
    }

    return true;
}
} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size < OHOS::AudioStandard::THRESHOLD) {
        return 0;
    }

    OHOS::AudioStandard::FuzzTest(data, size);
    return 0;
}
