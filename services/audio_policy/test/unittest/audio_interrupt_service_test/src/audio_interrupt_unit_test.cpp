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

#define PRINT_LINE printf("debug __LINE__:%d\n", __LINE__)

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_001
* @tc.desc  : Test AudioSessionService interfaces.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_001, TestSize.Level1)
{
    auto interruptServiceTest = GetTnterruptServiceTest();
    sptr<AudioPolicyServer> server = nullptr;
    interruptServiceTest->Init(server);
    interruptServiceTest->SetCallbackHandler(GetServerHandlerTest());
    interruptServiceTest->OnSessionTimeout(0);

    AudioSessionStrategy strategy;
    strategy.concurrencyMode = AudioConcurrencyMode::DEFAULT;

    auto sessionService = interruptServiceTest->sessionService_;
    interruptServiceTest->sessionService_ = nullptr;
    auto retStatus = interruptServiceTest->ActivateAudioSession(0, strategy);
    EXPECT_EQ(retStatus, ERR_UNKNOWN);
    interruptServiceTest->AddActiveInterruptToSession(0);
    retStatus = interruptServiceTest->DeactivateAudioSession(0);
    EXPECT_EQ(retStatus, ERR_UNKNOWN);

    interruptServiceTest->sessionService_ = sessionService;
    retStatus = interruptServiceTest->ActivateAudioSession(0, strategy);
    EXPECT_EQ(retStatus, SUCCESS);
    retStatus = interruptServiceTest->ActivateAudioSession(0, strategy);
    EXPECT_EQ(retStatus, ERR_ILLEGAL_STATE);
    interruptServiceTest->AddActiveInterruptToSession(0);
    retStatus = interruptServiceTest->DeactivateAudioSession(0);
    interruptServiceTest->sessionService_->sessionTimer_->timerThread_->join();
    EXPECT_EQ(retStatus, SUCCESS);
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_002
* @tc.desc  : Test RemovePlaceholderInterruptForSession.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_002, TestSize.Level1)
{
    sptr<AudioPolicyServer> server = nullptr;
    auto interruptServiceTest = GetTnterruptServiceTest();
    EXPECT_EQ(interruptServiceTest->sessionService_, nullptr);
    interruptServiceTest->RemovePlaceholderInterruptForSession(0, false);
    interruptServiceTest->RemovePlaceholderInterruptForSession(0, true);
    auto retStatus = interruptServiceTest->IsAudioSessionActivated(0);
    EXPECT_EQ(retStatus, static_cast<bool>(ERR_UNKNOWN));

    interruptServiceTest->Init(server);
    EXPECT_NE(interruptServiceTest->sessionService_, nullptr);
    interruptServiceTest->RemovePlaceholderInterruptForSession(0, false);
    interruptServiceTest->RemovePlaceholderInterruptForSession(0, true);
    retStatus = interruptServiceTest->IsAudioSessionActivated(0);
    EXPECT_EQ(retStatus, interruptServiceTest->sessionService_->IsAudioSessionActivated(0));
}

/**
* @tc.name  : Test AudioInterruptService.
* @tc.number: AudioInterruptService_003
* @tc.desc  : Test Session MIX.
*/
HWTEST(AudioInterruptUnitTest, AudioInterruptService_003, TestSize.Level1)
{
    sptr<AudioPolicyServer> server = nullptr;
    auto interruptServiceTest = GetTnterruptServiceTest();
    AudioInterrupt incomingInterrupt;
    AudioInterrupt activeInterrupt;
    AudioFocusEntry focusEntry;
    focusEntry.isReject = true;
    incomingInterrupt.audioFocusType.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;
    bool status = interruptServiceTest->CanMixForSession(incomingInterrupt, activeInterrupt, focusEntry);
    EXPECT_EQ(status, false);

    focusEntry.isReject = false;
    incomingInterrupt.audioFocusType.sourceType = SOURCE_TYPE_VOICE_RECOGNITION;
    status = interruptServiceTest->CanMixForSession(incomingInterrupt, activeInterrupt, focusEntry);
    EXPECT_EQ(status, false);

    interruptServiceTest->Init(nullptr); // populate "interruptServiceTest->sessionService_"
    incomingInterrupt.pid = 0;
    activeInterrupt.pid = 1;
    EXPECT_EQ(false, interruptServiceTest->sessionService_->IsAudioSessionActivated(incomingInterrupt.pid));
    status = interruptServiceTest->CanMixForSession(incomingInterrupt, activeInterrupt, focusEntry);
    EXPECT_EQ(status, false);

    AudioSessionStrategy strategy;
    strategy.concurrencyMode = AudioConcurrencyMode::DEFAULT;

    interruptServiceTest->sessionService_->ActivateAudioSession(incomingInterrupt.pid, strategy);
    EXPECT_EQ(true, interruptServiceTest->sessionService_->IsAudioSessionActivated(incomingInterrupt.pid));
    AUDIO_INFO_LOG("activate incomingInterrupt by tester");
    status = interruptServiceTest->CanMixForSession(incomingInterrupt, activeInterrupt, focusEntry);
    EXPECT_EQ(status, false);

    auto retStatus = interruptServiceTest->DeactivateAudioSession(incomingInterrupt.pid);
    EXPECT_EQ(retStatus, SUCCESS);
    interruptServiceTest->sessionService_->ActivateAudioSession(activeInterrupt.pid, strategy);
    EXPECT_EQ(true, interruptServiceTest->sessionService_->IsAudioSessionActivated(activeInterrupt.pid));
    AUDIO_INFO_LOG("activate activeInterrupt by tester");
    status = interruptServiceTest->CanMixForSession(incomingInterrupt, activeInterrupt, focusEntry);
    EXPECT_EQ(status, false);

    
    retStatus = interruptServiceTest->DeactivateAudioSession(activeInterrupt.pid);

    interruptServiceTest->sessionService_->sessionTimer_->timerThread_->join();
    EXPECT_EQ(retStatus, SUCCESS);
}
} // namespace AudioStandard
} // namespace OHOS
