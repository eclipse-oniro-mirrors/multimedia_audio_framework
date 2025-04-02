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

#include <gtest/gtest.h>
#include "audio_usb_manager.h"
#include "audio_policy_service.h"
#include "audio_usb_manager_unit_test.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

class DeviceStatusObserver : public IDeviceStatusObserver {
public:
    void OnDeviceStatusUpdated(DeviceType devType, bool isConnected,
        const std::string &macAddress, const std::string &deviceName,
        const AudioStreamInfo &streamInfo, DeviceRole role = DEVICE_ROLE_NONE, bool hasPair = false) override {};
    void OnMicrophoneBlockedUpdate(DeviceType devType, DeviceBlockStatus status) override {};
    void OnPnpDeviceStatusUpdated(AudioDeviceDescriptor &desc, bool isConnected) override {};
    void OnDeviceConfigurationChanged(DeviceType deviceType,
        const std::string &macAddress, const std::string &deviceName,
        const AudioStreamInfo &streamInfo) override {};
    void OnDeviceStatusUpdated(DStatusInfo statusInfo, bool isStop = false) override {};
    void OnServiceConnected(AudioServiceIndex serviceIndex) override {};
    void OnServiceDisconnected(AudioServiceIndex serviceIndex) override {};
    void OnForcedDeviceSelected(DeviceType devType, const std::string &macAddress) override {};
    void OnDeviceStatusUpdated(AudioDeviceDescriptor &desc, bool isConnected) override {};
    void OnDeviceInfoUpdated(AudioDeviceDescriptor &desc, const DeviceInfoUpdateCommand updateCommand) override {};
};

void AudioUsbManagerUnitTest::SetUpTestCase(void) {}
void AudioUsbManagerUnitTest::TearDownTestCase(void) {}
void AudioUsbManagerUnitTest::SetUp(void) {}
void AudioUsbManagerUnitTest::TearDown(void) {}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_001.
* @tc.desc  : Test Init.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_001, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    IDeviceStatusObserver *observer = nullptr;
    audioUsbManager->Init();

    audioUsbManager->initialized_ = true;
    audioUsbManager->Init();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_002.
* @tc.desc  : Test Deinit.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_002, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);
    audioUsbManager->Deinit();

    audioUsbManager->initialized_ = true;
    audioUsbManager->Deinit();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_003.
* @tc.desc  : Test Deinit.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_003, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    audioUsbManager->eventSubscriber_ = std::make_shared<AudioUsbManager::EventSubscriber>(subscribeInfo);
    audioUsbManager->initialized_ = true;
    audioUsbManager->Deinit();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_004.
* @tc.desc  : Test RefreshUsbAudioDevices.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_004, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    audioUsbManager->RefreshUsbAudioDevices();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_006.
* @tc.desc  : Test NotifyDevice.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_006, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    IDeviceStatusObserver *observer = new DeviceStatusObserver();
    audioUsbManager->Init();
    ASSERT_TRUE(observer != nullptr);

    UsbAudioDevice device;
    SoundCard soundCard;
    soundCard.isPlayer_ = true;
    soundCard.isCapturer_ = true;
    audioUsbManager->soundCardMap_.insert({device.usbAddr_, soundCard});
    audioUsbManager->NotifyDevice(device, true);
    audioUsbManager->Deinit();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_007.
* @tc.desc  : Test NotifyDevice.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_007, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    IDeviceStatusObserver *observer = new DeviceStatusObserver();
    audioUsbManager->Init();
    ASSERT_TRUE(observer != nullptr);

    UsbAudioDevice device;
    SoundCard soundCard;
    soundCard.isPlayer_ = false;
    soundCard.isCapturer_ = true;
    audioUsbManager->soundCardMap_.insert({device.usbAddr_, soundCard});
    audioUsbManager->NotifyDevice(device, true);
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_008.
* @tc.desc  : Test NotifyDevice.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_008, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    IDeviceStatusObserver *observer = new DeviceStatusObserver();
    audioUsbManager->Init();
    ASSERT_TRUE(observer != nullptr);

    UsbAudioDevice device;
    SoundCard soundCard;
    soundCard.isPlayer_ = true;
    soundCard.isCapturer_ = false;
    audioUsbManager->soundCardMap_.insert({device.usbAddr_, soundCard});
    audioUsbManager->NotifyDevice(device, true);
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_010.
* @tc.desc  : Test GetUsbAudioDevices.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_010, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);
    audioUsbManager->initialized_ = true;

    vector<UsbAudioDevice> result;
    auto ret = audioUsbManager->GetUsbAudioDevices(result);
    EXPECT_EQ(ret, SUCCESS);
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_011.
* @tc.desc  : Test HandleUsbAudioDeviceAttach.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_011, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    UsbAudioDevice device;
    audioUsbManager->HandleAudioDeviceEvent(make_pair(device, true));

    audioUsbManager->audioDevices_.push_back(device);
    audioUsbManager->HandleAudioDeviceEvent(make_pair(device, true));
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_012.
* @tc.desc  : Test HandleUsbAudioDeviceAttach.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_012, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);

    UsbAudioDevice device;
    audioUsbManager->HandleAudioDeviceEvent(make_pair(device, false));

    audioUsbManager->audioDevices_.push_back(device);
    audioUsbManager->HandleAudioDeviceEvent(make_pair(device, false));
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_013.
* @tc.desc  : Test GetUsbSoundCardMap.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_013, TestSize.Level1)
{
    auto audioUsbManager = &AudioUsbManager::GetInstance();
    ASSERT_TRUE(audioUsbManager != nullptr);
    audioUsbManager->initialized_ = true;

    AudioUsbManager::GetInstance().GetUsbSoundCardMap();
}

/**
* @tc.name  : Test AudioUsbManager.
* @tc.number: AudioUsbManagerUnitTest_014.
* @tc.desc  : Test OnReceiveEvent.
*/
HWTEST_F(AudioUsbManagerUnitTest, AudioUsbManagerUnitTest_014, TestSize.Level1)
{
    EventFwk::CommonEventSubscribeInfo subscribeInfo;
    auto subscriber = std::make_shared<AudioUsbManager::EventSubscriber>(subscribeInfo);
    ASSERT_TRUE(subscriber != nullptr);

    EventFwk::CommonEventData data;
    subscriber->OnReceiveEvent(data);

    OHOS::EventFwk::Want want;
    want.SetAction("usual.event.hardware.usb.action.USB_DEVICE_ATTACHED");
    data.SetWant(want);
    subscriber->OnReceiveEvent(data);
    want.SetAction("usual.event.hardware.usb.action.USB_DEVICE_DETACHED");
    data.SetWant(want);
    subscriber->OnReceiveEvent(data);
    string s = "{\"busNum\":1,\"devAddress\":1,\"configs\":[{\"interfaces\":[{\"clazz\":1,\"subClass\":1}]}]}";
    data.SetData(s);
    want.SetAction("usual.event.hardware.usb.action.USB_DEVICE_ATTACHED");
    data.SetWant(want);
    subscriber->OnReceiveEvent(data);
    want.SetAction("usual.event.hardware.usb.action.USB_DEVICE_DETACHED");
    data.SetWant(want);
    subscriber->OnReceiveEvent(data);
}
} // namespace AudioStandard
} // namespace OHOS
