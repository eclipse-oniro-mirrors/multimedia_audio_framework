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

#include "audio_capturer_session_unit_test.h"
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_001
 * @tc.desc  : Test AudioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_001, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = false;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_002
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_002, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_003
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_003, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_WAKEUP;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_004
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_004, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_MIC;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_005
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_005, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_MIC;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_006
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_006, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_007
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_007, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "ABC"}, {"voip_up", "ABC"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_008
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_008, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "ABC"}, {"voip_up", "PNR"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_009
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_009, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEnhancePropertyArray oldPropertyArray;
    AudioEnhancePropertyArray newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_010
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_010, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEffectPropertyArrayV3 oldPropertyArray;
    AudioEffectPropertyArrayV3 newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_011
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_011, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEffectPropertyArrayV3 oldPropertyArray;
    AudioEffectPropertyArrayV3 newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "ABC"}, {"voip_up", "PNR"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_012
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForEffect()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_012, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioEffectPropertyArrayV3 oldPropertyArray;
    AudioEffectPropertyArrayV3 newPropertyArray;

    oldPropertyArray.property = {{"record", "PNR"}, {"voip_up", "PNR"}};
    newPropertyArray.property = {{"record", "ABC"}, {"voip_up", "ABC"}};

    audioCapturerSession->audioEcManager_.isMicRefFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForEffect(oldPropertyArray, newPropertyArray);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_013
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForDeviceChange()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_013, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    AudioDeviceDescriptor outputDevice;
    std::string caller;

    audioCapturerSession->audioEcManager_.isEcFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_WAKEUP;

    audioCapturerSession->ReloadSourceForDeviceChange(inputDevice, outputDevice, caller);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_014
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForDeviceChange()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_014, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    AudioDeviceDescriptor outputDevice;
    std::string caller;

    audioCapturerSession->audioEcManager_.isEcFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_MIC;

    audioCapturerSession->ReloadSourceForDeviceChange(inputDevice, outputDevice, caller);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_015
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForDeviceChange()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_015, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    AudioDeviceDescriptor outputDevice;
    std::string caller;

    audioCapturerSession->audioEcManager_.isEcFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForDeviceChange(inputDevice, outputDevice, caller);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_016
 * @tc.desc  : Test udioCapturerSession::ReloadSourceForDeviceChange()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_016, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    inputDevice.deviceType_ = DEVICE_TYPE_DEFAULT;
    AudioDeviceDescriptor outputDevice;
    std::string caller;

    audioCapturerSession->audioEcManager_.isEcFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_VOICE_COMMUNICATION;

    audioCapturerSession->ReloadSourceForDeviceChange(inputDevice, outputDevice, caller);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_017
 * @tc.desc  : Test AudioCapturerSession::IsVoipDeviceChanged()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_017, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    AudioDeviceDescriptor outputDevice;

    auto ret = audioCapturerSession->IsVoipDeviceChanged(inputDevice, outputDevice);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_018
 * @tc.desc  : Test AudioCapturerSession::FillWakeupStreamPropInfo()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_018, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioStreamInfo streamInfo;
    std::shared_ptr<AdapterPipeInfo> pipeInfo = nullptr;
    AudioModuleInfo audioModuleInfo;

    auto ret = audioCapturerSession->FillWakeupStreamPropInfo(streamInfo, pipeInfo, audioModuleInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_019
 * @tc.desc  : Test AudioCapturerSession::FillWakeupStreamPropInfo()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_019, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioStreamInfo streamInfo;
    std::shared_ptr<AdapterPipeInfo> pipeInfo = std::make_shared<AdapterPipeInfo>();
    EXPECT_NE(pipeInfo, nullptr);
    AudioModuleInfo audioModuleInfo;

    auto ret = audioCapturerSession->FillWakeupStreamPropInfo(streamInfo, pipeInfo, audioModuleInfo);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_020
 * @tc.desc  : Test AudioCapturerSession::GetInstance()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_020, TestSize.Level1)
{
    shared_ptr<AudioA2dpOffloadManager> audioA2dpOffloadManager = nullptr;
    {
        auto& audioCapturerSession = AudioCapturerSession::GetInstance();

        audioA2dpOffloadManager = make_shared<AudioA2dpOffloadManager>();
        audioCapturerSession.Init(audioA2dpOffloadManager);
        audioCapturerSession.SetConfigParserFlag();

        audioCapturerSession.DeInit();
    }
    EXPECT_EQ(audioA2dpOffloadManager.use_count(), 1);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_021
 * @tc.desc  : Test AudioCapturerSession::OnCapturerSessionAdded()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_021, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    SessionInfo sessionInfo;
    sessionInfo.sourceType = SOURCE_TYPE_MIC;
    sessionInfo.rate = 44100;
    sessionInfo.channels = 2;
    AudioStreamInfo streamInfo;
    audioCapturerSession->SetConfigParserFlag();

    auto &audioVolumeManager = AudioVolumeManager::GetInstance();
    audioVolumeManager.SetDefaultDeviceLoadFlag(true);


    uint64_t sessionID = 1;
    audioCapturerSession->OnCapturerSessionRemoved(sessionID);

    auto ret = audioCapturerSession->OnCapturerSessionAdded(sessionID, sessionInfo, streamInfo);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_022
 * @tc.desc  : Test AudioCapturerSession::OnCapturerSessionAdded()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_022, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    SessionInfo sessionInfo;
    AudioStreamInfo streamInfo;

    uint64_t sessionID = 1;

    auto ret = audioCapturerSession->OnCapturerSessionAdded(sessionID, sessionInfo, streamInfo);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_023
 * @tc.desc  : Test AudioCapturerSession::SetWakeUpAudioCapturerFromAudioServer()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_023, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioStreamInfo streamInfo;
    AudioProcessConfig config;
    config.streamInfo = streamInfo;

    auto ret = audioCapturerSession->SetWakeUpAudioCapturerFromAudioServer(config);
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_024
 * @tc.desc  : Test AudioCapturerSession::CloseWakeUpAudioCapturer()
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_024, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    auto ret = audioCapturerSession->CloseWakeUpAudioCapturer();
    EXPECT_NE(ret, SUCCESS);
}

/**
 * @tc.name  : Test AudioCapturerSession.
 * @tc.number: AudioCapturerSession_025
 * @tc.desc  : Test ReloadSourceForDeviceChange() for valid source and device
 */
HWTEST(AudioCapturerSessionTest, AudioCapturerSession_025, TestSize.Level1)
{
    auto audioCapturerSession = std::make_shared<AudioCapturerSession>();
    EXPECT_NE(audioCapturerSession, nullptr);

    AudioDeviceDescriptor inputDevice;
    inputDevice.deviceType_ = DEVICE_TYPE_MIC;
    AudioDeviceDescriptor outputDevice;
    std::string caller = "testCase";

    const uint64_t testSessionId = 99;
    audioCapturerSession->audioEcManager_.isEcFeatureEnable_ = true;
    audioCapturerSession->audioEcManager_.normalSourceOpened_ = SOURCE_TYPE_MIC;
    audioCapturerSession->audioEcManager_.sessionIdUsedToOpenSource_ = testSessionId;

    audioCapturerSession->ReloadSourceForDeviceChange(inputDevice, outputDevice, caller);
    EXPECT_EQ(audioCapturerSession->audioEcManager_.GetOpenedNormalSourceSessionId(), testSessionId);
}
} // namespace AudioStandard
} // namespace OHOS
