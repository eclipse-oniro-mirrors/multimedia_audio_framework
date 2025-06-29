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
class RemoteAudioRenderSinkUnitTest : public testing::Test {
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

uint32_t RemoteAudioRenderSinkUnitTest::id_ = HDI_INVALID_ID;
std::shared_ptr<IAudioRenderSink> RemoteAudioRenderSinkUnitTest::sink_ = nullptr;
IAudioSinkAttr RemoteAudioRenderSinkUnitTest::attr_ = {};

void RemoteAudioRenderSinkUnitTest::SetUpTestCase()
{
    id_ = HdiAdapterManager::GetInstance().GetId(HDI_ID_BASE_RENDER, HDI_ID_TYPE_REMOTE, "test", true);
}

void RemoteAudioRenderSinkUnitTest::TearDownTestCase()
{
    HdiAdapterManager::GetInstance().ReleaseId(id_);
}

void RemoteAudioRenderSinkUnitTest::SetUp()
{
    sink_ = HdiAdapterManager::GetInstance().GetRenderSink(id_, true);
    if (sink_ == nullptr) {
        return;
    }
    attr_.channel = 2; // 2: channel
    sink_->Init(attr_);
}

void RemoteAudioRenderSinkUnitTest::TearDown()
{
    if (sink_ && sink_->IsInited()) {
        sink_->DeInit();
    }
    sink_ = nullptr;
}

/**
 * @tc.name   : Test RemoteSink API
 * @tc.number : RemoteSinkUnitTest_001
 * @tc.desc   : Test remote sink create
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_001, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
}

/**
 * @tc.name   : Test RemoteSink API
 * @tc.number : RemoteSinkUnitTest_002
 * @tc.desc   : Test remote sink init
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_002, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
    sink_->DeInit();
    int32_t ret = sink_->Init(attr_);
    EXPECT_EQ(ret, SUCCESS);
    ret = sink_->Init(attr_);
    EXPECT_EQ(ret, SUCCESS);
    EXPECT_TRUE(sink_->IsInited());
}

/**
 * @tc.name   : Test RemoteSink API
 * @tc.number : RemoteSinkUnitTest_003
 * @tc.desc   : Test remote sink start, stop, resume, pause, flush, reset
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_003, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
    int32_t ret = sink_->Start();
    EXPECT_EQ(ret, SUCCESS);
    ret = sink_->Stop();
    EXPECT_EQ(ret, SUCCESS);
    ret = sink_->Resume();
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->Pause();
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->Flush();
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->Reset();
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->Stop();
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name   : Test DirectSink API
 * @tc.number : RemoteSinkUnitTest_004
 * @tc.desc   : Test remote sink set/get volume
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_004, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
    int32_t ret = sink_->SetVolume(0.0f, 0.0f);
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->SetVolume(0.0f, 1.0f);
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->SetVolume(1.0f, 0.0f);
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    ret = sink_->SetVolume(1.0f, 1.0f);
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
    float left;
    float right;
    ret = sink_->GetVolume(left, right);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name   : Test RemoteSink API
 * @tc.number : RemoteSinkUnitTest_005
 * @tc.desc   : Test remote sink set audio scene
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_005, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
    std::vector<DeviceType> deviceTypes = { DEVICE_TYPE_SPEAKER };
    int32_t ret = sink_->SetAudioScene(AUDIO_SCENE_DEFAULT, deviceTypes);
    EXPECT_EQ(ret, ERR_ILLEGAL_STATE);
}

/**
 * @tc.name   : Test RemoteSink API
 * @tc.number : RemoteSinkUnitTest_006
 * @tc.desc   : Test remote sink update active device
 */
HWTEST_F(RemoteAudioRenderSinkUnitTest, RemoteSinkUnitTest_006, TestSize.Level1)
{
    EXPECT_TRUE(sink_ && sink_->IsInited());
    std::vector<DeviceType> deviceTypes = { DEVICE_TYPE_SPEAKER };
    int32_t ret = sink_->UpdateActiveDevice(deviceTypes);
    EXPECT_EQ(ret, ERR_NOT_SUPPORTED);
}

} // namespace AudioStandard
} // namespace OHOS
