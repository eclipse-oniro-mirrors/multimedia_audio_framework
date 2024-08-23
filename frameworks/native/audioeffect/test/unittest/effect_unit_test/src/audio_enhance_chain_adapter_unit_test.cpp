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

#include <unistd.h>
#ifndef LOG_TAG
#define LOG_TAG "AudioEnhanceChainAdapterUnitTest"
#endif

#include "audio_enhance_chain_adapter_unit_test.h"

#include <chrono>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "audio_effect.h"
#include "audio_utils.h"
#include "audio_effect_log.h"
#include "audio_enhance_chain_adapter.h"
#include "audio_errors.h"

using namespace std;
using namespace testing::ext;
using namespace testing;

namespace OHOS {
namespace AudioStandard {

namespace {
}

void AudioEnhanceChainAdapterUnitTest::SetUpTestCase(void) {}
void AudioEnhanceChainAdapterUnitTest::TearDownTestCase(void) {}
void AudioEnhanceChainAdapterUnitTest::SetUp(void) {}
void AudioEnhanceChainAdapterUnitTest::TearDown(void) {}

/**
* @tc.name   : Test EnhanceChainManagerCreateCb API
* @tc.number : EnhanceChainManagerCreateCb_001
* @tc.desc   : Test EnhanceChainManagerCreateCb interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerCreateCb_001, TestSize.Level1)
{
    const char *sceneType = "SCENE_MUSIC";
    const char *enhanceMode = "ENHANCE_DEFAULT";
    const char *upDevice = "DEVICE_TYPE_SPEAKER";
    const char *downDevice = "DEVICE_TYPE_BLUETOOTH_A2DP";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(ERROR, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerCreateCb API
* @tc.number : EnhanceChainManagerCreateCb_002
* @tc.desc   : Test EnhanceChainManagerCreateCb interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerCreateCb_002, TestSize.Level1)
{
    const char *sceneType = "";
    const char *enhanceMode = "";
    const char *upDevice = "";
    const char *downDevice = "";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(ERROR, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerCreateCb API
* @tc.number : EnhanceChainManagerCreateCb_003
* @tc.desc   : Test EnhanceChainManagerCreateCb interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerCreateCb_003, TestSize.Level1)
{
    const char *sceneType = nullptr;
    const char *enhanceMode = nullptr;
    const char *upDevice = nullptr;
    const char *downDevice = nullptr;

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerExist API
* @tc.number : EnhanceChainManagerExist_001
* @tc.desc   : Test EnhanceChainManagerExist interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerExist_001, TestSize.Level1)
{
    const char *sceneType = "SCENE_MUSIC";
    const char *enhanceMode = "ENHANCE_DEFAULT";
    const char *upDevice = "DEVICE_TYPE_SPEAKER";
    const char *downDevice = "DEVICE_TYPE_BLUETOOTH_A2DP";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    bool result2 = EnhanceChainManagerExist("SCENE_MUSIC");
    EXPECT_EQ(false, result2);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerExist API
* @tc.number : EnhanceChainManagerExist_002
* @tc.desc   : Test EnhanceChainManagerExist interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerExist_002, TestSize.Level1)
{
    const char *sceneType = "SCENE_MUSIC";
    const char *enhanceMode = "ENHANCE_DEFAULT";
    const char *upDevice = "DEVICE_TYPE_SPEAKER";
    const char *downDevice = "DEVICE_TYPE_BLUETOOTH_A2DP";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    bool result2 = EnhanceChainManagerExist("");
    EXPECT_EQ(false, result2);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerGetAlgoConfig API
* @tc.number : EnhanceChainManagerGetAlgoConfig001
* @tc.desc   : Test EnhanceChainManagerGetAlgoConfig interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerGetAlgoConfig_001, TestSize.Level1)
{
    const char *sceneType = "SCENE_MUSIC";
    const char *enhanceMode = "ENHANCE_DEFAULT";
    const char *upDevice = "DEVICE_TYPE_SPEAKER";
    const char *downDevice = "DEVICE_TYPE_BLUETOOTH_A2DP";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerGetAlgoConfig(sceneType, upDevice, downDevice);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerGetAlgoConfig API
* @tc.number : EnhanceChainManagerGetAlgoConfig002
* @tc.desc   : Test EnhanceChainManagerGetAlgoConfig interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerGetAlgoConfig_002, TestSize.Level1)
{
    const char *sceneType = "";
    const char *enhanceMode = "";
    const char *upDevice = "";
    const char *downDevice = "";

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerGetAlgoConfig(sceneType, upDevice, downDevice);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}

/**
* @tc.name   : Test EnhanceChainManagerGetAlgoConfig API
* @tc.number : EnhanceChainManagerGetAlgoConfig003
* @tc.desc   : Test EnhanceChainManagerGetAlgoConfig interface.
*/
HWTEST(AudioEnhanceChainAdapterUnitTest, EnhanceChainManagerGetAlgoConfig_003, TestSize.Level1)
{
    const char *sceneType = nullptr;
    const char *enhanceMode = nullptr;
    const char *upDevice = nullptr;
    const char *downDevice = nullptr;

    int32_t result = EnhanceChainManagerCreateCb(sceneType, enhanceMode, upDevice, downDevice);
    EXPECT_EQ(SUCCESS, result);

    result = EnhanceChainManagerInitEnhanceBuffer();
    EXPECT_EQ(ERROR, result);

    EnhanceChainManagerGetAlgoConfig(sceneType, upDevice, downDevice);

    EnhanceChainManagerReleaseCb(sceneType, upDevice, downDevice);
}
} // namespace AudioStandard
} // namespace OHOS