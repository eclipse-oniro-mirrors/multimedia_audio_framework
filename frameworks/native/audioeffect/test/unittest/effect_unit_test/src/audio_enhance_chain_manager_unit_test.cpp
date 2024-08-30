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

#ifndef LOG_TAG
#define LOG_TAG "AudioEnhanceChainManagerUnitTest"
#endif

#include "audio_enhance_chain_manager_unit_test.h"

#include <chrono>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "audio_effect.h"
#include "audio_utils.h"
#include "audio_effect_log.h"
#include "audio_effect_chain_manager.h"
#include "audio_errors.h"

using namespace std;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AudioStandard {

namespace {
constexpr uint32_t INFOCHANNELS = 2;
constexpr uint64_t INFOCHANNELLAYOUT = 0x3;
    
vector<EffectChain> DEFAULT_EFFECT_CHAINS = {{"EFFECTCHAIN_SPK_MUSIC", {}, ""}, {"EFFECTCHAIN_BT_MUSIC", {}, ""}};

unordered_map<string, string> DEFAULT_MAP = {
    {"SCENE_MOVIE_&_EFFECT_DEFAULT_&_DEVICE_TYPE_SPEAKER", "EFFECTCHAIN_SPK_MUSIC"},
    {"SCENE_MOVIE_&_EFFECT_DEFAULT_&_DEVICE_TYPE_BLUETOOTH_A2DP", "EFFECTCHAIN_BT_MUSIC"},
};

vector<shared_ptr<AudioEffectLibEntry>> DEFAULT_EFFECT_LIBRARY_LIST = {};

SessionEffectInfo DEFAULT_INFO = {
    "EFFECT_DEFAULT",
    "SCENE_MOVIE",
    INFOCHANNELS,
    INFOCHANNELLAYOUT,
    "0",
};
}

void AudioEnhanceChainManagerUnitTest::SetUpTestCase(void) {}
void AudioEnhanceChainManagerUnitTest::TearDownTestCase(void) {}
void AudioEnhanceChainManagerUnitTest::SetUp(void) {}
void AudioEnhanceChainManagerUnitTest::TearDown(void) {}

/**
* @tc.name   : Test CreateAudioEnhanceChainDynamic API
* @tc.number : CreateAudioEnhanceChainDynamic_001
* @tc.desc   : Test CreateAudioEnhanceChainDynamic interface.
*/
HWTEST(AudioEnhanceChainManagerUnitTest, CreateAudioEnhanceChainDynamic_001, TestSize.Level1)
{
    const char *sceneType = "SCENE_MUSIC";
    const char *enhanceMode = "ENHANCE_DEFAULT";
    const char *upDevice = "DEVICE_TYPE_SPEAKER";
    const char *downDevice = "DEVICE_TYPE_BLUETOOTH_A2DP";

    int32_t result = AudioEnhanceChainManager::GetInstance()->InitEnhanceBuffer();
    EXPECT_EQ(SUCCESS, result);

    AudioEnhanceChainManager::GetInstance()->InitAudioEnhanceChainManager(
        DEFAULT_EFFECT_CHAINS, DEFAULT_MAP, DEFAULT_EFFECT_LIBRARY_LIST);

    result = AudioEnhanceChainManager::GetInstance()->CreateAudioEnhanceChainDynamic(
        sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = AudioEnhanceChainManager::GetInstance()->ReleaseAudioEnhanceChainDynamic(
        sceneType, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);
}
} // namespace AudioStandard
} // namespace OHOS