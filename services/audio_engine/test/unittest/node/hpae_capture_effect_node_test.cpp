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
#include "hpae_capture_effect_node.h"
#include "hpae_source_input_cluster.h"
#include "test_case_common.h"

using namespace OHOS;
using namespace AudioStandard;
using namespace HPAE;

const uint32_t DEFAULT_FRAME_LENGTH = 960;
const uint32_t DEFAULT_NODE_ID = 1243;
const uint32_t DEFAULT_FORMAT = 16;

class HpaeCaptureEffectNodeTest : public testing::Test {
public:
    void SetUp() override;
    void TearDown() override;
};

void HpaeCaptureEffectNodeTest::SetUp()
{}

void HpaeCaptureEffectNodeTest::TearDown()
{}

static void GetTestNodeInfo(HpaeNodeInfo &nodeInfo)
{
    nodeInfo.nodeId = DEFAULT_NODE_ID;
    nodeInfo.frameLen = DEFAULT_FRAME_LENGTH;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    nodeInfo.sceneType = HPAE_SCENE_RECORD;
    nodeInfo.sourceBufferType = HPAE_SOURCE_BUFFER_TYPE_MIC;
}

static void GetTestAudioSourceAttr(IAudioSourceAttr &attr)
{
    attr.adapterName = "";
    attr.openMicSpeaker = 0;
    attr.format = AudioSampleFormat::INVALID_WIDTH;
    attr.sampleRate = SAMPLE_RATE_48000;
    attr.channel = STEREO;
    attr.volume = 0.0f;
    attr.bufferSize = 0;
    attr.isBigEndian = false;
    attr.filePath = NULL;
    attr.deviceNetworkId = NULL;
    attr.deviceType = 0;
    attr.sourceType = 0;
    attr.channelLayout = 0;
    attr.audioStreamFlag = 0;
}

static AudioSampleFormat ConverFormat(uint32_t format)
{
    return static_cast<AudioSampleFormat>(format / BITLENGTH - 1);
}

static int32_t TestCapturerSourceFrame(char *frame, uint64_t requestBytes, uint64_t *replyBytes)
{
    for (int32_t i = 0; i < requestBytes / SAMPLE_F32LE; i++) {
        *(float *)(frame + i * sizeof(float)) = i;
    }
    *replyBytes = requestBytes;
    return 0;
}

TEST_F(HpaeCaptureEffectNodeTest, HpaeCaptureEffectNodeTest_001)
{
    HpaeNodeInfo nodeInfo;
    GetTestNodeInfo(nodeInfo);
    std::shared_ptr<HpaeCaptureEffectNode> hpaeCaptureEffectNode = std::make_shared<HpaeCaptureEffectNode>(nodeInfo);
    EXPECT_EQ(hpaeCaptureEffectNode->GetSampleRate(), nodeInfo.samplingRate);
    EXPECT_EQ(hpaeCaptureEffectNode->GetNodeId(), nodeInfo.nodeId);
    EXPECT_EQ(hpaeCaptureEffectNode->GetFrameLen(), nodeInfo.frameLen);
    EXPECT_EQ(hpaeCaptureEffectNode->GetChannelCount(), nodeInfo.channels);
    EXPECT_EQ(hpaeCaptureEffectNode->GetBitWidth(), nodeInfo.format);
    EXPECT_EQ(hpaeCaptureEffectNode->GetCapturerEffectConfig(nodeInfo), false);
    AudioBufferConfig audioBufferConfig1 = {
        .samplingRate = SAMPLE_RATE_48000,
        .channels = CHANNEL_4,
        .format = DEFAULT_FORMAT,
    };
    AudioBufferConfig audioBufferConfig2 = {
        .samplingRate = SAMPLE_RATE_16000,
        .channels = STEREO,
        .format = DEFAULT_FORMAT,
    };
    AudioBufferConfig audioBufferConfig3 = {
        .samplingRate = SAMPLE_RATE_44100,
        .channels = CHANNEL_4,
        .format = DEFAULT_FORMAT,
    };
    hpaeCaptureEffectNode->SetCapturerEffectConfig(audioBufferConfig1, audioBufferConfig2, audioBufferConfig3);
    HpaeNodeInfo nodeInfo1;
    EXPECT_EQ(hpaeCaptureEffectNode->GetCapturerEffectConfig(nodeInfo1), true);
    EXPECT_EQ(nodeInfo1.samplingRate, audioBufferConfig1.samplingRate);
    EXPECT_EQ(nodeInfo1.channels, audioBufferConfig1.channels);
    EXPECT_EQ(nodeInfo1.format, ConverFormat(audioBufferConfig1.format));
    HpaeNodeInfo nodeInfo2;
    EXPECT_EQ(hpaeCaptureEffectNode->GetCapturerEffectConfig(nodeInfo2, HPAE_SOURCE_BUFFER_TYPE_EC), true);
    EXPECT_EQ(nodeInfo2.samplingRate, audioBufferConfig2.samplingRate);
    EXPECT_EQ(nodeInfo2.channels, audioBufferConfig2.channels);
    EXPECT_EQ(nodeInfo2.format, ConverFormat(audioBufferConfig2.format));
    HpaeNodeInfo nodeInfo3;
    EXPECT_EQ(hpaeCaptureEffectNode->GetCapturerEffectConfig(nodeInfo3, HPAE_SOURCE_BUFFER_TYPE_MICREF), true);
    EXPECT_EQ(nodeInfo3.samplingRate, audioBufferConfig3.samplingRate);
    EXPECT_EQ(nodeInfo3.channels, audioBufferConfig3.channels);
    EXPECT_EQ(nodeInfo3.format, ConverFormat(audioBufferConfig3.format));

    EXPECT_EQ(hpaeCaptureEffectNode->Reset(), true);
}

TEST_F(HpaeCaptureEffectNodeTest, HpaeCaptureEffectNodeTest_002)
{
    HpaeNodeInfo nodeInfo;
    GetTestNodeInfo(nodeInfo);
    std::shared_ptr<NodeStatusCallback> testStatuscallback = std::make_shared<NodeStatusCallback>();
    nodeInfo.statusCallback = testStatuscallback;
    std::shared_ptr<HpaeCaptureEffectNode> hpaeCaptureEffectNode = std::make_shared<HpaeCaptureEffectNode>(nodeInfo);
    EXPECT_EQ(hpaeCaptureEffectNode->GetSampleRate(), nodeInfo.samplingRate);
    EXPECT_EQ(hpaeCaptureEffectNode->GetNodeId(), nodeInfo.nodeId);
    EXPECT_EQ(hpaeCaptureEffectNode->GetFrameLen(), nodeInfo.frameLen);
    EXPECT_EQ(hpaeCaptureEffectNode->GetChannelCount(), nodeInfo.channels);
    EXPECT_EQ(hpaeCaptureEffectNode->GetBitWidth(), nodeInfo.format);

    std::shared_ptr<HpaeSourceInputCluster> hpaeSourceInputCluster = std::make_shared<HpaeSourceInputCluster>(nodeInfo);
    hpaeCaptureEffectNode->ConnectWithInfo(hpaeSourceInputCluster, nodeInfo);
    EXPECT_EQ(hpaeSourceInputCluster->GetSourceInputNodeUseCount(), 1 + 1);

    std::string deviceClass = "file_io";
    std::string deviceNetId = "LocalDevice";
    SourceType sourceType = SOURCE_TYPE_MIC;
    std::string sourceName = "mic";
    EXPECT_EQ(hpaeSourceInputCluster->GetCapturerSourceInstance(deviceClass, deviceNetId, sourceType, sourceName), 0);
    IAudioSourceAttr attr;
    GetTestAudioSourceAttr(attr);
    hpaeSourceInputCluster->CapturerSourceInit(attr);
    hpaeSourceInputCluster->CapturerSourceStart();

    uint64_t requestBytes = nodeInfo.frameLen * nodeInfo.channels * GetSizeFromFormat(nodeInfo.format);
    uint64_t replyBytes = 0;
    std::vector<char> testData(requestBytes);

    TestCapturerSourceFrame(testData.data(), requestBytes, &replyBytes);
    hpaeSourceInputCluster->sourceInputNode_->WriteCapturerData(testData.data(), requestBytes);
    hpaeCaptureEffectNode->DoProcess();
    OutputPort<HpaePcmBuffer *> *outputPort = hpaeCaptureEffectNode->GetOutputPort();
    HpaePcmBuffer* outPcmBuffer = outputPort->PullOutputData();
    float* outputPcmData = outPcmBuffer->GetPcmDataBuffer();
    for (int32_t j = 0; j < nodeInfo.frameLen; j++) {
        for (int32_t k = 0; k < nodeInfo.channels; k++) {
            float diff = outputPcmData[(j * nodeInfo.channels + k)] - (j * nodeInfo.channels + k);
            EXPECT_EQ(fabs(diff) < TEST_VALUE_PRESION, true);
        }
    }
    hpaeCaptureEffectNode->DisConnectWithInfo(hpaeSourceInputCluster, nodeInfo);
    EXPECT_EQ(hpaeSourceInputCluster->GetSourceInputNodeUseCount(), 1);
}

TEST_F(HpaeCaptureEffectNodeTest, HpaeCaptureEffectNodeTest_003)
{
    HpaeNodeInfo nodeInfo;
    GetTestNodeInfo(nodeInfo);
    std::shared_ptr<NodeStatusCallback> testStatuscallback = std::make_shared<NodeStatusCallback>();
    nodeInfo.statusCallback = testStatuscallback;
    std::shared_ptr<HpaeCaptureEffectNode> hpaeCaptureEffectNode = std::make_shared<HpaeCaptureEffectNode>(nodeInfo);
    EXPECT_EQ(hpaeCaptureEffectNode->GetSampleRate(), nodeInfo.samplingRate);
    EXPECT_EQ(hpaeCaptureEffectNode->GetNodeId(), nodeInfo.nodeId);
    EXPECT_EQ(hpaeCaptureEffectNode->GetFrameLen(), nodeInfo.frameLen);
    EXPECT_EQ(hpaeCaptureEffectNode->GetChannelCount(), nodeInfo.channels);
    EXPECT_EQ(hpaeCaptureEffectNode->GetBitWidth(), nodeInfo.format);
    CaptureEffectAttr attr = {
        .micChannels = STEREO,
        .ecChannels = STEREO,
        .micRefChannels = STEREO,
    };
    EXPECT_NE(hpaeCaptureEffectNode->CaptureEffectCreate(0, attr), 0);
    EXPECT_NE(hpaeCaptureEffectNode->CaptureEffectRelease(0), 0);
}
