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
#include "hpae_soft_link.h"
#include "hpae_manager_impl.h"
#include "hpae_audio_service_callback_unit_test.h"
namespace OHOS {
namespace AudioStandard {
namespace HPAE {
static std::string g_rootPath = "/data/";
class HpaeSoftLinkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp();
    void TearDown();
    void OpenAudioPort(bool openSink = true);
    void CloseAudioPort(bool closeSink = true);

    static std::shared_ptr<HpaeAudioServiceCallbackUnitTest> callback_;
    int32_t sinkId_ = -1;
    int32_t sourceId_ = -1;
};

class HpaeSoftLinkForTest : public HpaeSoftLink {
public:
    HpaeSoftLinkForTest(uint32_t sinkIdx, uint32_t sourceIdx, SoftLinkMode mode)
        : HpaeSoftLink(sinkIdx, sourceIdx, mode)
    {};
    virtual ~HpaeSoftLinkForTest() {};
    void OnStatusUpdate(IOperation operation, uint32_t streamIndex) override;
    void SetFalse(bool isRenderer, bool startFail);
private:
    bool rendererFail_ = false;
    bool capturerFail_ = false;
};

static AudioModuleInfo GetSinkAudioModeInfo(std::string name = "Speaker_File")
{
    AudioModuleInfo audioModuleInfo;
    audioModuleInfo.lib = "libmodule-hdi-sink.z.so";
    audioModuleInfo.channels = "2";
    audioModuleInfo.rate = "48000";
    audioModuleInfo.name = name;
    audioModuleInfo.adapterName = "file_io";
    audioModuleInfo.className = "file_io";
    audioModuleInfo.bufferSize = "7680";
    audioModuleInfo.format = "s32le";
    audioModuleInfo.fixedLatency = "1";
    audioModuleInfo.offloadEnable = "0";
    audioModuleInfo.networkId = "LocalDevice";
    audioModuleInfo.fileName = g_rootPath + audioModuleInfo.adapterName + "_" + audioModuleInfo.rate + "_" +
                               audioModuleInfo.channels + "_" + audioModuleInfo.format + ".pcm";
    std::stringstream typeValue;
    typeValue << static_cast<int32_t>(DEVICE_TYPE_SPEAKER);
    audioModuleInfo.deviceType = typeValue.str();
    return audioModuleInfo;
}

static AudioModuleInfo GetSourceAudioModeInfo(std::string name = "mic")
{
    AudioModuleInfo audioModuleInfo;
    audioModuleInfo.lib = "libmodule-hdi-source.z.so";
    audioModuleInfo.channels = "2";
    audioModuleInfo.rate = "48000";
    audioModuleInfo.name = name;
    audioModuleInfo.adapterName = "file_io";
    audioModuleInfo.className = "file_io";
    audioModuleInfo.bufferSize = "3840";
    audioModuleInfo.format = "s16le";
    audioModuleInfo.fixedLatency = "1";
    audioModuleInfo.offloadEnable = "0";
    audioModuleInfo.networkId = "LocalDevice";
    audioModuleInfo.fileName = g_rootPath + "source_" + audioModuleInfo.adapterName + "_" + audioModuleInfo.rate + "_" +
                               audioModuleInfo.channels + "_" + audioModuleInfo.format + ".pcm";
    std::stringstream typeValue;
    typeValue << static_cast<int32_t>(DEVICE_TYPE_FILE_SOURCE);
    audioModuleInfo.deviceType = typeValue.str();
    return audioModuleInfo;
}

std::shared_ptr<HpaeAudioServiceCallbackUnitTest> HpaeSoftLinkTest::callback_ =
    std::make_shared<HpaeAudioServiceCallbackUnitTest>();
void HpaeSoftLinkTest::SetUpTestCase()
{
    IHpaeManager::GetHpaeManager().Init();
    IHpaeManager::GetHpaeManager().RegisterSerivceCallback(callback_);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 200ms for sleep
}

void HpaeSoftLinkTest::TearDownTestCase()
{
    IHpaeManager::GetHpaeManager().DeInit();
}

void HpaeSoftLinkTest::SetUp()
{
    OpenAudioPort(true);
    OpenAudioPort(false);
}

void HpaeSoftLinkTest::TearDown()
{
    CloseAudioPort(true);
    CloseAudioPort(false);
}

void HpaeSoftLinkTest::OpenAudioPort(bool openSink)
{
    AudioModuleInfo moduleInfo = openSink ? GetSinkAudioModeInfo() : GetSourceAudioModeInfo();
    EXPECT_EQ(IHpaeManager::GetHpaeManager().OpenAudioPort(moduleInfo), SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 200ms for sleep
    if (openSink) {
        sinkId_ = callback_->GetPortId();
    } else {
        sourceId_ = callback_->GetPortId();
    }
}

void HpaeSoftLinkTest::CloseAudioPort(bool closeSink)
{
    IHpaeManager::GetHpaeManager().CloseAudioPort(closeSink ? sinkId_ : sourceId_);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 200ms for sleep
    if (closeSink) {
        sinkId_ = -1;
    } else {
        sourceId_ = -1;
    }
}

void HpaeSoftLinkForTest::OnStatusUpdate(IOperation operation, uint32_t streamIndex)
{
    CHECK_AND_RETURN_LOG(operation != OPERATION_RELEASED, "stream already released");
    CHECK_AND_RETURN_LOG(streamIndex == rendererStreamInfo_.sessionId || streamIndex == capturerStreamInfo_.sessionId,
        "invalid streamIndex");
    if (operation == OPERATION_STARTED) {
        if ((streamIndex == rendererStreamInfo_.sessionId && !rendererFail_) ||
            (streamIndex == capturerStreamInfo_.sessionId && !capturerFail_)) {
            streamStateMap_[streamIndex] = HpaeSoftLinkState::RUNNING;
        }
    } else if (operation == OPERATION_STOPPED) {
        streamStateMap_[streamIndex] = HpaeSoftLinkState::STOPPED;
        if (streamIndex == capturerStreamInfo_.sessionId) {
            FlushRingCache();
        }
    } else if (operation == OPERATION_RELEASED) {
        streamStateMap_[streamIndex] = HpaeSoftLinkState::RELEASED;
    } else {
        return;
    }

    std::lock_guard<std::mutex> lock(callbackMutex_);
    isStreamOperationFinish_ |=
        (streamIndex == rendererStreamInfo_.sessionId ? SOFTLINK_RENDERER_OPERATION : SOFTLINK_CAPTURER_OPERATION);
    callbackCV_.notify_all();
}

void HpaeSoftLinkForTest::SetFalse(bool isRenderer, bool startFail)
{
    if (isRenderer) {
        rendererFail_ = startFail;
    } else {
        capturerFail_ = startFail;
    }
}

TEST_F(HpaeSoftLinkTest, testSoftLink)
{
    std::shared_ptr<HpaeSoftLink> softLink_ =
        std::make_shared<HpaeSoftLink>(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink_, nullptr);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::NEW);

    EXPECT_EQ(softLink_->Init(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::PREPARED);

    EXPECT_EQ(softLink_->Init(), SUCCESS); // init after init
    EXPECT_EQ(softLink_->Stop(), ERR_ILLEGAL_STATE); // stop after init

    EXPECT_EQ(softLink_->Start(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RUNNING);

    EXPECT_EQ(softLink_->Init(), ERR_ILLEGAL_STATE); // init after start
    EXPECT_EQ(softLink_->Start(), SUCCESS); // start after start

    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2000ms for sleep

    EXPECT_EQ(softLink_->Stop(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::STOPPED);

    EXPECT_EQ(softLink_->Stop(), SUCCESS); // stop after stop

    EXPECT_EQ(softLink_->Release(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RELEASED);
}

TEST_F(HpaeSoftLinkTest, testCapturerOverFlow)
{
    std::shared_ptr<HpaeSoftLink> softLink_ =
        std::make_shared<HpaeSoftLink>(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink_, nullptr);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::NEW);
    EXPECT_EQ(softLink_->Init(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::PREPARED);
    EXPECT_EQ(softLink_->Start(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RUNNING);
    CloseAudioPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2000ms for sleep
    EXPECT_EQ(softLink_->Release(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RELEASED);
}

TEST_F(HpaeSoftLinkTest, testRendererUnderRun)
{
    std::shared_ptr<HpaeSoftLink> softLink_ =
        std::make_shared<HpaeSoftLink>(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink_, nullptr);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::NEW);
    EXPECT_EQ(softLink_->Init(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::PREPARED);
    EXPECT_EQ(softLink_->Start(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RUNNING);
    CloseAudioPort(false);
    std::this_thread::sleep_for(std::chrono::milliseconds(2000)); // 2000ms for sleep
    EXPECT_EQ(softLink_->Release(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RELEASED);
}

TEST_F(HpaeSoftLinkTest, testStaticFunc)
{
    std::shared_ptr<IHpaeSoftLink> softLink1 =
        IHpaeSoftLink::CreateSoftLink(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink1, nullptr);
    softLink1->SetVolume(0.f);

    CloseAudioPort(false);
    std::shared_ptr<IHpaeSoftLink> softLink2 =
        IHpaeSoftLink::CreateSoftLink(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_EQ(softLink2, nullptr);

    std::shared_ptr<IHpaeSoftLink> softLink3 =
        IHpaeSoftLink::CreateSoftLink(sinkId_, -1, SoftLinkMode::HEARING_AID);
    EXPECT_EQ(softLink3, nullptr);

    HpaeSoftLink::g_sessionId = 99999; // 99999 for max sessionId
    EXPECT_EQ(HpaeSoftLink::GenerateSessionId(), 99999); // 99999 for max sessionId
    EXPECT_EQ(HpaeSoftLink::g_sessionId, 90000); // 90000 for min sessionId
}

TEST_F(HpaeSoftLinkTest, testSoftLinkStart)
{
    std::shared_ptr<HpaeSoftLinkForTest> softLink_ =
        std::make_shared<HpaeSoftLinkForTest>(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink_, nullptr);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::NEW);
    EXPECT_EQ(softLink_->Init(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::PREPARED);
    softLink_->SetFalse(true, true);
    EXPECT_NE(softLink_->Start(), SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 200ms for sleep
    EXPECT_NE(softLink_->state_, HpaeSoftLinkState::RUNNING);

    softLink_->SetFalse(true, false);
    softLink_->SetFalse(false, true);
    EXPECT_NE(softLink_->Start(), SUCCESS);
    std::this_thread::sleep_for(std::chrono::milliseconds(200)); // 200ms for sleep
    EXPECT_NE(softLink_->state_, HpaeSoftLinkState::RUNNING);

    EXPECT_EQ(softLink_->Release(), SUCCESS);
    EXPECT_EQ(softLink_->state_, HpaeSoftLinkState::RELEASED);
}

TEST_F(HpaeSoftLinkTest, testTransSinkInfoToStreamInfo)
{
    std::shared_ptr<HpaeSoftLinkForTest> softLink_ =
        std::make_shared<HpaeSoftLinkForTest>(sinkId_, sourceId_, SoftLinkMode::HEARING_AID);
    EXPECT_NE(softLink_, nullptr);

    HpaeStreamInfo streamInfo;
    softLink_->TransSinkInfoToStreamInfo(streamInfo, HPAE_STREAM_CLASS_TYPE_RECORD);
    softLink_->linkMode_ = SoftLinkMode::OFFLOADINNERCAP_AID;
    softLink_->TransSinkInfoToStreamInfo(streamInfo, HPAE_STREAM_CLASS_TYPE_RECORD);

    EXPECT_EQ(streamInfo.sourceType, SOURCE_TYPE_OFFLOAD_CAPTURE);
}
} // namespace HPAE
} // namespace AudioStandard
} // namespace OHOS
