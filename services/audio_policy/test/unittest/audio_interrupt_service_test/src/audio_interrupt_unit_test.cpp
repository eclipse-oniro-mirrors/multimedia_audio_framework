/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "audio_interrupt_unit_test.h"

#include <thread>
#include <memory>
#include <vector>
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

void AudioInterruptUnitTest::SetUpTestCase(void) {}
void AudioInterruptUnitTest::TearDownTestCase(void) {}
void AudioInterruptUnitTest::SetUp(void) {}
void AudioInterruptUnitTest::TearDown(void) {}

std::shared_ptr<AudioInterruptService> GetTnterruptServiceTest()
{
    return std::make_shared<AudioInterruptService>();
}

std::shared_ptr<AudioPolicyServerHandler> GetServerHandlerTest()
{
    return DelayedSingleton<AudioPolicyServerHandler>::GetInstance();
}

sptr<AudioPolicyServer> GetPolicyServerTest()
{
    int32_t systemAbilityId = 3009;
    bool runOnCreate = false;
    sptr<AudioPolicyServer> server =
        sptr<AudioPolicyServer>::MakeSptr(systemAbilityId, runOnCreate);
    return server;
}

#define PRINT_LINE printf("debug __LINE__:%d\n", __LINE__)

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_004
* @tc.desc  : Test AddDumpInfo.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_004, TestSize.Level1)
{
    std::string dumpString;
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.empty(), true);
    interruptServiceTest->AudioInterruptZoneDump(dumpString);
    interruptServiceTest->zonesMap_[0] = std::make_shared<AudioInterruptZone>();
    interruptServiceTest->zonesMap_[0]->interruptCbsMap[0] = nullptr;
    interruptServiceTest->AudioInterruptZoneDump(dumpString);
    EXPECT_NE(dumpString.find("1 AudioInterruptZoneDump (s) available"), std::string::npos);
    EXPECT_NE(dumpString.find("Interrupt callback size: 1"), std::string::npos);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_005
* @tc.desc  : Test AbandonAudioFocus.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_005, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    AudioInterrupt incomingInterrupt;
    int32_t clientID = interruptServiceTest->clientOnFocus_;
    auto retStatus = interruptServiceTest->AbandonAudioFocus(clientID, incomingInterrupt);
    EXPECT_EQ(retStatus, SUCCESS);
}

class RemoteObjectTestStub : public IRemoteObject {
public:
    RemoteObjectTestStub() : IRemoteObject(u"IRemoteObject") {}
    int32_t GetObjectRefCount() { return 0; };
    int SendRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option) { return 0; };
    bool AddDeathRecipient(const sptr<DeathRecipient> &recipient) { return true; };
    bool RemoveDeathRecipient(const sptr<DeathRecipient> &recipient) { return true; };
    int Dump(int fd, const std::vector<std::u16string> &args) { return 0; };

    DECLARE_INTERFACE_DESCRIPTOR(u"RemoteObjectTestStub");
};

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_006
* @tc.desc  : Test SetAudioInterruptCallback and UnsetAudioInterruptCallback.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_006, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    auto retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, nullptr, 0);
    EXPECT_EQ(retStatus, ERR_INVALID_PARAM);

    retStatus = interruptServiceTest->UnsetAudioInterruptCallback(0, 0);
    EXPECT_EQ(retStatus, ERR_INVALID_PARAM);

    interruptServiceTest->zonesMap_[0] = std::make_shared<AudioInterruptZone>();
    retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, sptr<RemoteObjectTestStub>::MakeSptr(), 0);
    EXPECT_EQ(retStatus, SUCCESS);

    retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, sptr<RemoteObjectTestStub>::MakeSptr(), 0);
    EXPECT_EQ(retStatus, ERR_INVALID_PARAM);

    retStatus = interruptServiceTest->UnsetAudioInterruptCallback(0, 0);
    EXPECT_EQ(retStatus, SUCCESS);

    retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, sptr<RemoteObjectTestStub>::MakeSptr(), 0);
    EXPECT_EQ(retStatus, SUCCESS);
    interruptServiceTest->zonesMap_[0]->interruptCbsMap.clear();
    retStatus = interruptServiceTest->UnsetAudioInterruptCallback(0, 0);
    EXPECT_EQ(retStatus, SUCCESS);

    retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, sptr<RemoteObjectTestStub>::MakeSptr(), 0);
    EXPECT_EQ(retStatus, SUCCESS);
    interruptServiceTest->zonesMap_[0] = nullptr;
    retStatus = interruptServiceTest->UnsetAudioInterruptCallback(0, 0);
    EXPECT_EQ(retStatus, SUCCESS);

    retStatus = interruptServiceTest->SetAudioInterruptCallback(0, 0, sptr<RemoteObjectTestStub>::MakeSptr(), 0);
    EXPECT_EQ(retStatus, SUCCESS);
    interruptServiceTest->zonesMap_.clear();
    retStatus = interruptServiceTest->UnsetAudioInterruptCallback(0, 0);
    EXPECT_EQ(retStatus, SUCCESS);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_007
* @tc.desc  : Test ActivateAudioInterrupt.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_007, TestSize.Level1)
{
    auto server = GetPolicyServerTest();
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(server);

    AudioInterrupt audioInterrupt;
    EXPECT_NE(interruptServiceTest->policyServer_, nullptr);
    auto retStatus = interruptServiceTest->ActivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, SUCCESS);

    AudioInterrupt audioInterrupt2;
    audioInterrupt2.sessionId = 2;
    audioInterrupt2.pid = 2;
    EXPECT_EQ(interruptServiceTest->zonesMap_.empty(), false);
    interruptServiceTest->zonesMap_[0]->audioFocusInfoList.push_back(std::make_pair(audioInterrupt2, ACTIVE));
    retStatus = interruptServiceTest->ActivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, ERR_FOCUS_DENIED);

    audioInterrupt.parallelPlayFlag = true;
    retStatus = interruptServiceTest->ActivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, SUCCESS);

    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.empty(), true);
    audioInterrupt.parallelPlayFlag = false;
    retStatus = interruptServiceTest->ActivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, ERR_FOCUS_DENIED);

    retStatus = interruptServiceTest->DeactivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, SUCCESS);

    audioInterrupt.parallelPlayFlag = true;
    retStatus = interruptServiceTest->DeactivateAudioInterrupt(0, audioInterrupt);
    EXPECT_EQ(retStatus, SUCCESS);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_008
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_008, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);

    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    AudioInterrupt audioInterrupt1;
    audioInterrupt1.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt1.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt1.audioFocusType.isPlay = false;
    AudioInterrupt audioInterrupt2;
    audioInterrupt2.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt2.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt2.audioFocusType.isPlay = false;

    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt1, AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt2, AudioFocuState{});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_009
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_009, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);

    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    AudioInterrupt audioInterrupt1;
    audioInterrupt1.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt1.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt1.audioFocusType.isPlay = false;
    AudioInterrupt audioInterrupt2;
    audioInterrupt2.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt2.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt2.audioFocusType.isPlay = false;

    audioInterruptZone->audioFocusInfoList.clear();
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt1, AudioFocuState{DUCK});
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt2, AudioFocuState{});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_010
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_010, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);

    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    AudioInterrupt audioInterrupt1;
    audioInterrupt1.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt1.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt1.audioFocusType.isPlay = false;
    AudioInterrupt audioInterrupt2;
    audioInterrupt2.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt2.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt2.audioFocusType.isPlay = false;

    audioInterruptZone->audioFocusInfoList.clear();
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt1, AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt2, AudioFocuState{DUCK});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);

    audioInterruptZone->audioFocusInfoList.clear();
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt1, AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt2, AudioFocuState{STOP});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_011
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_011, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);

    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    AudioInterrupt audioInterrupt1;
    audioInterrupt1.audioFocusType.streamType = STREAM_DEFAULT;
    audioInterrupt1.audioFocusType.sourceType = SOURCE_TYPE_MIC;
    audioInterrupt1.audioFocusType.isPlay = false;
    AudioInterrupt audioInterrupt2;
    audioInterrupt2.audioFocusType.streamType = STREAM_RING;
    audioInterrupt2.audioFocusType.sourceType = SOURCE_TYPE_INVALID;
    audioInterrupt2.audioFocusType.isPlay = true;

    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt1, AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(audioInterrupt2, AudioFocuState{});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_012
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_012, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);
    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    audioInterruptZone->audioFocusInfoList.emplace_back(AudioInterrupt(), AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(AudioInterrupt(), AudioFocuState{});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_013
* @tc.desc  : Test ResumeAudioFocusList and SimulateFocusEntry.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_013, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    interruptServiceTest->Init(GetPolicyServerTest());
    interruptServiceTest->zonesMap_.clear();
    EXPECT_EQ(interruptServiceTest->zonesMap_.size(), 0);
    auto newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    EXPECT_EQ(newAudioFocuInfoList.size(), 0);
    auto audioInterruptZone = std::make_shared<AudioInterruptZone>();
    audioInterruptZone->audioFocusInfoList.emplace_back(AudioInterrupt(), AudioFocuState{PAUSE});
    audioInterruptZone->audioFocusInfoList.emplace_back(AudioInterrupt(), AudioFocuState{});
    interruptServiceTest->zonesMap_[0] = audioInterruptZone;
    newAudioFocuInfoList = interruptServiceTest->SimulateFocusEntry(0);
    interruptServiceTest->ResumeAudioFocusList(0, true);
    interruptServiceTest->ResumeAudioFocusList(0, false);
    EXPECT_EQ(newAudioFocuInfoList.size(), 2);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_015
* @tc.desc  : Test SendInterruptEvent.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_015, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    std::list<std::pair<AudioInterrupt, AudioFocuState>> pairList;
    pairList.emplace_back(AudioInterrupt(), AudioFocuState::ACTIVE);
    AudioFocuState oldState{};
    AudioFocuState newState{};
    auto it = pairList.begin();
    bool removeFocusInfo = true;
    interruptServiceTest->SendInterruptEvent(oldState, newState, it, removeFocusInfo);
    interruptServiceTest->SetCallbackHandler(GetServerHandlerTest());
    interruptServiceTest->SendInterruptEvent(oldState, newState, it, removeFocusInfo);

    interruptServiceTest->SendInterruptEvent(PAUSE, ACTIVE, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(DUCK, ACTIVE, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(PAUSE, DUCK, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(DUCK, DUCK, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(DUCK, PAUSE, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(PAUSE, PAUSE, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(DUCK, PLACEHOLDER, it, removeFocusInfo);
    interruptServiceTest->SendInterruptEvent(DUCK, STOP, it, removeFocusInfo);
    EXPECT_NE(interruptServiceTest->handler_, nullptr);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_017
* @tc.desc  : Test IsActiveStreamLowPriority and IsIncomingStreamLowPriority.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_017, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    AudioFocusEntry entry;
    entry.actionOn = INCOMING;
    bool ret = interruptServiceTest->IsActiveStreamLowPriority(entry);
    EXPECT_EQ(ret, false);

    entry.actionOn = CURRENT;
    entry.hintType = INTERRUPT_HINT_PAUSE;
    ret = interruptServiceTest->IsActiveStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.hintType = INTERRUPT_HINT_STOP;
    ret = interruptServiceTest->IsActiveStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.hintType = INTERRUPT_HINT_DUCK;
    ret = interruptServiceTest->IsActiveStreamLowPriority(entry);
    EXPECT_EQ(ret, true);

    entry.hintType = INTERRUPT_HINT_UNDUCK;
    ret = interruptServiceTest->IsActiveStreamLowPriority(entry);
    EXPECT_EQ(ret, false);
    entry.isReject = true;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.isReject = false;
    entry.actionOn = CURRENT;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, false);

    entry.actionOn = INCOMING;
    entry.hintType = INTERRUPT_HINT_PAUSE;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.hintType = INTERRUPT_HINT_STOP;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.hintType = INTERRUPT_HINT_DUCK;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, true);
    entry.hintType = INTERRUPT_HINT_NONE;
    ret = interruptServiceTest->IsIncomingStreamLowPriority(entry);
    EXPECT_EQ(ret, false);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_018
* @tc.desc  : Test WriteServiceStartupError.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_018, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    EXPECT_NO_THROW(
        interruptServiceTest->WriteServiceStartupError();
    );
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_019
* @tc.desc  : Test SendFocusChangeEvent.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_019, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    AudioInterrupt audioInterrupt;
    EXPECT_NO_THROW(
        interruptServiceTest->SendFocusChangeEvent(0, 0, audioInterrupt);

        interruptServiceTest->SetCallbackHandler(GetServerHandlerTest());
        interruptServiceTest->zonesMap_.clear();
        interruptServiceTest->SendFocusChangeEvent(0, 0, audioInterrupt);

        interruptServiceTest->zonesMap_[0] = nullptr;
        interruptServiceTest->SendFocusChangeEvent(0, 0, audioInterrupt);

        interruptServiceTest->zonesMap_[0] = std::make_shared<AudioInterruptZone>();;
        interruptServiceTest->SendFocusChangeEvent(0, 0, audioInterrupt);
    );
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_021
* @tc.desc  : Test ClearAudioFocusInfoListOnAccountsChanged.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_021, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    sptr<AudioPolicyServer> server = nullptr;
    interruptServiceTest->zonesMap_.clear();
    interruptServiceTest->ClearAudioFocusInfoListOnAccountsChanged(0);
    interruptServiceTest->zonesMap_[0] = std::make_shared<AudioInterruptZone>();
    interruptServiceTest->handler_ = nullptr;

    EXPECT_EQ(interruptServiceTest->zonesMap_[0]->audioFocusInfoList.empty(), true);
    interruptServiceTest->ClearAudioFocusInfoListOnAccountsChanged(0);
    
    interruptServiceTest->SetCallbackHandler(GetServerHandlerTest());
    EXPECT_EQ(interruptServiceTest->zonesMap_[0]->audioFocusInfoList.empty(), true);
    interruptServiceTest->ClearAudioFocusInfoListOnAccountsChanged(0);
    AudioInterrupt a1, a2, a3;
    a1.streamUsage = StreamUsage::STREAM_USAGE_VOICE_MODEM_COMMUNICATION;
    a2.streamUsage = StreamUsage::STREAM_USAGE_VOICE_RINGTONE;
    a3.streamUsage = StreamUsage::STREAM_USAGE_UNKNOWN;
    interruptServiceTest->zonesMap_[0]->audioFocusInfoList.push_back({a1, AudioFocuState::ACTIVE});
    interruptServiceTest->zonesMap_[0]->audioFocusInfoList.push_back({a2, AudioFocuState::ACTIVE});
    interruptServiceTest->zonesMap_[0]->audioFocusInfoList.push_back({a3, AudioFocuState::ACTIVE});
    EXPECT_EQ(interruptServiceTest->zonesMap_[0]->audioFocusInfoList.size(), 3);
    interruptServiceTest->ClearAudioFocusInfoListOnAccountsChanged(0);
    EXPECT_EQ(interruptServiceTest->zonesMap_[0]->audioFocusInfoList.size(), 2);
}
} // namespace AudioStandard
} // namespace OHOS
