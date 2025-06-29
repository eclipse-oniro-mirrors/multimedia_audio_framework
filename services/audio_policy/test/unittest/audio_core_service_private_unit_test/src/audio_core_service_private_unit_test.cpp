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

#include "audio_core_service_private_unit_test.h"
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

static const int32_t BLUETOOTH_FETCH_RESULT_CONTINUE = 1;
static const int32_t BLUETOOTH_FETCH_RESULT_ERROR = 2;

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_001
 * @tc.desc  : Test AudioCoreService::GetEncryptAddr()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_001, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::string addr = "abc";

    auto ret = audioCoreService->GetEncryptAddr(addr);
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_002
 * @tc.desc  : Test AudioCoreService::GetEncryptAddr()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_002, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::string addr = "";

    auto ret = audioCoreService->GetEncryptAddr(addr);
    EXPECT_EQ(ret, "");
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_003
 * @tc.desc  : Test AudioCoreService::GetEncryptAddr()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_003, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::string addr = "12345678901234567";

    auto ret = audioCoreService->GetEncryptAddr(addr);
    EXPECT_EQ(ret, "123456**:**:**567");
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_005
 * @tc.desc  : Test AudioCoreService::ScoInputDeviceFetchedForRecongnition()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_005, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    bool handleFlag = true;
    std::string address = "abc";
    ConnectState connectState = CONNECTED;

    auto ret = audioCoreService->ScoInputDeviceFetchedForRecongnition(handleFlag, address, connectState);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_006
 * @tc.desc  : Test AudioCoreService::ScoInputDeviceFetchedForRecongnition()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_006, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    bool handleFlag = true;
    std::string address = "abc";
    ConnectState connectState = DEACTIVE_CONNECTED;

    auto ret = audioCoreService->ScoInputDeviceFetchedForRecongnition(handleFlag, address, connectState);
    EXPECT_EQ(ret, ERROR);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_007
 * @tc.desc  : Test AudioCoreService::ScoInputDeviceFetchedForRecongnition()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_007, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    bool handleFlag = false;
    std::string address = "abc";
    ConnectState connectState = DEACTIVE_CONNECTED;

    auto ret = audioCoreService->ScoInputDeviceFetchedForRecongnition(handleFlag, address, connectState);
    EXPECT_EQ(ret, ERROR);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_008
 * @tc.desc  : Test AudioCoreService::BluetoothScoFetch()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_008, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->capturerInfo_.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    streamDesc->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->BluetoothScoFetch(streamDesc);
    EXPECT_EQ(Util::IsScoSupportSource(streamDesc->capturerInfo_.sourceType), true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_009
 * @tc.desc  : Test AudioCoreService::CheckModemScene()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_009, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);

    audioCoreService->CheckModemScene(reason);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_010
 * @tc.desc  : Test AudioCoreService::CheckModemScene()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_010, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    audioCoreService->pipeManager_->modemCommunicationIdMap_.insert(std::make_pair(0, streamDesc));

    audioCoreService->CheckModemScene(reason);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_011
 * @tc.desc  : Test AudioCoreService::HandleAudioCaptureState()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_011, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_RELEASED;
    streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_012
 * @tc.desc  : Test AudioCoreService::HandleAudioCaptureState()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_012, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_STOPPED;
    streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType = SOURCE_TYPE_MIC;

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_013
 * @tc.desc  : Test AudioCoreService::HandleAudioCaptureState()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_013, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_NEW;

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_014
 * @tc.desc  : Test AudioCoreService::HandleAudioCaptureState()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_014, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioMode mode = AUDIO_MODE_PLAYBACK;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_NEW;

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_015
 * @tc.desc  : Test AudioCoreService::BluetoothDeviceFetchOutputHandle()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_015, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> desc = nullptr;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    std::string encryptMacAddr = "abc";

    auto ret = audioCoreService->BluetoothDeviceFetchOutputHandle(desc, reason, encryptMacAddr);
    EXPECT_EQ(ret, BLUETOOTH_FETCH_RESULT_ERROR);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_016
 * @tc.desc  : Test AudioCoreService::BluetoothDeviceFetchOutputHandle()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_016, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> desc = std::make_shared<AudioStreamDescriptor>();
    std::shared_ptr<AudioDeviceDescriptor> deviceDesc = std::make_shared<AudioDeviceDescriptor>();
    deviceDesc->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    desc->newDeviceDescs_.push_back(deviceDesc);
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    std::string encryptMacAddr = "abc";

    auto ret = audioCoreService->BluetoothDeviceFetchOutputHandle(desc, reason, encryptMacAddr);
    EXPECT_EQ(ret, BLUETOOTH_FETCH_RESULT_ERROR);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_017
 * @tc.desc  : Test AudioCoreService::BluetoothDeviceFetchOutputHandle()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_017, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> desc = std::make_shared<AudioStreamDescriptor>();
    std::shared_ptr<AudioDeviceDescriptor> deviceDesc = std::make_shared<AudioDeviceDescriptor>();
    deviceDesc->deviceType_ = DEVICE_TYPE_BLUETOOTH_SCO;
    desc->newDeviceDescs_.push_back(deviceDesc);
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    std::string encryptMacAddr = "abc";

    auto ret = audioCoreService->BluetoothDeviceFetchOutputHandle(desc, reason, encryptMacAddr);
    EXPECT_EQ(ret, BLUETOOTH_FETCH_RESULT_ERROR);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_018
 * @tc.desc  : Test AudioCoreService::ActivateA2dpDeviceWhenDescEnabled()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_018, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioDeviceDescriptor> desc = std::make_shared<AudioDeviceDescriptor>();
    desc->isEnable_ = true;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    auto ret = audioCoreService->ActivateA2dpDeviceWhenDescEnabled(desc, reason);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_019
 * @tc.desc  : Test AudioCoreService::ActivateA2dpDeviceWhenDescEnabled()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_019, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioDeviceDescriptor> desc = std::make_shared<AudioDeviceDescriptor>();
    desc->isEnable_ = false;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    auto ret = audioCoreService->ActivateA2dpDeviceWhenDescEnabled(desc, reason);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_020
 * @tc.desc  : Test AudioCoreService::ActivateA2dpDeviceWhenDescEnabled()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_020, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioDeviceDescriptor> desc = std::make_shared<AudioDeviceDescriptor>();
    desc->isEnable_ = false;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    auto ret = audioCoreService->ActivateA2dpDeviceWhenDescEnabled(desc, reason);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_022
 * @tc.desc  : Test AudioCoreService::LoadA2dpModule()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_022, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    DeviceType deviceType = DEVICE_TYPE_INVALID;
    AudioStreamInfo audioStreamInfo;
    std::string networkId = "abc";
    std::string sinkName = "abc";
    SourceType sourceType = SOURCE_TYPE_MIC;

    auto ret = audioCoreService->LoadA2dpModule(deviceType, audioStreamInfo, networkId, sinkName, sourceType);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_023
 * @tc.desc  : Test AudioCoreService::ReloadA2dpAudioPort()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_023, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioModuleInfo moduleInfo;
    moduleInfo.role = "abc";
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_A2DP;
    AudioStreamInfo audioStreamInfo;
    std::string networkId = "abc";
    std::string sinkName = "abc";
    SourceType sourceType = SOURCE_TYPE_MIC;

    auto ret = audioCoreService->ReloadA2dpAudioPort(moduleInfo, deviceType, audioStreamInfo,
        networkId, sinkName, sourceType);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_024
 * @tc.desc  : Test AudioCoreService::ReloadA2dpAudioPort()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_024, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioModuleInfo moduleInfo;
    moduleInfo.role = "sink";
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_A2DP_IN;
    AudioStreamInfo audioStreamInfo;
    std::string networkId = "abc";
    std::string sinkName = "abc";
    SourceType sourceType = SOURCE_TYPE_MIC;

    auto ret = audioCoreService->ReloadA2dpAudioPort(moduleInfo, deviceType, audioStreamInfo,
        networkId, sinkName, sourceType);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_025
 * @tc.desc  : Test AudioCoreService::GetA2dpModuleInfo()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_025, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioModuleInfo moduleInfo;
    moduleInfo.role = "source";
    AudioStreamInfo audioStreamInfo;
    SourceType sourceType = SOURCE_TYPE_MIC;

    audioCoreService->GetA2dpModuleInfo(moduleInfo, audioStreamInfo, sourceType);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_026
 * @tc.desc  : Test AudioCoreService::GetA2dpModuleInfo()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_026, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    AudioModuleInfo moduleInfo;
    moduleInfo.role = "sink";
    AudioStreamInfo audioStreamInfo;
    SourceType sourceType = SOURCE_TYPE_MIC;

    audioCoreService->GetA2dpModuleInfo(moduleInfo, audioStreamInfo, sourceType);
    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_027
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_027, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_NONE;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_NONE;
    deviceInfo.macAddress_ = "macAddress";
    deviceInfo.connectState_ = CONNECTED;
    deviceInfo.descriptorType_ = AudioDeviceDescriptor::AUDIO_DEVICE_DESCRIPTOR;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_028
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_028, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    deviceInfo.macAddress_ = "macAddress";
    deviceInfo.connectState_ = CONNECTED;
    deviceInfo.descriptorType_ = AudioDeviceDescriptor::DEVICE_INFO;
    deviceInfo.a2dpOffloadFlag_ = A2DP_OFFLOAD;

    audioCoreService->audioA2dpOffloadFlag_.a2dpOffloadFlag_ = NO_A2DP_DEVICE;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_029
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_029, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_INVALID;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_INVALID;
    deviceInfo.macAddress_ = "macAddress";
    deviceInfo.connectState_ = CONNECTED;
    deviceInfo.descriptorType_ = AudioDeviceDescriptor::DEVICE_INFO;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_030
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_030, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_USB_HEADSET;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;
    desc->deviceRole_ = DEVICE_ROLE_NONE;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_USB_HEADSET;
    deviceInfo.macAddress_ = "macAddress";
    deviceInfo.connectState_ = CONNECTED;
    deviceInfo.descriptorType_ = AudioDeviceDescriptor::DEVICE_INFO;
    deviceInfo.deviceRole_ = DEVICE_ROLE_NONE;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_031
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_031, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_USB_HEADSET;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_USB_HEADSET;
    deviceInfo.macAddress_ = "macAddress";
    deviceInfo.connectState_ = SUSPEND_CONNECTED;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_032
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_032, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_USB_HEADSET;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_USB_HEADSET;
    deviceInfo.macAddress_ = "abc";
    deviceInfo.connectState_ = SUSPEND_CONNECTED;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_033
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_033, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_USB_HEADSET;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "networkId";
    deviceInfo.deviceType_ = DEVICE_TYPE_DP;
    deviceInfo.macAddress_ = "abc";
    deviceInfo.connectState_ = SUSPEND_CONNECTED;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_034
 * @tc.desc  : Test AudioCoreService::IsSameDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_034, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    auto desc = std::make_shared<AudioDeviceDescriptor>();
    desc->networkId_ = "networkId";
    desc->deviceType_ = DEVICE_TYPE_USB_HEADSET;
    desc->macAddress_ = "macAddress";
    desc->connectState_ = CONNECTED;

    AudioDeviceDescriptor deviceInfo;
    deviceInfo.networkId_ = "abc";
    deviceInfo.deviceType_ = DEVICE_TYPE_DP;
    deviceInfo.macAddress_ = "abc";
    deviceInfo.connectState_ = SUSPEND_CONNECTED;

    auto ret = audioCoreService->IsSameDevice(desc, deviceInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_037
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_037, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_NEW;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    uint32_t flag = 0;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeNew(pipeInfo, flag, reason);
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_038
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_038, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_MOVE;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    uint32_t flag = 0;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeNew(pipeInfo, flag, reason);
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_039
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_039, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_RECREATE;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    uint32_t flag = 0;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeNew(pipeInfo, flag, reason);
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_040
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_040, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    EXPECT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    uint32_t flag = 0;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeNew(pipeInfo, flag, reason);
    EXPECT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_043
 * @tc.desc  : Test AudioCoreService::BluetoothScoFetch
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_043, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    streamDesc->newDeviceDescs_.push_back(audioDeviceDescriptor);

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->BluetoothScoFetch(streamDesc);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_044
 * @tc.desc : Test AudioCoreService::HandleAudioCaptureState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_044, TestSize.Level1)
{
    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_RELEASED;
    streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_045
 * @tc.desc : Test AudioCoreService::HandleAudioCaptureState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_045, TestSize.Level1)
{
    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_RELEASED;
    streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType = SOURCE_TYPE_PLAYBACK_CAPTURE;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_046
 * @tc.desc : Test AudioCoreService::HandleAudioCaptureState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_046, TestSize.Level1)
{
    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_STOPPED;
    streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_047
 * @tc.desc : Test AudioCoreService::HandleAudioCaptureState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_047, TestSize.Level1)
{
    AudioMode mode = AUDIO_MODE_RECORD;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_INVALID;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_048
 * @tc.desc : Test AudioCoreService::HandleAudioCaptureState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_048, TestSize.Level1)
{
    AudioMode mode = AUDIO_MODE_PLAYBACK;
    AudioStreamChangeInfo streamChangeInfo;
    streamChangeInfo.audioCapturerChangeInfo.capturerState = CAPTURER_STOPPED;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->HandleAudioCaptureState(mode, streamChangeInfo);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_049
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_049, TestSize.Level1)
{
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(DEVICE_TYPE_INVALID, true);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_050
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_050, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_EARPIECE;

    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_051
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_051, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_SPEAKER;

    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_052
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_052, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_WIRED_HEADSET;

    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_053
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_053, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_WIRED_HEADPHONES;

    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_054
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_054, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_USB_HEADSET;
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_055
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_055, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_DP;
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_056
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_056, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_057
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_057, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_BLUETOOTH_A2DP;
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_058
 * @tc.desc : Test AudioCoreService::HasLowLatencyCapability
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_058, TestSize.Level1)
{
    DeviceType deviceType = DEVICE_TYPE_DEFAULT;
    auto audioCoreService = AudioCoreService::GetCoreService();

    bool result = audioCoreService->HasLowLatencyCapability(deviceType, false);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_059
 * @tc.desc : Test AudioCoreService::GetRealUid
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_059, TestSize.Level1)
{
    auto streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->callerUid_ = 1013;
    streamDesc->appInfo_.appUid = 1013;

    auto audioCoreService = AudioCoreService::GetCoreService();

    int32_t result = audioCoreService->GetRealUid(streamDesc);

    EXPECT_EQ(result, 1013);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_060
 * @tc.desc : Test AudioCoreService::GetRealUid
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_060, TestSize.Level1)
{
    auto streamDesc = std::make_shared<AudioStreamDescriptor>();
    auto audioCoreService = AudioCoreService::GetCoreService();

    streamDesc->callerUid_ = 0;

    int32_t result = audioCoreService->GetRealUid(streamDesc);

    EXPECT_EQ(result, 0);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_061
 * @tc.desc : Test AudioCoreService::UpdateRendererInfoWhenNoPermission
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_061, TestSize.Level1)
{
    auto audioRendererChangeInfos = std::make_shared<AudioRendererChangeInfo>();
    bool hasSystemPermission = true;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->UpdateRendererInfoWhenNoPermission(audioRendererChangeInfos, hasSystemPermission);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_062
 * @tc.desc : Test AudioCoreService::UpdateRendererInfoWhenNoPermission
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_062, TestSize.Level1)
{
    auto audioRendererChangeInfos = std::make_shared<AudioRendererChangeInfo>();
    bool hasSystemPermission = false;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->UpdateRendererInfoWhenNoPermission(audioRendererChangeInfos, hasSystemPermission);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_063
 * @tc.desc : Test AudioCoreService::UpdateRendererInfoWhenNoPermission
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_063, TestSize.Level1)
{
    auto audioCapturerChangeInfos = std::make_shared<AudioCapturerChangeInfo>();
    bool hasSystemPermission = true;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->UpdateCapturerInfoWhenNoPermission(audioCapturerChangeInfos, hasSystemPermission);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_064
 * @tc.desc : Test AudioCoreService::UpdateRendererInfoWhenNoPermission
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_064, TestSize.Level1)
{
    auto audioCapturerChangeInfos = std::make_shared<AudioCapturerChangeInfo>();
    bool hasSystemPermission = false;

    auto audioCoreService = AudioCoreService::GetCoreService();

    audioCoreService->UpdateCapturerInfoWhenNoPermission(audioCapturerChangeInfos, hasSystemPermission);

    EXPECT_NE(audioCoreService, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_065
 * @tc.desc : Test AudioCoreService::GetFastControlParam
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_065, TestSize.Level1)
{
    auto audioCoreService = AudioCoreService::GetCoreService();
    audioCoreService->isFastControlled_ = true;

    SetSysPara("persist.multimedia.audioflag.fastcontrolled", 0);

    bool result = audioCoreService->GetFastControlParam();

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_066
 * @tc.desc : Test AudioCoreService::GetFastControlParam
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_066, TestSize.Level1)
{
    auto audioCoreService = AudioCoreService::GetCoreService();
    audioCoreService->isFastControlled_ = true;

    SetSysPara("persist.multimedia.audioflag.fastcontrolled", 1);

    bool result = audioCoreService->GetFastControlParam();

    EXPECT_EQ(result, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_067
 * @tc.desc : Test AudioCoreService::NeedRehandleA2DPDevice
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_067, TestSize.Level1)
{
    auto desc = std::make_shared<AudioDeviceDescriptor>();

    auto audioCoreService = AudioCoreService::GetCoreService();
    desc->deviceType_ = DEVICE_TYPE_BLUETOOTH_SCO;

    bool result = audioCoreService->NeedRehandleA2DPDevice(desc);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_068
 * @tc.desc : Test AudioCoreService::NeedRehandleA2DPDevice
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_068, TestSize.Level1)
{
    auto desc = std::make_shared<AudioDeviceDescriptor>();

    auto audioCoreService = AudioCoreService::GetCoreService();
    desc->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    std::string moduleName = BLUETOOTH_SPEAKER;
    AudioIOHandle moduleId = 0;

    audioCoreService->audioIOHandleMap_.AddIOHandleInfo(moduleName, moduleId);
    bool result = audioCoreService->NeedRehandleA2DPDevice(desc);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_069
 * @tc.desc : Test AudioCoreService::NeedRehandleA2DPDevice
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_069, TestSize.Level1)
{
    auto desc = std::make_shared<AudioDeviceDescriptor>();

    auto audioCoreService = AudioCoreService::GetCoreService();
    desc->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;
    std::string moduleName = BLUETOOTH_MIC;
    AudioIOHandle moduleId = 0;

    audioCoreService->audioIOHandleMap_.AddIOHandleInfo(moduleName, moduleId);
    bool result = audioCoreService->NeedRehandleA2DPDevice(desc);

    EXPECT_EQ(result, false);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_070
 * @tc.desc : Test AudioCoreService::TriggerRecreateRendererStreamCallback
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_070, TestSize.Level1)
{
    int32_t callerPid = 0;
    int32_t sessionId = 0;
    uint32_t routeFlag = true;
    AudioStreamDeviceChangeReasonExt::ExtEnum reason =
        AudioStreamDeviceChangeReasonExt::ExtEnum::SET_DEFAULT_OUTPUT_DEVICE;

    auto audioCoreService = AudioCoreService::GetCoreService();
    audioCoreService->SetCallbackHandler(nullptr);

    audioCoreService->TriggerRecreateRendererStreamCallback(callerPid, sessionId, routeFlag, reason);

    EXPECT_EQ(audioCoreService->audioPolicyServerHandler_, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_071
 * @tc.desc : Test AudioCoreService::TriggerRecreateRendererStreamCallback
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_071, TestSize.Level1)
{
    int32_t callerPid = 0;
    int32_t sessionId = 0;
    uint32_t routeFlag = true;
    AudioStreamDeviceChangeReasonExt::ExtEnum reason =
        AudioStreamDeviceChangeReasonExt::ExtEnum::SET_DEFAULT_OUTPUT_DEVICE;

    auto audioCoreService = AudioCoreService::GetCoreService();
    std::shared_ptr<AudioPolicyServerHandler> handler = std::make_shared<AudioPolicyServerHandler>();
    audioCoreService->SetCallbackHandler(handler);

    audioCoreService->TriggerRecreateRendererStreamCallback(callerPid, sessionId, routeFlag, reason);

    EXPECT_NE(audioCoreService->audioPolicyServerHandler_, nullptr);
}
/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_072
 * @tc.desc : Test AudioCoreService::TriggerRecreateCapturerStreamCallback
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_072, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->sessionId_ = 0,
    streamDesc->callerUid_ = 0;
    streamDesc->appInfo_.appUid = 0;
    streamDesc->appInfo_.appPid = 0;
    streamDesc->appInfo_.appTokenId = 0;
    streamDesc->streamStatus_ = STREAM_STATUS_NEW;
    streamDesc->routeFlag_ = true;

    auto audioCoreService = AudioCoreService::GetCoreService();
    audioCoreService->SetCallbackHandler(nullptr);

    audioCoreService->TriggerRecreateCapturerStreamCallback(streamDesc);

    EXPECT_EQ(audioCoreService->audioPolicyServerHandler_, nullptr);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_073
 * @tc.desc : Test AudioCoreService::TriggerRecreateCapturerStreamCallback
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_073, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->sessionId_ = 123456,
    streamDesc->callerUid_ = 0;
    streamDesc->appInfo_.appUid = 0;
    streamDesc->appInfo_.appPid = 0;
    streamDesc->appInfo_.appTokenId = 0;
    streamDesc->streamStatus_ = STREAM_STATUS_NEW;
    streamDesc->routeFlag_ = true;

    auto audioCoreService = AudioCoreService::GetCoreService();
    std::shared_ptr<AudioPolicyServerHandler> handler = std::make_shared<AudioPolicyServerHandler>();
    audioCoreService->SetCallbackHandler(handler);
    
    audioCoreService->TriggerRecreateCapturerStreamCallback(streamDesc);
    EXPECT_NE(audioCoreService->audioPolicyServerHandler_, nullptr);

    bool ret = SwitchStreamUtil::RemoveAllRecordBySessionId(123456);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_074
 * @tc.desc : Test AudioCoreService::HandleStreamStatusToCapturerState
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_074, TestSize.Level2)
{
    auto audioCoreService = AudioCoreService::GetCoreService();
    
    CapturerState state = audioCoreService->HandleStreamStatusToCapturerState(STREAM_STATUS_NEW);
    EXPECT_EQ(state, CAPTURER_PREPARED);

    state = audioCoreService->HandleStreamStatusToCapturerState(STREAM_STATUS_STARTED);
    EXPECT_EQ(state, CAPTURER_RUNNING);

    state = audioCoreService->HandleStreamStatusToCapturerState(STREAM_STATUS_PAUSED);
    EXPECT_EQ(state, CAPTURER_PAUSED);

    state = audioCoreService->HandleStreamStatusToCapturerState(STREAM_STATUS_STOPPED);
    EXPECT_EQ(state, CAPTURER_STOPPED);

    state = audioCoreService->HandleStreamStatusToCapturerState(STREAM_STATUS_RELEASED);
    EXPECT_EQ(state, CAPTURER_RELEASED);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_100
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_100, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_NEW;
    audioStreamDescriptor->routeFlag_ = AUDIO_OUTPUT_FLAG_DIRECT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    uint32_t flag = 0;
    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);
    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeUpdate(pipeInfo, flag, reason);
    EXPECT_EQ(flag, AUDIO_OUTPUT_FLAG_DIRECT);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_101
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_101, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeUpdate(pipeInfo, flag, reason);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_102
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_102, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_RECREATE;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    pipeInfo->moduleInfo_.name = BLUETOOTH_MIC;

    uint32_t flag = 0;

    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeUpdate(pipeInfo, flag, reason);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_103
 * @tc.desc  : Test AudioCoreService::ProcessOutputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_103, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = static_cast<AudioStreamAction>(5);
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);
    pipeInfo->moduleInfo_.name = OFFLOAD_PRIMARY_SPEAKER;

    uint32_t flag = 0;

    AudioStreamDeviceChangeReasonExt::ExtEnum extEnum = AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN;
    AudioStreamDeviceChangeReasonExt reason(extEnum);

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessOutputPipeUpdate(pipeInfo, flag, reason);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_104
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_104, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_NEW;
    audioStreamDescriptor->routeFlag_ = AUDIO_OUTPUT_FLAG_DIRECT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeNew(pipeInfo, flag);
    EXPECT_EQ(flag, AUDIO_OUTPUT_FLAG_DIRECT);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_105
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_105, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeNew(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_106
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_106, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_RECREATE;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeNew(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_107
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeNew()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_107, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = static_cast<AudioStreamAction>(5);
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeNew(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_108
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_108, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_NEW;
    audioStreamDescriptor->routeFlag_ = AUDIO_OUTPUT_FLAG_DIRECT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeUpdate(pipeInfo, flag);
    EXPECT_EQ(flag, AUDIO_OUTPUT_FLAG_DIRECT);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_109
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_109, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeUpdate(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_110
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_110, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = AUDIO_STREAM_ACTION_RECREATE;
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeUpdate(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_111
 * @tc.desc  : Test AudioCoreService::ProcessInputPipeUpdate()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_111, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    std::shared_ptr<AudioStreamDescriptor> audioStreamDescriptor = std::make_shared<AudioStreamDescriptor>();
    audioStreamDescriptor->streamAction_ = static_cast<AudioStreamAction>(5);
    pipeInfo->streamDescriptors_.push_back(audioStreamDescriptor);

    uint32_t flag = 0;

    audioCoreService->pipeManager_ = std::make_shared<AudioPipeManager>();

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    audioStreamDescriptor->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->ProcessInputPipeUpdate(pipeInfo, flag);
    ASSERT_NE(audioCoreService->pipeManager_, nullptr);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_112
 * @tc.desc  : Test AudioCoreService::SwitchActiveA2dpDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_112, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    auto deviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    ASSERT_NE(deviceDescriptor, nullptr);

    A2dpDeviceConfigInfo a2dpDeviceConfigInfo;
    audioCoreService->audioA2dpDevice_.connectedA2dpDeviceMap_.insert({"00:00:00:00:00:00", a2dpDeviceConfigInfo});
    deviceDescriptor->macAddress_ = "00:00:00:00:00:00";
    AudioIOHandle audioIOHandle;
    audioCoreService->audioIOHandleMap_.IOHandles_.insert({BLUETOOTH_SPEAKER, audioIOHandle});

    auto ret = audioCoreService->SwitchActiveA2dpDevice(deviceDescriptor);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_113
 * @tc.desc  : Test AudioCoreService::SwitchActiveA2dpDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_113, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    auto deviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    ASSERT_NE(deviceDescriptor, nullptr);

    A2dpDeviceConfigInfo a2dpDeviceConfigInfo;
    audioCoreService->audioA2dpDevice_.connectedA2dpDeviceMap_.insert({"00:00:00:00:00:00", a2dpDeviceConfigInfo});
    deviceDescriptor->macAddress_ = "00:00:00:00:00:00";
    AudioIOHandle audioIOHandle;
    audioCoreService->audioIOHandleMap_.IOHandles_.insert({"abc", audioIOHandle});

    auto ret = audioCoreService->SwitchActiveA2dpDevice(deviceDescriptor);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_114
 * @tc.desc  : Test AudioCoreService::SwitchActiveA2dpDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_114, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    auto deviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    ASSERT_NE(deviceDescriptor, nullptr);

    A2dpDeviceConfigInfo a2dpDeviceConfigInfo;
    audioCoreService->audioA2dpDevice_.connectedA2dpDeviceMap_.insert({"abc", a2dpDeviceConfigInfo});
    deviceDescriptor->macAddress_ = "abc";
    AudioIOHandle audioIOHandle;
    audioCoreService->audioIOHandleMap_.IOHandles_.insert({BLUETOOTH_SPEAKER, audioIOHandle});

    auto ret = audioCoreService->SwitchActiveA2dpDevice(deviceDescriptor);
    EXPECT_NE(Bluetooth::AudioA2dpManager::GetActiveA2dpDevice(), "00:00:00:00:00:00");
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_115
 * @tc.desc  : Test AudioCoreService::MoveToNewInputDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_115, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);

    streamDesc->oldDeviceDescs_.clear();
    EXPECT_EQ(streamDesc->oldDeviceDescs_.size(), 0);
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    streamDesc->newDeviceDescs_.push_back(audioDeviceDescriptor);

    audioCoreService->MoveToNewInputDevice(streamDesc);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_116
 * @tc.desc  : Test AudioCoreService::MoveToNewInputDevice()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_116, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);

    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>();
    streamDesc->oldDeviceDescs_.push_back(audioDeviceDescriptor);
    EXPECT_NE(streamDesc->oldDeviceDescs_.size(), 0);
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor2 = std::make_shared<AudioDeviceDescriptor>();
    streamDesc->newDeviceDescs_.push_back(audioDeviceDescriptor2);

    audioCoreService->MoveToNewInputDevice(streamDesc);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_117
 * @tc.desc  : Test AudioCoreService::IsNewDevicePlaybackSupported()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_117, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = nullptr;
    bool ret = audioCoreService->IsNewDevicePlaybackSupported(streamDesc);
    EXPECT_EQ(ret, false);

    streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);
    ret = audioCoreService->IsNewDevicePlaybackSupported(streamDesc);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_118
 * @tc.desc  : Test AudioCoreService::IsNewDevicePlaybackSupported()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_118, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);
    streamDesc->newDeviceDescs_.push_back(nullptr);
    bool ret = audioCoreService->IsNewDevicePlaybackSupported(streamDesc);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_119
 * @tc.desc  : Test AudioCoreService::IsNewDevicePlaybackSupported()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_119, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = std::make_shared<AudioDeviceDescriptor>();
    ASSERT_NE(newDeviceDesc, nullptr);
    newDeviceDesc->deviceType_ = DEVICE_TYPE_SPEAKER;
    streamDesc->newDeviceDescs_.push_back(newDeviceDesc);
    streamDesc->streamInfo_.encoding = ENCODING_EAC3;

    bool ret = audioCoreService->IsNewDevicePlaybackSupported(streamDesc);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_120
 * @tc.desc  : Test AudioCoreService::IsNewDevicePlaybackSupported()
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_120, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    ASSERT_NE(streamDesc, nullptr);
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = std::make_shared<AudioDeviceDescriptor>();
    ASSERT_NE(newDeviceDesc, nullptr);
    newDeviceDesc->deviceType_ = DEVICE_TYPE_HDMI;
    streamDesc->newDeviceDescs_.push_back(newDeviceDesc);
    streamDesc->streamInfo_.encoding = ENCODING_EAC3;

    bool ret = audioCoreService->IsNewDevicePlaybackSupported(streamDesc);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: AudioCoreServicePrivate_121
 * @tc.desc  : Test AudioCoreService::UpdateInputDeviceWhenStopping
 */
HWTEST(AudioCoreServicePrivateTest, AudioCoreServicePrivate_121, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);

    int32_t uid = getuid();
    std::vector<uint32_t> sessionIDSet  = audioCoreService->streamCollector_.GetAllCapturerSessionIDForUID(uid);
    std::shared_ptr<AudioDeviceDescriptor> device;

    for (const auto &sessionID : sessionIDSet) {
        audioCoreService->audioDeviceManager_.SetInputDevice(DEVICE_TYPE_MIC, sessionID, SOURCE_TYPE_MIC, 1);
        device = audioCoreService->audioDeviceManager_.GetSelectedCaptureDevice(sessionID);
        EXPECT_EQ(device->deviceType_ == DEVICE_TYPE_MIC, true);
    }

    audioCoreService->UpdateInputDeviceWhenStopping(uid);

    for (const auto &sessionID : sessionIDSet) {
        device = audioCoreService->audioDeviceManager_.GetSelectedCaptureDevice(sessionID);
        EXPECT_EQ(device->deviceType_ != DEVICE_TYPE_MIC, true);
    }
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: IsFastAllowedTest_001
 * @tc.desc  : Test AudioCoreService::IsFastAllowed, return true when bundleName is null.
 */
HWTEST(AudioCoreServicePrivateTest, IsFastAllowedTest_001, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);
    std::string bundleName = "";
    EXPECT_EQ(audioCoreService->IsFastAllowed(bundleName), true);
}

/**
 * @tc.name  : Test AudioCoreService.
 * @tc.number: IsFastAllowedTest_002
 * @tc.desc  : Test AudioCoreService::IsFastAllowed, return true when bundleName is normal app.
 */
HWTEST(AudioCoreServicePrivateTest, IsFastAllowedTest_002, TestSize.Level1)
{
    auto audioCoreService = std::make_shared<AudioCoreService>();
    ASSERT_NE(audioCoreService, nullptr);
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    std::string bundleName = "com.example.app";
    streamDesc->SetBunduleName(bundleName);
    EXPECT_EQ(audioCoreService->IsFastAllowed(streamDesc->bundleName_), true);
}
} // namespace AudioStandard
} // namespace OHOS