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

#include <iostream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "audio_utils.h"
#include "common/hdi_adapter_info.h"
#include "manager/hdi_adapter_manager.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
class FastAudioRenderSinkUnitTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    virtual void SetUp();
    virtual void TearDown();

protected:
    static uint32_t id_;
    static std::shared_ptr<IAudioRenderSink> sink_;
    static IAudioSinkAttr attr_;
};

uint32_t FastAudioRenderSinkUnitTest::id_ = HDI_INVALID_ID;
std::shared_ptr<IAudioRenderSink> FastAudioRenderSinkUnitTest::sink_ = nullptr;
IAudioSinkAttr FastAudioRenderSinkUnitTest::attr_ = {};

void FastAudioRenderSinkUnitTest::SetUpTestCase()
{
    HdiAdapterManager &manager = HdiAdapterManager::GetInstance();
    id_ = manager.GetId(HDI_ID_BASE_RENDER, HDI_ID_TYPE_FAST, HDI_ID_INFO_DIRECT, true);
}

void FastAudioRenderSinkUnitTest::TearDownTestCase()
{
    HdiAdapterManager::GetInstance().ReleaseId(id_);
}

void FastAudioRenderSinkUnitTest::SetUp()
{
    sink_ = HdiAdapterManager::GetInstance().GetRenderSink(id_, true);
    if (sink_ == nullptr) {
        return;
    }
}

void FastAudioRenderSinkUnitTest::TearDown()
{
    sink_ = nullptr;
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_001
 * @tc.desc   : Test fast sink create
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_001, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_002
 * @tc.desc   : Test fast sink deinit
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_002, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    if (sink_->IsInited()) {
        sink_->DeInit();
    }
    EXPECT_FALSE(sink_->IsInited());
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_003
 * @tc.desc   : Test fast sink start, stop, resume, pause, flush, reset
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_003, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    int32_t ret = sink_->Start();
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->Stop();
    EXPECT_EQ(ret, SUCCESS);
    ret = sink_->Resume();
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->Pause();
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->Flush();
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->Reset();
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->Stop();
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_004
 * @tc.desc   : Test fast sink set/get volume
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_004, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    int32_t ret = sink_->SetVolume(0.0f, 0.0f);
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->SetVolume(0.0f, 1.0f);
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->SetVolume(1.0f, 0.0f);
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    ret = sink_->SetVolume(1.0f, 1.0f);
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
    float left;
    float right;
    ret = sink_->GetVolume(left, right);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_005
 * @tc.desc   : Test fast sink set audio scene
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_005, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    std::vector<DeviceType> deviceTypes = { DEVICE_TYPE_SPEAKER };
    int32_t ret = sink_->SetAudioScene(AUDIO_SCENE_DEFAULT, deviceTypes);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_006
 * @tc.desc   : Test fast sink update active device
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_006, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    std::vector<DeviceType> deviceTypes = { DEVICE_TYPE_SPEAKER };
    int32_t ret = sink_->UpdateActiveDevice(deviceTypes);
    EXPECT_EQ(ret, ERR_NOT_SUPPORTED);
}

/**
 * @tc.name   : Test FastSink API
 * @tc.number : FastSinkUnitTest_007
 * @tc.desc   : Test fast sink update apps uid
 */
HWTEST_F(FastAudioRenderSinkUnitTest, FastSinkUnitTest_007, TestSize.Level1)
{
    EXPECT_TRUE(sink_);
    vector<int32_t> appsUid;
    int32_t ret = sink_->UpdateAppsUid(appsUid);
    EXPECT_EQ(ret, ERR_INVALID_HANDLE);
}

} // namespace AudioStandard
} // namespace OHOS
