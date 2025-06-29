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
#include "test_case_common.h"
#include "hpae_inner_capturer_manager.h"
#include <string>
#include "audio_errors.h"
#include <thread>
#include <chrono>

using namespace OHOS;
using namespace AudioStandard;
using namespace HPAE;

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
const uint32_t DEFAULT_SESSION_ID = 123456;
const float FRAME_LENGTH_IN_SECOND = 0.02;
std::string g_rootPath = "/data/";


static HpaeSinkInfo GetInCapSinkInfo()
{
    HpaeSinkInfo sinkInfo;
    sinkInfo.deviceNetId = DEFAULT_TEST_DEVICE_NETWORKID;
    sinkInfo.deviceClass = DEFAULT_TEST_DEVICE_CLASS;
    sinkInfo.adapterName = DEFAULT_TEST_DEVICE_CLASS;
    sinkInfo.filePath = g_rootPath + "constructHpaeInnerCapturerManagerTest.pcm";
    sinkInfo.samplingRate = SAMPLE_RATE_48000;
    sinkInfo.frameLen = SAMPLE_RATE_48000 * FRAME_LENGTH_IN_SECOND;
    sinkInfo.format = SAMPLE_F32LE;
    sinkInfo.channels = STEREO;
    sinkInfo.deviceType = DEVICE_TYPE_SPEAKER;
    return sinkInfo;
}

class HpaeInnerCapturerManagerUnitTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
    std::shared_ptr<HpaeInnerCapturerManager> hpaeInnerCapturerManager_ = nullptr;
};

void HpaeInnerCapturerManagerUnitTest::SetUp(void)
{
    HpaeSinkInfo sinkInfo = GetInCapSinkInfo();
    hpaeInnerCapturerManager_ = std::make_shared<HPAE::HpaeInnerCapturerManager>(sinkInfo);
}

void HpaeInnerCapturerManagerUnitTest::TearDown(void)
{
    hpaeInnerCapturerManager_->DeInit();
    hpaeInnerCapturerManager_ = nullptr;
}

static HpaeStreamInfo GetInCapPlayStreamInfo()
{
    HpaeStreamInfo streamInfo;
    streamInfo.channels = STEREO;
    streamInfo.samplingRate = SAMPLE_RATE_44100;
    streamInfo.frameLen = SAMPLE_RATE_44100 * FRAME_LENGTH_IN_SECOND;
    streamInfo.format = SAMPLE_S16LE;
    streamInfo.sessionId = DEFAULT_SESSION_ID + 1;
    streamInfo.streamType = STREAM_MUSIC;
    streamInfo.streamClassType = HPAE_STREAM_CLASS_TYPE_PLAY;
    streamInfo.sourceType = SOURCE_TYPE_PLAYBACK_CAPTURE;
    return streamInfo;
}

static HpaeStreamInfo GetInCapRecordStreamInfo()
{
    HpaeStreamInfo streamInfo;
    streamInfo.channels = STEREO;
    streamInfo.samplingRate = SAMPLE_RATE_44100;
    streamInfo.frameLen = SAMPLE_RATE_44100 * FRAME_LENGTH_IN_SECOND;
    streamInfo.format = SAMPLE_S16LE;
    streamInfo.sessionId = DEFAULT_SESSION_ID;
    streamInfo.streamType = STREAM_MUSIC;
    streamInfo.streamClassType = HPAE_STREAM_CLASS_TYPE_RECORD;
    streamInfo.sourceType = SOURCE_TYPE_PLAYBACK_CAPTURE;
    return streamInfo;
}

static void WaitForMsgProcessing(std::shared_ptr<HpaeInnerCapturerManager>& hpaeInnerCapturerManager)
{
    int waitCount = 0;
    const int32_t waitCountThd = 5;  // 5ms
    while (hpaeInnerCapturerManager->IsMsgProcessing()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));  // 20ms frameLen, need optimize
        waitCount++;
        if (waitCount >= waitCountThd) {
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(40));  // 40ms wait time, need optimize
    EXPECT_EQ(hpaeInnerCapturerManager->IsMsgProcessing(), false);
    EXPECT_EQ(waitCount < waitCountThd, true);
}

/**
 * @tc.name  : Test Construct
 * @tc.type  : FUNC
 * @tc.number: Construct_001
 * @tc.desc  : Test Construct when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, Construct_001)
{
    EXPECT_NE(hpaeInnerCapturerManager_, nullptr);
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSinkInfo sinkInfo = GetInCapSinkInfo();
    HpaeSinkInfo dstSinkInfo = hpaeInnerCapturerManager_->GetSinkInfo();
    EXPECT_EQ(dstSinkInfo.deviceNetId == sinkInfo.deviceNetId, true);
    EXPECT_EQ(dstSinkInfo.deviceClass == sinkInfo.deviceClass, true);
    EXPECT_EQ(dstSinkInfo.adapterName == sinkInfo.adapterName, true);
    EXPECT_EQ(dstSinkInfo.frameLen == sinkInfo.frameLen, true);
    EXPECT_EQ(dstSinkInfo.samplingRate == sinkInfo.samplingRate, true);
    EXPECT_EQ(dstSinkInfo.format == sinkInfo.format, true);
    EXPECT_EQ(dstSinkInfo.channels == sinkInfo.channels, true);
    EXPECT_EQ(dstSinkInfo.deviceType == sinkInfo.deviceType, true);
}

/**
 * @tc.name  : Test Init
 * @tc.type  : FUNC
 * @tc.number: Init_001
 * @tc.desc  : Test Init.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, Init_001)
{
    EXPECT_NE(hpaeInnerCapturerManager_, nullptr);
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsInit(), true);
}

/**
 * @tc.name  : Test DeInit
 * @tc.type  : FUNC
 * @tc.number: DeInit_001
 * @tc.desc  : Test DeInit.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, DeInit_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DeInit(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsInit(), false);
}

/**
 * @tc.name  : Test CreateStream
 * @tc.type  : FUNC
 * @tc.number: CreateStream_001
 * @tc.desc  : Test CreateRendererStream when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, CreateStream_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsInit(), true);
    HpaeStreamInfo streamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(streamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSinkInputInfo sinkInputInfo;
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSinkInputInfo(streamInfo.sessionId, sinkInputInfo), SUCCESS);
}

/**
 * @tc.name  : Test CreateStream
 * @tc.type  : FUNC
 * @tc.number: CreateStream_002
 * @tc.desc  : Test CreateCapturerStream when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, CreateStream_002)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsInit(), true);
    HpaeStreamInfo streamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(streamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSourceOutputInfo sourceOutoputInfo;
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSourceOutputInfo(streamInfo.sessionId, sourceOutoputInfo), SUCCESS);
}

/**
 * @tc.name  : Test DestroyStream
 * @tc.type  : FUNC
 * @tc.number: DestroyStream_001
 * @tc.desc  : Test DestroyRendererStream when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, DestroyStream_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsInit(), true);
    HpaeStreamInfo streamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(streamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(streamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(streamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSinkInputInfo sinkInputInfo;
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSinkInputInfo(streamInfo.sessionId,
        sinkInputInfo) == ERR_INVALID_OPERATION, true);
}

/**
 * @tc.name  : Test DestroyStream
 * @tc.type  : FUNC
 * @tc.number: DestroyStream_002
 * @tc.desc  : Test DestroyCapturerStream when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, DestroyStream_002)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo streamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(streamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(streamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSourceOutputInfo sourceOutoputInfo;
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSourceOutputInfo(streamInfo.sessionId, sourceOutoputInfo)
        == ERR_INVALID_OPERATION, SUCCESS);
}

/**
 * @tc.name  : Test StreamStartPauseFlushChange_001
 * @tc.type  : FUNC
 * @tc.number: StreamStartPauseFlushChange_001
 * @tc.desc  : Test StreamStartPauseFlushChange when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, StreamStartPauseFlushChange_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSourceOutputInfo sourceOutoputInfo;

    HpaeStreamInfo playStreamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    std::shared_ptr<WriteFixedDataCb> writeInPlayDataCb = std::make_shared<WriteFixedDataCb>(SAMPLE_S16LE);
    EXPECT_EQ(hpaeInnerCapturerManager_->RegisterWriteCallback(playStreamInfo.sessionId, writeInPlayDataCb), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSinkInputInfo sinkInputInfo;

    EXPECT_EQ(hpaeInnerCapturerManager_->Pause(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsRunning(), true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Pause(playStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsRunning(), true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Flush(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Flush(playStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSinkInputInfo(playStreamInfo.sessionId, sinkInputInfo) == SUCCESS, true);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSourceOutputInfo(recordStreamInfo.sessionId, sourceOutoputInfo), SUCCESS);
    EXPECT_EQ(sourceOutoputInfo.capturerSessionInfo.state, HPAE_SESSION_PAUSED);
    EXPECT_EQ(sinkInputInfo.rendererSessionInfo.state, HPAE_SESSION_PAUSED);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(playStreamInfo.sessionId) == SUCCESS, true);
}

/**
 * @tc.name  : Test StreamStartStopDrainChange_001
 * @tc.type  : FUNC
 * @tc.number: StreamStartStopDrainChange_001
 * @tc.desc  : Test StreamStartStopDrainChange when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, StreamStartStopDrainChange_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo;
    recordStreamInfo.channels = STEREO;
    recordStreamInfo.samplingRate = SAMPLE_RATE_44100;
    recordStreamInfo.frameLen = SAMPLE_RATE_44100 * FRAME_LENGTH_IN_SECOND;
    recordStreamInfo.format = SAMPLE_S16LE;
    recordStreamInfo.sessionId = DEFAULT_SESSION_ID;
    recordStreamInfo.streamType = STREAM_MUSIC;
    recordStreamInfo.streamClassType = HPAE_STREAM_CLASS_TYPE_RECORD;
    recordStreamInfo.sourceType = SOURCE_TYPE_PLAYBACK_CAPTURE;
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSourceOutputInfo sourceOutoputInfo;

    HpaeStreamInfo playStreamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    std::shared_ptr<WriteFixedDataCb> writeInPlayDataCb = std::make_shared<WriteFixedDataCb>(SAMPLE_S16LE);
    EXPECT_EQ(hpaeInnerCapturerManager_->RegisterWriteCallback(playStreamInfo.sessionId, writeInPlayDataCb), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeSinkInputInfo sinkInputInfo;

    EXPECT_EQ(hpaeInnerCapturerManager_->Drain(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Drain(playStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Stop(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Stop(playStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->IsRunning(), true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSinkInputInfo(playStreamInfo.sessionId, sinkInputInfo) == SUCCESS, true);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetSourceOutputInfo(recordStreamInfo.sessionId, sourceOutoputInfo), SUCCESS);
    EXPECT_EQ(sourceOutoputInfo.capturerSessionInfo.state, HPAE_SESSION_STOPPED);
    EXPECT_EQ(sinkInputInfo.rendererSessionInfo.state, HPAE_SESSION_STOPPED);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(recordStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(playStreamInfo.sessionId) == SUCCESS, true);
}

/**
 * @tc.name  : Test AddNodeToSink_001
 * @tc.type  : FUNC
 * @tc.number: AddNodeToSink_001
 * @tc.desc  : Test AddNodeToSink when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, AddNodeToSink_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);

    HpaeStreamInfo playStreamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo playSencondStreamInfo = GetInCapPlayStreamInfo();
    ++playSencondStreamInfo.sessionId;
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playSencondStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playSencondStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeNodeInfo playSencondNodeInfo;
    playSencondNodeInfo.sessionId = playSencondStreamInfo.sessionId + 1;
    playSencondNodeInfo.channels = STEREO;
    playSencondNodeInfo.format = SAMPLE_S16LE;
    playSencondNodeInfo.frameLen = SAMPLE_RATE_44100 * FRAME_LENGTH_IN_SECOND;
    playSencondNodeInfo.samplingRate = SAMPLE_RATE_44100;
    playSencondNodeInfo.sceneType = HPAE_SCENE_EFFECT_NONE;
    playSencondNodeInfo.deviceClass = DEFAULT_TEST_DEVICE_CLASS;
    playSencondNodeInfo.deviceNetId = DEFAULT_TEST_DEVICE_NETWORKID;
    std::shared_ptr<HpaeSinkInputNode> HpaeSinkInputSencondNode =
        std::make_shared<HpaeSinkInputNode>(playSencondNodeInfo);
    EXPECT_EQ(HpaeSinkInputSencondNode != nullptr, true);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(playStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    hpaeInnerCapturerManager_->AddSingleNodeToSinkInner(HpaeSinkInputSencondNode, false);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->SuspendStreamManager(true), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->AddNodeToSink(HpaeSinkInputSencondNode), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->SuspendStreamManager(false), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(playSencondNodeInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(playSencondStreamInfo.sessionId) == SUCCESS, true);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->DestroyStream(recordStreamInfo.sessionId) == SUCCESS, true);
}

/**
 * @tc.name  : Test SetMute_001
 * @tc.type  : FUNC
 * @tc.number: SetMute_001
 * @tc.desc  : Test SetMute when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, SetMute_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);

    EXPECT_EQ(hpaeInnerCapturerManager_->SetMute(true), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->SetMute(false), SUCCESS);
}

/**
 * @tc.name  : Test OnFadeDone_001
 * @tc.type  : FUNC
 * @tc.number: OnFadeDone_001
 * @tc.desc  : Test OnFadeDone when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, OnFadeDone_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);

    HpaeStreamInfo playStreamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    std::shared_ptr<WriteFixedDataCb> writeInPlayDataCb = std::make_shared<WriteFixedDataCb>(SAMPLE_S16LE);
    EXPECT_EQ(hpaeInnerCapturerManager_->RegisterWriteCallback(playStreamInfo.sessionId, writeInPlayDataCb), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    hpaeInnerCapturerManager_->OnFadeDone(playStreamInfo.sessionId, OPERATION_STOPPED);
}

/**
 * @tc.name  : Test SendRequestInner_001
 * @tc.type  : FUNC
 * @tc.number: SendRequestInner_001
 * @tc.desc  : Test SendRequestInner when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, SendRequestInner_001)
{
    auto request = []() {
    };
    hpaeInnerCapturerManager_->SendRequestInner(request);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    hpaeInnerCapturerManager_->SendRequestInner(request);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
}

/**
 * @tc.name  : Test Other_001
 * @tc.type  : FUNC
 * @tc.number: Other_001
 * @tc.desc  : Test Other when config in vaild.
 */
TEST_F(HpaeInnerCapturerManagerUnitTest, Other_001)
{
    EXPECT_EQ(hpaeInnerCapturerManager_->Init(), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    HpaeStreamInfo recordStreamInfo = GetInCapRecordStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(recordStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(recordStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);

    HpaeStreamInfo playStreamInfo = GetInCapPlayStreamInfo();
    EXPECT_EQ(hpaeInnerCapturerManager_->CreateStream(playStreamInfo), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    std::shared_ptr<WriteFixedDataCb> writeInPlayDataCb = std::make_shared<WriteFixedDataCb>(SAMPLE_S16LE);
    EXPECT_EQ(hpaeInnerCapturerManager_->RegisterWriteCallback(playStreamInfo.sessionId, writeInPlayDataCb), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->Start(playStreamInfo.sessionId), SUCCESS);
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    
    std::vector<SinkInput> sinkInputs;
    sinkInputs = hpaeInnerCapturerManager_->GetAllSinkInputsInfo();
    std::vector<SourceOutput> sourceOutputs;
    sourceOutputs = hpaeInnerCapturerManager_->GetAllSourceOutputsInfo();
    std::string config = hpaeInnerCapturerManager_->GetDeviceHDFDumpInfo();
    WaitForMsgProcessing(hpaeInnerCapturerManager_);
    int32_t testVariable = 1;
    EXPECT_EQ(hpaeInnerCapturerManager_->SetClientVolume(playStreamInfo.sessionId, 1.0f), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->SetRate(playStreamInfo.sessionId, testVariable), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->SetAudioEffectMode(playStreamInfo.sessionId, testVariable), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetAudioEffectMode(playStreamInfo.sessionId, testVariable), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->SetPrivacyType(playStreamInfo.sessionId, testVariable), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetPrivacyType(playStreamInfo.sessionId, testVariable), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->GetWritableSize(playStreamInfo.sessionId), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->UpdateSpatializationState(playStreamInfo.sessionId, true, true), SUCCESS);
    EXPECT_EQ(hpaeInnerCapturerManager_->UpdateMaxLength(playStreamInfo.sessionId, testVariable), SUCCESS);
}
};
}  // namespace OHOS::AudioStandard
}  // namespace OHOS