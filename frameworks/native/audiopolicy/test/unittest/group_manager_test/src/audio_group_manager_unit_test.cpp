/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "audio_group_manager_unit_test.h"

#include "audio_errors.h"
#include "audio_info.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
namespace {
    constexpr int32_t MAX_VOL = 15;
    constexpr int32_t MIN_VOL = 0;
    std::string networkId = "LocalDevice";
    constexpr int32_t ERROR_62980101 = -62980101;
}

void AudioGroupManagerUnitTest::SetUpTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}
void AudioGroupManagerUnitTest::TearDownTestCase(void)
{
    system("param set debug.media_service.histreamer 0");
}
void AudioGroupManagerUnitTest::SetUp(void) {}
void AudioGroupManagerUnitTest::TearDown(void) {}



/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_001
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_001, TestSize.Level1)
{
    int32_t volume = 0;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALL, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALL);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ALL, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ALL, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_002
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_002, TestSize.Level1)
{
    int32_t volume = 2;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALARM, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ALARM, mute);
        EXPECT_EQ(SUCCESS, ret);

        // stream alarm can not set mute
        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ALARM, isMute);
        EXPECT_EQ(false, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_003
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_003, TestSize.Level1)
{
    int32_t volume = 4;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ACCESSIBILITY, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ACCESSIBILITY, mute);
        EXPECT_EQ(SUCCESS, ret);

        // stream accessibility can not set mute
        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ACCESSIBILITY, isMute);
        EXPECT_EQ(false, isMute);
    }
}

/**
* @tc.name  : Test AudioVolume API
* @tc.number: AudioVolume_004
* @tc.desc  : Test AudioVolume manager interface multiple requests
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, AudioVolume_004, TestSize.Level1)
{
    int32_t volume = 5;
    bool mute = true;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ULTRASONIC, volume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);
        EXPECT_EQ(volume, ret);

        ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_ULTRASONIC, mute);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_ULTRASONIC, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_001
* @tc.desc  : Test setting volume of ringtone stream with max volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, MAX_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t volume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MAX_VOL, volume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_002
* @tc.desc  : Test setting volume of ringtone stream with min volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, MIN_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t volume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, volume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_003
* @tc.desc  : Test setting volume of media stream with max volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_MUSIC, MAX_VOL);
        EXPECT_EQ(SUCCESS, ret);

        int32_t mediaVol = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MAX_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_004
* @tc.desc  : Test setting volume of alarm stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_004, TestSize.Level0)
{
    int32_t ErrorVolume = 17;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ALARM, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ALARM);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_005
* @tc.desc  : Test setting volume of accessibility stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_005, TestSize.Level0)
{
    int32_t ErrorVolume = 18;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ACCESSIBILITY, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ACCESSIBILITY);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test SetVolume API
* @tc.number: SetVolumeTest_006
* @tc.desc  : Test setting volume of ultrasonic stream with error volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetVolumeTest_006, TestSize.Level0)
{
    int32_t ErrorVolume = -5;
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t FirstVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);

        auto ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_ULTRASONIC, ErrorVolume);
        EXPECT_NE(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_ULTRASONIC);
        EXPECT_EQ(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test GetMaxVolume API
* @tc.number: GetMaxVolumeTest_001
* @tc.desc  : Test GetMaxVolume of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, GetMaxVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
    int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t mediaVol = audioGroupMngr_->GetMaxVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MAX_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetMaxVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MAX_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test GetMaxVolume API
* @tc.number: GetMinVolumeTest_001
* @tc.desc  : Test GetMaxVolume of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, GetMinVolumeTest_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        int32_t mediaVol = audioGroupMngr_->GetMinVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(MIN_VOL, mediaVol);

        int32_t ringVolume = audioGroupMngr_->GetMinVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(MIN_VOL, ringVolume);
    }
}

/**
* @tc.name  : Test SetMute API
* @tc.number: SetMute_001
* @tc.desc  : Test mute functionality of ringtone stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_RING, true);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_002
* @tc.desc  : Test unmute functionality of ringtone stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_RING, false);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(false, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_003
* @tc.desc  : Test mute functionality of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_MUSIC, true);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_MUSIC, isMute);
        EXPECT_EQ(true, isMute);
    }
}

/**
* @tc.name  : Test SetMute IsStreamMute API
* @tc.number: SetMute_004
* @tc.desc  : Test unmute functionality of media stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, SetMute_004, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->SetMute(AudioVolumeType::STREAM_MUSIC, false);
        EXPECT_EQ(SUCCESS, ret);

        bool isMute;
        ret = audioGroupMngr_->IsStreamMute(AudioVolumeType::STREAM_RING, isMute);
        EXPECT_EQ(false, isMute);
    }
}

/**
* @tc.name  : Test IsVolumeUnadjustable API
* @tc.number: Audio_Group_Manager_IsVolumeUnadjustable_001
* @tc.desc  : Test volume is unadjustable or adjustable functionality
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_IsVolumeUnadjustable_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        auto ret = audioGroupMngr_->IsVolumeUnadjustable();
        GTEST_LOG_(INFO) << "Is volume unadjustable: " << ret;
        EXPECT_EQ(false, ret);
    }
}

/**
* @tc.name  : Test AdjustVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustVolumeByStep_001
* @tc.desc  : Test adjust volume to up by step functionality
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustVolumeByStep_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
        ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_MUSIC, 7);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->AdjustVolumeByStep(VolumeAdjustType::VOLUME_ADJUST_UP);
        GTEST_LOG_(INFO) << "Adjust volume by step: " << ret;
        EXPECT_EQ(SUCCESS, ret);
    }
}

/**
* @tc.name  : Test AdjustVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustVolumeByStep_002
* @tc.desc  : Test adjust volume to down by step functionality
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustVolumeByStep_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        ret = audioGroupMngr_->AdjustVolumeByStep(VolumeAdjustType::VOLUME_ADJUST_DOWN);
        GTEST_LOG_(INFO) << "Adjust volume by step: " << ret;
        EXPECT_EQ(SUCCESS, ret);
    }
}

/**
* @tc.name  : Test AdjustSystemVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustSystemVolumeByStep_001
* @tc.desc  : Test adjust system volume by step to up of STREAM_RECORDING stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustSystemVolumeByStep_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t FirstVolume = 7;
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
        ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, FirstVolume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->AdjustSystemVolumeByStep(AudioVolumeType::STREAM_RING, VolumeAdjustType::VOLUME_ADJUST_UP);
        GTEST_LOG_(INFO) << "Adjust system volume by step: " << ret;
        EXPECT_EQ(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_GT(SecondVolume, FirstVolume);
    }
}

/**
* @tc.name  : Test AdjustSystemVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustSystemVolumeByStep_002
* @tc.desc  : Test adjust system volume by step to down of STREAM_RECORDING stream
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustSystemVolumeByStep_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t FirstVolume = 7;
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
        ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, FirstVolume);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->AdjustSystemVolumeByStep(AudioVolumeType::STREAM_RING, VolumeAdjustType::VOLUME_ADJUST_DOWN);
        GTEST_LOG_(INFO) << "Adjust system volume by step: " << ret;
        EXPECT_EQ(SUCCESS, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_GT(FirstVolume, SecondVolume);
    }
}

/**
* @tc.name  : Test AdjustSystemVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustSystemVolumeByStep_003
* @tc.desc  : Test adjust system volume by step to up of STREAM_RING stream when is max volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustSystemVolumeByStep_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
        auto maxVol = audioGroupMngr_->GetMaxVolume(AudioVolumeType::STREAM_RING);
        ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_RING, maxVol);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->AdjustSystemVolumeByStep(AudioVolumeType::STREAM_RING, VolumeAdjustType::VOLUME_ADJUST_UP);
        GTEST_LOG_(INFO) << "Adjust system volume by step: " << ret;
        EXPECT_EQ(ERROR_62980101, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_RING);
        EXPECT_EQ(maxVol, SecondVolume);
    }
}

/**
* @tc.name  : Test AdjustSystemVolumeByStep API
* @tc.number: Audio_Group_Manager_AdjustSystemVolumeByStep_003
* @tc.desc  : Test adjust system volume by step to down of STREAM_MUSIC stream when is min volume
* @tc.require: issueI5M1XV
*/
HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_AdjustSystemVolumeByStep_004, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int32_t ret = -1;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);
        auto minVol = audioGroupMngr_->GetMinVolume(AudioVolumeType::STREAM_MUSIC);
        ret = audioGroupMngr_->SetVolume(AudioVolumeType::STREAM_MUSIC, minVol);
        EXPECT_EQ(SUCCESS, ret);

        ret = audioGroupMngr_->AdjustSystemVolumeByStep(AudioVolumeType::STREAM_MUSIC, VolumeAdjustType::VOLUME_ADJUST_DOWN);
        GTEST_LOG_(INFO) << "Adjust system volume by step: " << ret;
        EXPECT_EQ(ERROR_62980101, ret);

        int32_t SecondVolume = audioGroupMngr_->GetVolume(AudioVolumeType::STREAM_MUSIC);
        EXPECT_EQ(minVol, SecondVolume);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_001, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_ACCESSIBILITY, 3, DeviceType::DEVICE_TYPE_SPEAKER);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_002, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_ALARM, vol, DeviceType::DEVICE_TYPE_SPEAKER);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_003, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_VOICE_CALL, vol, DeviceType::DEVICE_TYPE_SPEAKER);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_004, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_MUSIC, vol, DeviceType::DEVICE_TYPE_WIRED_HEADPHONES);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_005, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_RING, vol, DeviceType::DEVICE_TYPE_EARPIECE);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_006, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_VOICE_CALL, vol, DeviceType::DEVICE_TYPE_WIRED_HEADSET);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_007, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_ULTRASONIC, vol, DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}

HWTEST(AudioGroupManagerUnitTest, Audio_Group_Manager_GetSystemVolumeInDb_008, TestSize.Level0)
{
    std::vector<sptr<VolumeGroupInfo>> infos;
    AudioSystemManager::GetInstance()->GetVolumeGroups(networkId, infos);
    int vol = 3;
    if (infos.size() > 0) {
        int32_t groupId = infos[0]->volumeGroupId_;
        auto audioGroupMngr_ = AudioSystemManager::GetInstance()->GetGroupManager(groupId);

        float db = audioGroupMngr_->GetSystemVolumeInDb(AudioVolumeType::STREAM_VOICE_ASSISTANT, vol, DeviceType::DEVICE_TYPE_USB_HEADSET);
        GTEST_LOG_(INFO) << "Get system volume in Db: " << db;
        EXPECT_LT(SUCCESS, db);
    }
}
} // namespace AudioStandard
} // namespace OHOS
