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

#include "oh_audio_stream_manager_unit_test.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {

void OHAudioStreamManagerUnitTest::SetUpTestCase(void) { }

void OHAudioStreamManagerUnitTest::TearDownTestCase(void) { }

void OHAudioStreamManagerUnitTest::SetUp(void) { }

void OHAudioStreamManagerUnitTest::TearDown(void) { }

/**
 * @tc.name  : Test OH_AudioStreamManager_GetDirectPlaybackSupport.
 * @tc.number: OH_AudioStreamManager_GetDirectPlaybackSupport_001
 * @tc.desc  : Test OH_AudioStreamManager_GetDirectPlaybackSupport.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_GetDirectPlaybackSupport_001, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    auto result = OH_AudioManager_GetAudioStreamManager(&audioStreamManager);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
    EXPECT_NE(audioStreamManager, nullptr);

    OH_AudioStreamInfo streamInfo;
    streamInfo.samplingRate = 48000;
    streamInfo.channelLayout = CH_LAYOUT_STEREO;
    streamInfo.encodingType = AUDIOSTREAM_ENCODING_TYPE_RAW;
    streamInfo.sampleFormat = AUDIOSTREAM_SAMPLE_S24LE;
    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MUSIC;
    OH_AudioStream_DirectPlaybackMode directPlaybackMode = AUDIOSTREAM_DIRECT_PLAYBACK_PCM_SUPPORTED;
    result = OH_AudioStreamManager_GetDirectPlaybackSupport(audioStreamManager, &streamInfo, usage,
        &directPlaybackMode);
    EXPECT_EQ(directPlaybackMode, AUDIOSTREAM_DIRECT_PLAYBACK_NOT_SUPPORTED);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
}

/**
 * @tc.name  : Test OH_AudioStreamManager_GetDirectPlaybackSupport.
 * @tc.number: OH_AudioStreamManager_GetDirectPlaybackSupport_002
 * @tc.desc  : Test OH_AudioStreamManager_GetDirectPlaybackSupport.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_GetDirectPlaybackSupport_002, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    auto result = OH_AudioManager_GetAudioStreamManager(&audioStreamManager);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
    EXPECT_NE(audioStreamManager, nullptr);

    OH_AudioStreamInfo streamInfo;
    streamInfo.samplingRate = 24000;
    streamInfo.channelLayout = CH_LAYOUT_STEREO;
    streamInfo.encodingType = AUDIOSTREAM_ENCODING_TYPE_E_AC3;
    streamInfo.sampleFormat = AUDIOSTREAM_SAMPLE_F32LE;
    OH_AudioStream_Usage usage = AUDIOSTREAM_USAGE_MUSIC;
    OH_AudioStream_DirectPlaybackMode directPlaybackMode = AUDIOSTREAM_DIRECT_PLAYBACK_PCM_SUPPORTED;
    result = OH_AudioStreamManager_GetDirectPlaybackSupport(audioStreamManager, &streamInfo, usage,
        &directPlaybackMode);
    EXPECT_EQ(directPlaybackMode, AUDIOSTREAM_DIRECT_PLAYBACK_NOT_SUPPORTED);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
}

/**
 * @tc.name  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 * @tc.number: OH_AudioStreamManager_IsAcousticEchoCancelerSupported_001
 * @tc.desc  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_IsAcousticEchoCancelerSupported_001, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    bool supported = false;
    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_MIC;
    auto result = OH_AudioStreamManager_IsAcousticEchoCancelerSupported(audioStreamManager, sourceType, &supported);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM);
}

/**
 * @tc.name  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 * @tc.number: OH_AudioStreamManager_IsAcousticEchoCancelerSupported_002
 * @tc.desc  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_IsAcousticEchoCancelerSupported_002, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    auto result = OH_AudioManager_GetAudioStreamManager(&audioStreamManager);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
    EXPECT_NE(audioStreamManager, nullptr);
    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_MIC;
    result = OH_AudioStreamManager_IsAcousticEchoCancelerSupported(audioStreamManager, sourceType, nullptr);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM);
}

/**
 * @tc.name  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 * @tc.number: OH_AudioStreamManager_IsAcousticEchoCancelerSupported_003
 * @tc.desc  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_IsAcousticEchoCancelerSupported_003, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    auto result = OH_AudioManager_GetAudioStreamManager(&audioStreamManager);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
    EXPECT_NE(audioStreamManager, nullptr);
    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_INVALID;
    result = OH_AudioStreamManager_IsAcousticEchoCancelerSupported(audioStreamManager, sourceType, nullptr);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM);
}

/**
 * @tc.name  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 * @tc.number: OH_AudioStreamManager_IsAcousticEchoCancelerSupported_004
 * @tc.desc  : Test OH_AudioStreamManager_IsAcousticEchoCancelerSupported.
 */
HWTEST(OHAudioStreamManagerUnitTest, OH_AudioStreamManager_IsAcousticEchoCancelerSupported_004, TestSize.Level0)
{
    OH_AudioStreamManager *audioStreamManager = nullptr;
    auto result = OH_AudioManager_GetAudioStreamManager(&audioStreamManager);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
    EXPECT_NE(audioStreamManager, nullptr);
    OH_AudioStream_SourceType sourceType = AUDIOSTREAM_SOURCE_TYPE_MIC;
    bool supported = false;
    result = OH_AudioStreamManager_IsAcousticEchoCancelerSupported(audioStreamManager, sourceType, &supported);
    EXPECT_EQ(supported, false);
    EXPECT_EQ(result, AUDIOCOMMON_RESULT_SUCCESS);
}
} // namespace AudioStandard
} // namespace OHOS
