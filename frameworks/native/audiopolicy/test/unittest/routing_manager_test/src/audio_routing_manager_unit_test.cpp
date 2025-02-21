/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <thread>
#include "audio_errors.h"
#include "audio_info.h"
#include "audio_routing_manager_unit_test.h"
#include "audio_stream_manager.h"
#include "audio_system_manager.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

void AudioRoutingManagerUnitTest::SetUpTestCase(void) {}
void AudioRoutingManagerUnitTest::TearDownTestCase(void) {}
void AudioRoutingManagerUnitTest::SetUp(void) {}
void AudioRoutingManagerUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test Audio_Routing_Manager_SetMicStateChangeCallback_001 via legal state
 * @tc.number: Audio_Routing_Manager_SetMicStateChangeCallback_001
 * @tc.desc  : Test SetMicStateChangeCallback interface. Returns success.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_SetMicStateChangeCallback_001, TestSize.Level1)
{
    int32_t ret = -1;
    std::shared_ptr<AudioManagerMicStateChangeCallbackTest> callback =
        std::make_shared<AudioManagerMicStateChangeCallbackTest>();
    ret = AudioRoutingManager::GetInstance()->SetMicStateChangeCallback(callback);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio_Routing_Manager_getPreferredOutputDeviceForRendererInfo_001 via legal state
 * @tc.number: Audio_Routing_Manager_getPreferredOutputDeviceForRendererInfo_001
 * @tc.desc  : Test getPreferredOutputDeviceForRendererInfo interface. Returns success.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_getPreferredOutputDeviceForRendererInfo_001, TestSize.Level1)
{
    int32_t ret = -1;
    AudioRendererInfo rendererInfo;
    std::vector<sptr<AudioDeviceDescriptor>> desc;
    ret = AudioRoutingManager::GetInstance()->GetPreferredOutputDeviceForRendererInfo(rendererInfo, desc);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio_Routing_Manager_PreferredOutputDeviceChangeCallback_001 via legal state
 * @tc.number: Audio_Routing_Manager_PreferredOutputDeviceChangeCallback_001
 * @tc.desc  : Test PreferredOutputDeviceChangeCallback interface. Returns success.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_PreferredOutputDeviceChangeCallback_001, TestSize.Level1)
{
    int32_t ret = -1;
    AudioRendererInfo rendererInfo;
    std::shared_ptr<AudioPreferredOutputDeviceChangeCallbackTest> callback =
        std::make_shared<AudioPreferredOutputDeviceChangeCallbackTest>();
    ret = AudioRoutingManager::GetInstance()->SetPreferredOutputDeviceChangeCallback(rendererInfo, callback);
    EXPECT_EQ(SUCCESS, ret);

    ret = AudioRoutingManager::GetInstance()->UnsetPreferredOutputDeviceChangeCallback();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio_Routing_Manager_getPreferredInputDeviceForCapturerInfo_001 via legal state
 * @tc.number: Audio_Routing_Manager_getPreferredInputDeviceForCapturerInfo_001
 * @tc.desc  : Test getPreferredInputDeviceForCapturerInfo interface. Returns success.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_getPreferredInputDeviceForCapturerInfo_001, TestSize.Level1)
{
    int32_t ret = -1;
    AudioCapturerInfo capturerInfo;
    std::vector<sptr<AudioDeviceDescriptor>> desc;
    ret = AudioRoutingManager::GetInstance()->GetPreferredInputDeviceForCapturerInfo(capturerInfo, desc);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio_Routing_Manager_PreferredInputDeviceChangeCallback_001 via legal state
 * @tc.number: Audio_Routing_Manager_PreferredInputDeviceChangeCallback_001
 * @tc.desc  : Test PreferredInputDeviceChangeCallback interface. Returns success.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_PreferredInputDeviceChangeCallback_001, TestSize.Level1)
{
    int32_t ret = -1;
    AudioCapturerInfo capturerInfo;
    std::shared_ptr<AudioPreferredInputDeviceChangeCallbackTest> callback =
        std::make_shared<AudioPreferredInputDeviceChangeCallbackTest>();
    ret = AudioRoutingManager::GetInstance()->SetPreferredInputDeviceChangeCallback(capturerInfo, callback);
    EXPECT_EQ(SUCCESS, ret);

    ret = AudioRoutingManager::GetInstance()->UnsetPreferredInputDeviceChangeCallback();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio_Routing_Manager_GetAvailableMicrophones_001 via legal state
 * @tc.number: Audio_Routing_Manager_GetAvailableMicrophones_001
 * @tc.desc  : Test GetAvailableMicrophones interface.
 */
HWTEST(AudioRoutingManagerUnitTest, Audio_Routing_Manager_GetAvailableMicrophones_001, TestSize.Level1)
{
    int32_t ret = -1;
    auto inputDeviceDescriptors = AudioSystemManager::GetInstance()->GetDevices(DeviceFlag::INPUT_DEVICES_FLAG);
    if (inputDeviceDescriptors.size() == 0) {
        ret = SUCCESS;
        EXPECT_EQ(SUCCESS, ret);
        return;
    }
    auto microphoneDescriptors = AudioRoutingManager::GetInstance()->GetAvailableMicrophones();
    EXPECT_GT(microphoneDescriptors.size(), 0);
    for (auto inputDescriptor : inputDeviceDescriptors) {
        for (auto micDescriptor : microphoneDescriptors) {
            if (micDescriptor->deviceType_ == inputDescriptor->deviceType_) {
                ret = SUCCESS;
            }
        }
    }
    EXPECT_EQ(SUCCESS, ret);
}
} // namespace AudioStandard
} // namespace OHOS