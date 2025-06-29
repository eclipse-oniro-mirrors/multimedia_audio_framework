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
#include <cstdio>
#include "hpae_sink_input_node.h"
#include "hpae_render_effect_node.h"
#include "hpae_sink_output_node.h"
#include "hpae_source_input_node.h"
#include <fstream>
#include <streambuf>
#include <string>
#include "test_case_common.h"
#include "audio_errors.h"
#include "audio_effect_chain_manager.h"

using namespace OHOS;
using namespace AudioStandard;
using namespace HPAE;
namespace OHOS {
namespace AudioStandard {
namespace HPAE {
static constexpr uint32_t TEST_ID = 1266;
static constexpr uint32_t TEST_FRAMELEN1 = 960;
std::vector<EffectChain> DEFAULT_EFFECT_CHAINS = {
    {"EFFECTCHAIN_SPK_MUSIC", {"apply1", "apply2", "apply3"}, ""},
    {"EFFECTCHAIN_BT_MUSIC", {}, ""}
};

EffectChainManagerParam DEFAULT_EFFECT_CHAIN_MANAGER_PARAM{
    3,
    "SCENE_DEFAULT",
    {},
    {{"SCENE_MOVIE_&_EFFECT_DEFAULT_&_DEVICE_TYPE_SPEAKER", "EFFECTCHAIN_SPK_MUSIC"},
        {"SCENE_MOVIE_&_EFFECT_DEFAULT_&_DEVICE_TYPE_BLUETOOTH_A2DP", "EFFECTCHAIN_BT_MUSIC"}},
    {{"effect1", "property1"}, {"effect4", "property5"}, {"effect1", "property4"}}
};

std::vector<std::shared_ptr<AudioEffectLibEntry>> DEFAULT_EFFECT_LIBRARY_LIST = {};
class HpaeRenderEffectNodeTest : public testing::Test {
public:
    void SetUp();
    void TearDown();
};

void HpaeRenderEffectNodeTest::SetUp()
{}

void HpaeRenderEffectNodeTest::TearDown()
{}

TEST_F(HpaeRenderEffectNodeTest, testCreate_001)
{
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);
    nodeInfo.effectInfo.effectScene = (AudioEffectScene)0xff;
    EXPECT_EQ(hpaeRenderEffectNode->AudioRendererCreate(nodeInfo), 0);
    EXPECT_NE(hpaeRenderEffectNode->ReleaseAudioEffectChain(nodeInfo), 0);
}

TEST_F(HpaeRenderEffectNodeTest, testCreate_002)
{
    constexpr uint32_t idOffset = 5;
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);
    EXPECT_EQ(hpaeRenderEffectNode->AudioRendererCreate(nodeInfo), 0);
    HpaeNodeInfo nodeInfo2 = nodeInfo;
    nodeInfo2.nodeId += idOffset;
    EXPECT_NE(hpaeRenderEffectNode->ReleaseAudioEffectChain(nodeInfo2), 0);
    EXPECT_EQ(hpaeRenderEffectNode->ReleaseAudioEffectChain(nodeInfo), 0);
}

TEST_F(HpaeRenderEffectNodeTest, testSignalProcess_001)
{
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    HpaeNodeInfo dstNodeInfo;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);

    std::vector<HpaePcmBuffer *> inputs;
    EXPECT_EQ(hpaeRenderEffectNode->SignalProcess(inputs), nullptr);
    PcmBufferInfo pcmBufferInfo(MONO, TEST_FRAMELEN1, SAMPLE_RATE_44100);
    HpaePcmBuffer hpaePcmBuffer(pcmBufferInfo);
    inputs.emplace_back(&hpaePcmBuffer);
    EXPECT_NE(hpaeRenderEffectNode->SignalProcess(inputs), nullptr);
}

TEST_F(HpaeRenderEffectNodeTest, testSignalProcess_002)
{
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);

    std::vector<HpaePcmBuffer *> inputs;
    PcmBufferInfo pcmBufferInfo(MONO, TEST_FRAMELEN1, SAMPLE_RATE_44100);
    HpaePcmBuffer hpaePcmBuffer(pcmBufferInfo);
    hpaePcmBuffer.SetBufferSilence(true);
    inputs.emplace_back(&hpaePcmBuffer);
    EXPECT_NE(hpaeRenderEffectNode->SignalProcess(inputs), nullptr);
    hpaeRenderEffectNode->ReconfigOutputBuffer();
}

TEST_F(HpaeRenderEffectNodeTest, testSignalProcess_003)
{
    AudioEffectChainManager::GetInstance()->InitAudioEffectChainManager(DEFAULT_EFFECT_CHAINS,
        DEFAULT_EFFECT_CHAIN_MANAGER_PARAM, DEFAULT_EFFECT_LIBRARY_LIST);
    std::string sceneStr = "SCENE_MUSIC";
    AudioEffectChainManager::GetInstance()->CreateAudioEffectChainDynamic(sceneStr);
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_16000;
    nodeInfo.channels = CHANNEL_6;
    nodeInfo.format = SAMPLE_F32LE;
    nodeInfo.effectInfo.effectScene = SCENE_MUSIC;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);

    std::vector<HpaePcmBuffer *> inputs;
    PcmBufferInfo pcmBufferInfo(MONO, TEST_FRAMELEN1, SAMPLE_RATE_44100);
    HpaePcmBuffer hpaePcmBuffer(pcmBufferInfo);
    hpaePcmBuffer.SetBufferSilence(true);
    inputs.emplace_back(&hpaePcmBuffer);
    EXPECT_NE(hpaeRenderEffectNode->SignalProcess(inputs), nullptr);
    hpaeRenderEffectNode->ReconfigOutputBuffer();
    AudioEffectChainManager::GetInstance()->ResetInfo();
}

TEST_F(HpaeRenderEffectNodeTest, testModifyAudioEffectChainInfo_001)
{
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);
    EXPECT_NE(hpaeRenderEffectNode, nullptr);
    ModifyAudioEffectChainInfoReason testReason = static_cast<ModifyAudioEffectChainInfoReason>(2);
    hpaeRenderEffectNode->ModifyAudioEffectChainInfo(nodeInfo, testReason);
    nodeInfo.effectInfo.effectScene = (AudioEffectScene)0xff;
    hpaeRenderEffectNode->ModifyAudioEffectChainInfo(nodeInfo, testReason);
}

TEST_F(HpaeRenderEffectNodeTest, testUpdateAudioEffectChainInfo_001)
{
    HpaeNodeInfo nodeInfo;
    nodeInfo.nodeId = TEST_ID;
    nodeInfo.frameLen = TEST_FRAMELEN1;
    nodeInfo.samplingRate = SAMPLE_RATE_48000;
    nodeInfo.channels = STEREO;
    nodeInfo.format = SAMPLE_F32LE;
    std::shared_ptr<HpaeRenderEffectNode> hpaeRenderEffectNode = std::make_shared<HpaeRenderEffectNode>(nodeInfo);
    EXPECT_NE(hpaeRenderEffectNode, nullptr);
    hpaeRenderEffectNode->UpdateAudioEffectChainInfo(nodeInfo);
    nodeInfo.effectInfo.effectScene = (AudioEffectScene)0xff;
    hpaeRenderEffectNode->UpdateAudioEffectChainInfo(nodeInfo);
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS