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
#include <cmath>
#include <memory>
#include "hpae_sink_input_node.h"
#include "hpae_sink_output_node.h"
#include "test_case_common.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
const char *ROOT_PATH = "/data/source_file_io_48000_2_s16le.pcm";
class HpaeSinkOutputNodeTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
};

void HpaeSinkOutputNodeTest::SetUp()
{}

void HpaeSinkOutputNodeTest::TearDown()
{}

static void PrepareNodeInfo(HpaeNodeInfo &nodeInfo)
{
    size_t frameLen = 960;
    uint32_t nodeId = 1243;
    nodeInfo.nodeId = nodeId;
    nodeInfo.frameLen = frameLen;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
}

TEST_F(HpaeSinkOutputNodeTest, constructHpaeSinkOutputNode)
{
    uint32_t sessionId = 10001;
    HpaeNodeInfo nodeInfo;
    PrepareNodeInfo(nodeInfo);
    nodeInfo.sessionId = sessionId;
    std::shared_ptr<HpaeSinkOutputNode> hpaeSinkOutputNode = std::make_shared<HpaeSinkOutputNode>(nodeInfo);
    EXPECT_EQ(hpaeSinkOutputNode->GetSampleRate(), nodeInfo.samplingRate);
    EXPECT_EQ(hpaeSinkOutputNode->GetNodeId(), nodeInfo.nodeId);
    EXPECT_EQ(hpaeSinkOutputNode->GetFrameLen(), nodeInfo.frameLen);
    EXPECT_EQ(hpaeSinkOutputNode->GetChannelCount(), nodeInfo.channels);
    EXPECT_EQ(hpaeSinkOutputNode->GetBitWidth(), nodeInfo.format);
    EXPECT_EQ(hpaeSinkOutputNode->GetSessionId(), nodeInfo.sessionId);

    HpaeNodeInfo &retNi = hpaeSinkOutputNode->GetNodeInfo();
    EXPECT_EQ(retNi.samplingRate, nodeInfo.samplingRate);
    EXPECT_EQ(retNi.nodeId, nodeInfo.nodeId);
    EXPECT_EQ(retNi.frameLen, nodeInfo.frameLen);
    EXPECT_EQ(retNi.channels, nodeInfo.channels);
    EXPECT_EQ(retNi.format, nodeInfo.format);
    EXPECT_EQ(retNi.sessionId, nodeInfo.sessionId);
}

static int32_t TestRendererRenderFrame(const char *data, uint64_t len)
{
    for (int32_t i = 0; i < len / SAMPLE_F32LE; i++) {
        float diff = *((float *)data + i) - i;
        EXPECT_EQ(diff, 0);
    }
    return 0;
}

TEST_F(HpaeSinkOutputNodeTest, testHpaeSinkOutConnectNode)
{
    size_t usedCount = 2;
    HpaeNodeInfo nodeInfo;
    PrepareNodeInfo(nodeInfo);
    std::shared_ptr<HpaeSinkOutputNode> hpaeSinkOutputNode = std::make_shared<HpaeSinkOutputNode>(nodeInfo);
    std::shared_ptr<HpaeSinkInputNode> hpaeSinkInputNode = std::make_shared<HpaeSinkInputNode>(nodeInfo);
    hpaeSinkOutputNode->Connect(hpaeSinkInputNode);
    std::shared_ptr<WriteIncDataCb> writeIncDataCb = std::make_shared<WriteIncDataCb>(SAMPLE_F32LE);
    hpaeSinkInputNode->RegisterWriteCallback(writeIncDataCb);
    std::string deviceClass = "file_io";
    std::string deviceNetId = "LocalDevice";
    EXPECT_EQ(hpaeSinkOutputNode->GetRenderSinkInstance(deviceClass, deviceNetId), 0);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_NEW, true);
    IAudioSinkAttr attr;
    attr.adapterName = "file_io";
    attr.openMicSpeaker = 0;
    attr.format = nodeInfo.format;
    attr.sampleRate = nodeInfo.samplingRate;
    attr.channel = nodeInfo.channels;
    attr.volume = 0.0f;
    attr.filePath = ROOT_PATH;
    attr.deviceNetworkId = deviceNetId.c_str();
    attr.deviceType = 0;
    attr.channelLayout = 0;
    attr.audioStreamFlag = 0;

    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkInit(attr), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_IDLE, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStart(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_RUNNING, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkPause(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStop(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    hpaeSinkOutputNode->DoProcess();
    TestRendererRenderFrame(hpaeSinkOutputNode->GetRenderFrameData(),
        nodeInfo.frameLen * nodeInfo.channels * GetSizeFromFormat(nodeInfo.format));
    EXPECT_EQ(hpaeSinkInputNode.use_count(), usedCount);
    hpaeSinkOutputNode->DisConnect(hpaeSinkInputNode);
    EXPECT_EQ(hpaeSinkInputNode.use_count(), 1);
    hpaeSinkOutputNode->RenderSinkDeInit();
}

TEST_F(HpaeSinkOutputNodeTest, testHpaeSinkOutConnectNodeRemote)
{
    size_t usedCount = 2;
    std::string deviceClass = "remote";
    std::string deviceNetId = "LocalDevice";
    HpaeNodeInfo nodeInfo;
    nodeInfo.deviceClass = deviceClass;
    PrepareNodeInfo(nodeInfo);
    std::shared_ptr<HpaeSinkOutputNode> hpaeSinkOutputNode = std::make_shared<HpaeSinkOutputNode>(nodeInfo);
    std::shared_ptr<HpaeSinkInputNode> hpaeSinkInputNode = std::make_shared<HpaeSinkInputNode>(nodeInfo);
    hpaeSinkOutputNode->Connect(hpaeSinkInputNode);
    std::shared_ptr<WriteIncDataCb> writeIncDataCb = std::make_shared<WriteIncDataCb>(SAMPLE_F32LE);
    hpaeSinkInputNode->RegisterWriteCallback(writeIncDataCb);
    EXPECT_EQ(hpaeSinkOutputNode->GetRenderSinkInstance(deviceClass, deviceNetId), 0);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_NEW, true);
    IAudioSinkAttr attr;
    attr.adapterName = "file_io";
    attr.openMicSpeaker = 0;
    attr.format = nodeInfo.format;
    attr.sampleRate = nodeInfo.samplingRate;
    attr.channel = nodeInfo.channels;
    attr.volume = 0.0f;
    attr.filePath = ROOT_PATH;
    attr.deviceNetworkId = deviceNetId.c_str();
    attr.deviceType = 0;
    attr.channelLayout = 0;
    attr.audioStreamFlag = 0;

    hpaeSinkOutputNode->RenderSinkInit(attr);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_IDLE, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStart(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_RUNNING, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkPause(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStop(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    hpaeSinkOutputNode->remoteTimePoint_ = std::chrono::high_resolution_clock::now();
    hpaeSinkOutputNode->DoProcess();
    TestRendererRenderFrame(hpaeSinkOutputNode->GetRenderFrameData(),
        nodeInfo.frameLen * nodeInfo.channels * GetSizeFromFormat(nodeInfo.format));
    EXPECT_EQ(hpaeSinkInputNode.use_count(), usedCount);
    hpaeSinkOutputNode->DisConnect(hpaeSinkInputNode);
    EXPECT_EQ(hpaeSinkInputNode.use_count(), 1);
    hpaeSinkOutputNode->RenderSinkDeInit();
}

TEST_F(HpaeSinkOutputNodeTest, testHpaeSinkOutHandlePaPower)
{
    std::string deviceClass = "primary";
    std::string deviceNetId = "LocalDevice";
    HpaeNodeInfo nodeInfo;
    nodeInfo.deviceClass = deviceClass;
    PrepareNodeInfo(nodeInfo);
    std::shared_ptr<HpaeSinkOutputNode> hpaeSinkOutputNode = std::make_shared<HpaeSinkOutputNode>(nodeInfo);
    std::shared_ptr<HpaeSinkInputNode> hpaeSinkInputNode = std::make_shared<HpaeSinkInputNode>(nodeInfo);
    hpaeSinkOutputNode->Connect(hpaeSinkInputNode);
    std::shared_ptr<WriteIncDataCb> writeIncDataCb = std::make_shared<WriteIncDataCb>(SAMPLE_F32LE);
    hpaeSinkInputNode->RegisterWriteCallback(writeIncDataCb);
    EXPECT_EQ(hpaeSinkOutputNode->GetRenderSinkInstance(deviceClass, deviceNetId), 0);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_NEW, true);
    IAudioSinkAttr attr;
    attr.adapterName = "primary";
    attr.openMicSpeaker = 0;
    attr.format = nodeInfo.format;
    attr.sampleRate = nodeInfo.samplingRate;
    attr.channel = nodeInfo.channels;
    attr.volume = 0.0f;
    attr.filePath = ROOT_PATH;
    attr.deviceNetworkId = deviceNetId.c_str();
    attr.deviceType = 0;
    attr.channelLayout = 0;
    attr.audioStreamFlag = 0;

    hpaeSinkOutputNode->RenderSinkInit(attr);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_IDLE, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStart(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_RUNNING, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkPause(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    EXPECT_EQ(hpaeSinkOutputNode->RenderSinkStop(), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_SUSPENDED, true);
    std::vector<HpaePcmBuffer *> &outputVec = hpaeSinkOutputNode->inputStream_.ReadPreOutputData();
    EXPECT_FALSE(outputVec.empty());
    HpaePcmBuffer *outputData = outputVec.front();
    outputData->pcmBufferInfo_.state = PCM_BUFFER_STATE_SILENCE;
    hpaeSinkOutputNode->isOpenPaPower_ = false;
    hpaeSinkOutputNode->silenceDataUs_ = 500000000; // 500000000 us, long silence time
    hpaeSinkOutputNode->HandlePaPower(outputData);
    hpaeSinkOutputNode->RenderSinkDeInit();
}

#ifdef ENABLE_HOOK_PCM
TEST_F(HpaeSinkOutputNodeTest, testDoProcessAfterResetPcmDumper)
{
    HpaeNodeInfo nodeInfo;
    std::string deviceClass = "remote";
    std::string deviceNetId = "LocalDevice";
    nodeInfo.deviceClass = deviceClass;
    PrepareNodeInfo(nodeInfo);
    std::shared_ptr<HpaeSinkOutputNode> hpaeSinkOutputNode = std::make_shared<HpaeSinkOutputNode>(nodeInfo);
    std::shared_ptr<HpaeSinkInputNode> hpaeSinkInputNode = std::make_shared<HpaeSinkInputNode>(nodeInfo);
    hpaeSinkOutputNode->Connect(hpaeSinkInputNode);
    std::shared_ptr<WriteIncDataCb> writeIncDataCb = std::make_shared<WriteIncDataCb>(SAMPLE_F32LE);
    hpaeSinkInputNode->RegisterWriteCallback(writeIncDataCb);

    EXPECT_EQ(hpaeSinkOutputNode->GetRenderSinkInstance(deviceClass, deviceNetId), SUCCESS);
    EXPECT_EQ(hpaeSinkOutputNode->GetSinkState() == STREAM_MANAGER_NEW, true);

    IAudioSinkAttr attr;
    attr.adapterName = "file_io";
    attr.openMicSpeaker = 0;
    attr.format = nodeInfo.format;
    attr.sampleRate = nodeInfo.samplingRate;
    attr.channel = nodeInfo.channels;
    attr.volume = 0.0f;
    attr.filePath = ROOT_PATH;
    attr.deviceNetworkId = deviceNetId.c_str();
    attr.deviceType = 0;
    attr.channelLayout = 0;
    attr.audioStreamFlag = 0;
    hpaeSinkOutputNode->RenderSinkInit(attr);
    hpaeSinkOutputNode->RenderSinkStart();
    hpaeSinkOutputNode->outputPcmDumper_ = nullptr;
    hpaeSinkOutputNode->DoProcess();
    hpaeSinkOutputNode->RenderSinkDeInit();
}
#endif
} // namespace HPAE
} // namespace AudioStandard
} // namespace OHOS