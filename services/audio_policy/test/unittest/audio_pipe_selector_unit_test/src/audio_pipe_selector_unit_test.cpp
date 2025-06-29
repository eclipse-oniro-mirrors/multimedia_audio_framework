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
#include "audio_policy_utils.h"
#include "audio_pipe_selector_unit_test.h"
#include "audio_stream_descriptor.h"
#include "audio_stream_descriptor.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

void AudioPipeSelectorUnitTest::SetUpTestCase(void) {}
void AudioPipeSelectorUnitTest::TearDownTestCase(void) {}
void AudioPipeSelectorUnitTest::SetUp(void) {}
void AudioPipeSelectorUnitTest::TearDown(void) {}

/**
 * @tc.name: IsPipeExist_001
 * @tc.desc: Test IsPipeExist when newPipeInfo->adapterName_ != adapterName.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, IsPipeExist_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> newPipeInfoList;
    std::string adapterName = "test_adapter";
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = 1;
    streamDesc->sessionId_ = 100;
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> streamDescToPipeInfo;
    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->adapterName_ = "different_adapter";
    pipeInfo->routeFlag_ = 1;
    newPipeInfoList.push_back(pipeInfo);

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    bool result = audioPipeSelector->IsPipeExist(newPipeInfoList, adapterName,
        streamDesc, streamDescToPipeInfo);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsPipeExist_002
 * @tc.desc: Test IsPipeExist when newPipeInfo->routeFlag_ != streamDesc->routeFlag_.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, IsPipeExist_002, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> newPipeInfoList;
    std::string adapterName = "test_adapter";
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = 2;
    streamDesc->sessionId_ = 100;
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> streamDescToPipeInfo;

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->adapterName_ = "test_adapter";
    pipeInfo->routeFlag_ = 1;
    newPipeInfoList.push_back(pipeInfo);

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    bool result = audioPipeSelector->IsPipeExist(newPipeInfoList, adapterName,
        streamDesc, streamDescToPipeInfo);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: IsPipeExist_003
 * @tc.desc: Test IsPipeExist when streamDescToPipeInfo.find(streamDesc->sessionId_) == streamDescToPipeInfo.end().
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, IsPipeExist_003, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> newPipeInfoList;
    std::string adapterName = "test_adapter";
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = 1;
    streamDesc->sessionId_ = 100;
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> streamDescToPipeInfo;

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->adapterName_ = "test_adapter";
    pipeInfo->routeFlag_ = 1;
    newPipeInfoList.push_back(pipeInfo);

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    bool result = audioPipeSelector->IsPipeExist(newPipeInfoList, adapterName,
        streamDesc, streamDescToPipeInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsPipeExist_004
 * @tc.desc: Test IsPipeExist when streamDescToPipeInfo.find(streamDesc->sessionId_) != streamDescToPipeInfo.end().
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, IsPipeExist_004, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> newPipeInfoList;
    std::string adapterName = "test_adapter";
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = 1;
    streamDesc->sessionId_ = 100;
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> streamDescToPipeInfo;

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->adapterName_ = "test_adapter";
    pipeInfo->routeFlag_ = 1;
    newPipeInfoList.push_back(pipeInfo);

    streamDescToPipeInfo[100] = pipeInfo;
    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    bool result = audioPipeSelector->IsPipeExist(newPipeInfoList, adapterName,
        streamDesc, streamDescToPipeInfo);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: GetPipeType_001
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains
 *  AUDIO_OUTPUT_FLAG_FAST and AUDIO_OUTPUT_FLAG_VOIP.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_001, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_FAST | AUDIO_OUTPUT_FLAG_VOIP;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_CALL_OUT);
}

/**
 * @tc.name: GetPipeType_002
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains AUDIO_OUTPUT_FLAG_FAST.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_002, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_FAST;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_LOWLATENCY_OUT);
}

/**
 * @tc.name: GetPipeType_003
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains
 *  AUDIO_OUTPUT_FLAG_DIRECT and AUDIO_OUTPUT_FLAG_VOIP.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_003, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_DIRECT | AUDIO_OUTPUT_FLAG_VOIP;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_CALL_OUT);
}

/**
 * @tc.name: GetPipeType_004
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains AUDIO_OUTPUT_FLAG_DIRECT.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_004, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_DIRECT;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_DIRECT_OUT);
}

/**
 * @tc.name: GetPipeType_005
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains
 *  AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_005, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_OFFLOAD);
}

/**
 * @tc.name: GetPipeType_006
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag contains AUDIO_OUTPUT_FLAG_MULTICHANNEL.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_006, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_MULTICHANNEL;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_MULTICHANNEL);
}

/**
 * @tc.name: GetPipeType_007
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_PLAYBACK and flag does not contain any specific flags.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_007, TestSize.Level1)
{
    uint32_t flag = AUDIO_OUTPUT_FLAG_NORMAL;
    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_NORMAL_OUT);
}

/**
 * @tc.name: GetPipeType_008
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_RECORD and flag contains
 *  AUDIO_INPUT_FLAG_FAST and AUDIO_INPUT_FLAG_VOIP.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_008, TestSize.Level1)
{
    uint32_t flag = AUDIO_INPUT_FLAG_FAST | AUDIO_INPUT_FLAG_VOIP;
    AudioMode audioMode = AUDIO_MODE_RECORD;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_CALL_IN);
}

/**
 * @tc.name: GetPipeType_009
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_RECORD and flag contains AUDIO_INPUT_FLAG_FAST.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_009, TestSize.Level1)
{
    uint32_t flag = AUDIO_INPUT_FLAG_FAST;
    AudioMode audioMode = AUDIO_MODE_RECORD;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_LOWLATENCY_IN);
}

/**
 * @tc.name: GetPipeType_010
 * @tc.desc: Test GetPipeType when audioMode is AUDIO_MODE_RECORD and flag does not contain any specific flags.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetPipeType_010, TestSize.Level1)
{
    uint32_t flag = AUDIO_INPUT_FLAG_NORMAL;
    AudioMode audioMode = AUDIO_MODE_RECORD;
    AudioPipeType result = AudioPipeSelector::GetPipeSelector()->GetPipeType(flag, audioMode);
    EXPECT_EQ(result, PIPE_TYPE_NORMAL_IN);
}

/**
 * @tc.name: GetAdapterNameByStreamDesc_001
 * @tc.desc: Test GetAdapterNameByStreamDesc when streamDesc is not nullptr and pipeInfoPtr
 *  and adapterInfoPtr are not nullptr.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, GetAdapterNameByStreamDesc_001, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->newDeviceDescs_.push_back(std::make_shared<AudioDeviceDescriptor>());
    streamDesc->newDeviceDescs_.front()->deviceType_ = DEVICE_TYPE_SPEAKER;
    streamDesc->newDeviceDescs_.front()->networkId_ = "0";
    streamDesc->streamInfo_.format = AudioSampleFormat::SAMPLE_S16LE;
    streamDesc->streamInfo_.samplingRate = AudioSamplingRate::SAMPLE_RATE_44100;
    streamDesc->streamInfo_.channels = AudioChannel::STEREO;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    std::string result = audioPipeSelector->GetAdapterNameByStreamDesc(streamDesc);
    EXPECT_EQ(result, "");
}

/**
 * @tc.name: ConvertStreamDescToPipeInfo_001
 * @tc.desc: Test ConvertStreamDescToPipeInfo when pipeInfoPtr and adapterInfoPtr are not nullptr.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, ConvertStreamDescToPipeInfo_001, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = 1;
    streamDesc->sessionId_ = 100;
    streamDesc->newDeviceDescs_.push_back(std::make_shared<AudioDeviceDescriptor>());
    streamDesc->newDeviceDescs_.front()->deviceType_ = DEVICE_TYPE_SPEAKER;
    streamDesc->newDeviceDescs_.front()->networkId_ = "0";
    streamDesc->capturerInfo_.sourceType = SourceType::SOURCE_TYPE_MIC;

    std::shared_ptr<PipeStreamPropInfo> streamPropInfo = std::make_shared<PipeStreamPropInfo>();
    streamPropInfo->format_ = AudioSampleFormat::SAMPLE_S16LE;
    streamPropInfo->sampleRate_ = 44100;
    streamPropInfo->channelLayout_ = AudioChannelLayout::CH_LAYOUT_STEREO;
    streamPropInfo->bufferSize_ = 1024;

    std::shared_ptr<AdapterPipeInfo> pipeInfoPtr = std::make_shared<AdapterPipeInfo>();
    pipeInfoPtr->paProp_.lib_ = "test_lib";
    pipeInfoPtr->paProp_.role_ = "test_role";
    pipeInfoPtr->paProp_.moduleName_ = "test_module";
    pipeInfoPtr->name_ = "test_name";
    pipeInfoPtr->role_ = PIPE_ROLE_OUTPUT;

    std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = std::make_shared<PolicyAdapterInfo>();
    adapterInfoPtr->adapterName = "test_adapter";

    pipeInfoPtr->adapterInfo_ = adapterInfoPtr;
    streamPropInfo->pipeInfo_ = pipeInfoPtr;

    AudioPipeInfo info;
    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    audioPipeSelector->ConvertStreamDescToPipeInfo(streamDesc, streamPropInfo, info);
    EXPECT_EQ(info.pipeRole_, PIPE_ROLE_OUTPUT);
}

/**
 * @tc.name: JudgeStreamAction_001
 * @tc.desc: Test JudgeStreamAction when newPipe and oldPipe have the same adapterName and routeFlag.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_001, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "test_adapter";
    newPipe->routeFlag_ = 1;

    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "test_adapter";
    oldPipe->routeFlag_ = 1;
    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_DEFAULT);
}

/**
 * @tc.name: JudgeStreamAction_002
 * @tc.desc: Test JudgeStreamAction when newPipe and oldPipe have different adapterName and
 *  routeFlag, and neither is FAST or DIRECT.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_002, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "new_adapter";
    newPipe->routeFlag_ = 0x1234;

    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "old_adapter";
    oldPipe->routeFlag_ = 0x123456;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_RECREATE);
}

/**
 * @tc.name: JudgeStreamAction_003
 * @tc.desc: Test JudgeStreamAction when oldPipe's routeFlag is AUDIO_OUTPUT_FLAG_FAST.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_003, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "new_adapter";
    newPipe->routeFlag_ = 0x1234;

    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "old_adapter";
    oldPipe->routeFlag_ = AUDIO_OUTPUT_FLAG_FAST;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_RECREATE);
}

/**
 * @tc.name: JudgeStreamAction_004
 * @tc.desc: Test JudgeStreamAction when newPipe's routeFlag is AUDIO_OUTPUT_FLAG_FAST.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_004, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "new_adapter";
    newPipe->routeFlag_ = AUDIO_OUTPUT_FLAG_FAST;

    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "old_adapter";
    oldPipe->routeFlag_ = 0x123456;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_RECREATE);
}

/**
 * @tc.name: JudgeStreamAction_005
 * @tc.desc: Test JudgeStreamAction when oldPipe's routeFlag is AUDIO_OUTPUT_FLAG_DIRECT.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_005, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "new_adapter";
    newPipe->routeFlag_ = 0x1234;
    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "old_adapter";
    oldPipe->routeFlag_ = AUDIO_OUTPUT_FLAG_DIRECT;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_RECREATE);
}

/**
 * @tc.name: JudgeStreamAction_006
 * @tc.desc: Test JudgeStreamAction when newPipe's routeFlag is AUDIO_OUTPUT_FLAG_DIRECT.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, JudgeStreamAction_006, TestSize.Level1)
{
    std::shared_ptr<AudioPipeInfo> newPipe = std::make_shared<AudioPipeInfo>();
    newPipe->adapterName_ = "new_adapter";
    newPipe->routeFlag_ = AUDIO_OUTPUT_FLAG_DIRECT;

    std::shared_ptr<AudioPipeInfo> oldPipe = std::make_shared<AudioPipeInfo>();
    oldPipe->adapterName_ = "old_adapter";
    oldPipe->routeFlag_ = 0x123456;
    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    AudioStreamAction result = audioPipeSelector->JudgeStreamAction(newPipe, oldPipe);
    EXPECT_EQ(result, AUDIO_STREAM_ACTION_RECREATE);
}

/**
 * @tc.name: FetchPipeAndExecute_001
 * @tc.desc: Test FetchPipeAndExecute when streamDesc->routeFlag_ == AUDIO_FLAG_NONE and enter the first if branch,
 *           then enter the first for loop's if branch, do not enter the second if branch, finally enter the second
 *           for loop's second if branch.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, FetchPipeAndExecute_001, TestSize.Level1)
{
    std::shared_ptr<AudioStreamDescriptor> streamDesc = std::make_shared<AudioStreamDescriptor>();
    streamDesc->routeFlag_ = AUDIO_FLAG_NONE;
    streamDesc->audioMode_ = AUDIO_MODE_PLAYBACK;
    streamDesc->newDeviceDescs_.push_back(std::make_shared<AudioDeviceDescriptor>());
    streamDesc->newDeviceDescs_.front()->deviceType_ = DEVICE_TYPE_SPEAKER;
    streamDesc->newDeviceDescs_.front()->networkId_ = "0";
    streamDesc->streamInfo_.format = AudioSampleFormat::SAMPLE_S16LE;
    streamDesc->streamInfo_.samplingRate = AudioSamplingRate::SAMPLE_RATE_44100;
    streamDesc->streamInfo_.channels = AudioChannel::STEREO;

    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfoList;
    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->pipeRole_ = PIPE_ROLE_OUTPUT;
    pipeInfo->adapterName_ = "test_adapter";
    pipeInfo->routeFlag_ = 1;
    pipeInfoList.push_back(pipeInfo);
    AudioPipeManager::GetPipeManager()->curPipeList_ = pipeInfoList;

    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    std::vector<std::shared_ptr<AudioPipeInfo>> result = audioPipeSelector->FetchPipeAndExecute(streamDesc);
    EXPECT_FALSE(result.empty());
    EXPECT_EQ(result[0]->pipeAction_, PIPE_ACTION_DEFAULT);
}

/**
 * @tc.name: FetchPipesAndExecute_001
 * @tc.desc: Test FetchPipesAndExecute when streamDescs is empty.
 * @tc.type: FUNC
 * @tc.require: #I5Y4MZ
 */
HWTEST_F(AudioPipeSelectorUnitTest, FetchPipesAndExecute_001, TestSize.Level1)
{
    std::vector<std::shared_ptr<AudioStreamDescriptor>> streamDescs;
    auto audioPipeSelector = AudioPipeSelector::GetPipeSelector();
    std::vector<std::shared_ptr<AudioPipeInfo>> result = audioPipeSelector->FetchPipesAndExecute(streamDescs);
    EXPECT_TRUE(result.empty());
}
} // namespace AudioStandard
} // namespace OHOS