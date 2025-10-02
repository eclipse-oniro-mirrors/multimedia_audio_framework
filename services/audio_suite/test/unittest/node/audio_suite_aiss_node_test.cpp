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
#include <gmock/gmock.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <chrono>
#include <thread>
#include <dlfcn.h>
#include "securec.h"
#include "audio_suite_aiss_node.h"
#include "audio_suite_aiss_algo_interface_impl.h"
#include "audio_suite_algo_interface.h"

using namespace OHOS;
using namespace AudioStandard;
using namespace AudioSuite;
using namespace testing::ext;
using ::testing::_;
using ::testing::Return;

class MockSuiteNodeReadTapDataCallback : public SuiteNodeReadTapDataCallback {
    MOCK_METHOD(void, OnReadTapDataCallback, (void*, int32_t), (override));
};

class AudioSuiteAissNodeTest : public testing::Test {
public:
    void SetUp()
    {
        AudioFormat audioFormat;
        impl = std::make_shared<AudioSuiteAissNode>(NODE_TYPE_AUDIO_SEPARATION, audioFormat);
    };
    void TearDown()
    {
        impl = nullptr;
    };
    std::shared_ptr<AudioSuiteAissNode> impl = nullptr;
};

namespace {
    const std::string INPUT_PATH = "/data/aiss_48000_2_S32LE.pcm";
    constexpr uint32_t FRAME_LEN_MS = 20;
    constexpr uint32_t DEFAULT_SAMPLING_RATE = 48000;
    constexpr uint32_t TEST_CONVERT_SAMPLING_RATE = 44100;
    constexpr uint32_t DEFAULT_CHANNELS_IN = 2;
    constexpr uint32_t DEFAULT_CHANNELS_OUT = 4;
    constexpr uint32_t BYTES_PER_SAMPLE = 4;
    const AudioChannelLayout LAY_OUT = CH_LAYOUT_STEREO;

    HWTEST_F(AudioSuiteAissNodeTest, InitTest, TestSize.Level0)
    {
        EXPECT_EQ(impl->Init(), SUCCESS);
        EXPECT_EQ(impl->DeInit(), SUCCESS);
    }

    HWTEST_F(AudioSuiteAissNodeTest, GetOutputPortTest, TestSize.Level0)
    {
        auto humanPort = impl->GetOutputPort(AUDIO_NODE_HUMAN_SOUND_OUTPORT_TYPE);
        ASSERT_NE(humanPort, nullptr);
        auto bkgPort = impl->GetOutputPort(AUDIO_NODE_BACKGROUND_SOUND_OUTPORT_TYPE);
        ASSERT_NE(bkgPort, nullptr);
        auto port = impl->GetOutputPort(static_cast<AudioNodePortType>(100));
        ASSERT_EQ(port, nullptr);
    }

    HWTEST_F(AudioSuiteAissNodeTest, FlushTest, TestSize.Level0)
    {
        EXPECT_EQ(impl->Flush(), SUCCESS);
    }

    HWTEST_F(AudioSuiteAissNodeTest, ResetTest, TestSize.Level0)
    {
        EXPECT_EQ(impl->Reset(), SUCCESS);
    }

    HWTEST_F(AudioSuiteAissNodeTest, TapTest, TestSize.Level0)
    {
        auto humanCallback = std::make_shared<MockSuiteNodeReadTapDataCallback>();
        auto bkgCallback = std::make_shared<MockSuiteNodeReadTapDataCallback>();
        AudioSuitePcmBuffer buffer(SAMPLE_RATE_48000, 2, CH_LAYOUT_STEREO);
        impl->HandleTapCallback(&buffer);
        EXPECT_EQ(impl->InstallTap(AUDIO_NODE_HUMAN_SOUND_OUTPORT_TYPE, humanCallback), SUCCESS);
        EXPECT_EQ(impl->InstallTap(AUDIO_NODE_BACKGROUND_SOUND_OUTPORT_TYPE, bkgCallback), SUCCESS);
        EXPECT_EQ(impl->InstallTap(static_cast<AudioNodePortType>(100), humanCallback), ERROR);
        impl->HandleTapCallback(&buffer);
        EXPECT_EQ(impl->RemoveTap(AUDIO_NODE_HUMAN_SOUND_OUTPORT_TYPE), SUCCESS);
        EXPECT_EQ(impl->RemoveTap(AUDIO_NODE_BACKGROUND_SOUND_OUTPORT_TYPE), SUCCESS);
        EXPECT_EQ(impl->RemoveTap(static_cast<AudioNodePortType>(100)), ERROR);
    }

    HWTEST_F(AudioSuiteAissNodeTest, ProcessTest, TestSize.Level0)
    {
        EXPECT_NE(impl->DoProcess(), SUCCESS);
        std::ifstream inputFile(INPUT_PATH, std::ios::binary | std::ios::ate);
        ASSERT_TRUE(inputFile.is_open());
        AudioSuitePcmBuffer inputBuffer(DEFAULT_SAMPLING_RATE, DEFAULT_CHANNELS_IN, LAY_OUT);
        const uint32_t byteSizePerFrameIn = DEFAULT_SAMPLING_RATE * FRAME_LEN_MS /
            1000 * DEFAULT_CHANNELS_IN * BYTES_PER_SAMPLE;
        inputFile.read(reinterpret_cast<char *>(inputBuffer.GetPcmDataBuffer()), byteSizePerFrameIn);
        ASSERT_FALSE(inputFile.fail() && !inputFile.eof());
        std::vector<AudioSuitePcmBuffer*> inputs;
        inputs.emplace_back(&inputBuffer);
        AudioSuitePcmBuffer *outputBuffer = impl->SignalProcess(inputs);
        EXPECT_NE(outputBuffer, nullptr);
        inputFile.close();
    }

    HWTEST_F(AudioSuiteAissNodeTest, convertTest, TestSize.Level0)
    {
        AudioSuitePcmBuffer inputBuffer(DEFAULT_SAMPLING_RATE, DEFAULT_CHANNELS_IN, LAY_OUT);
        AudioSuitePcmBuffer outputBuffer = impl->rateConvert(inputBuffer,
            TEST_CONVERT_SAMPLING_RATE, DEFAULT_CHANNELS_IN);
        EXPECT_EQ(TEST_CONVERT_SAMPLING_RATE, outputBuffer.GetSampleRate());
        AudioSuitePcmBuffer channelBuffer = impl->channelConvert(inputBuffer,
            DEFAULT_SAMPLING_RATE, DEFAULT_CHANNELS_OUT);
        EXPECT_EQ(DEFAULT_CHANNELS_OUT, channelBuffer.GetChannelCount());
    }
}