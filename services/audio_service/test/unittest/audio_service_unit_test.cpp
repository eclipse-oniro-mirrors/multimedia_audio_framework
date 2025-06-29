/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#include "iservice_registry.h"
#include "system_ability_definition.h"

#include "audio_service.h"
#include "audio_service_log.h"
#include "audio_errors.h"
#include "audio_system_manager.h"

#include "audio_manager_proxy.h"
#include "audio_manager_listener_stub.h"
#include "audio_process_proxy.h"
#include "audio_process_in_client.h"
#include "fast_audio_stream.h"
#include "audio_endpoint_private.h"

using namespace testing::ext;

namespace OHOS {
namespace AudioStandard {
std::unique_ptr<AudioManagerProxy> audioManagerProxy;
std::shared_ptr<AudioProcessInClient> processClient_;
std::shared_ptr<FastAudioStream> fastAudioStream_;
const int32_t TEST_RET_NUM = 0;
const int32_t RENDERER_FLAGS = 0;
#ifdef HAS_FEATURE_INNERCAPTURER
const int32_t MEDIA_SERVICE_UID = 1013;
#endif
constexpr int32_t ERROR_62980101 = -62980101;

static const uint32_t NORMAL_ENDPOINT_RELEASE_DELAY_TIME_MS = 3000; // 3s
static const uint32_t A2DP_ENDPOINT_RELEASE_DELAY_TIME = 3000; // 3s
static const uint32_t VOIP_ENDPOINT_RELEASE_DELAY_TIME = 200; // 200ms
static const uint32_t VOIP_REC_ENDPOINT_RELEASE_DELAY_TIME = 60; // 60ms
static const uint32_t A2DP_ENDPOINT_RE_CREATE_RELEASE_DELAY_TIME = 200; // 200ms

class AudioServiceUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

class AudioParameterCallbackTest : public AudioParameterCallback {
    virtual void OnAudioParameterChange(const std::string networkId, const AudioParamKey key,
        const std::string& condition, const std::string& value) {}
};

void AudioServiceUnitTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void AudioServiceUnitTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void AudioServiceUnitTest::SetUp(void)
{
    // input testcase setup step，setup invoked before each testcases
}

void AudioServiceUnitTest::TearDown(void)
{
    // input testcase teardown step，teardown invoked after each testcases
}

/**
 * @tc.name  : Test AudioProcessProxy API
 * @tc.type  : FUNC
 * @tc.number: AudioProcessProxy_001
 * @tc.desc  : Test AudioProcessProxy interface.
 */
HWTEST(AudioServiceUnitTest, AudioProcessProxy_001, TestSize.Level1)
{
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(nullptr, samgr);
    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    EXPECT_NE(nullptr, object);
    std::unique_ptr<AudioProcessProxy> audioProcessProxy = std::make_unique<AudioProcessProxy>(object);

    int32_t ret = -1;
    std::shared_ptr<OHAudioBuffer> buffer;
    uint32_t spanSizeInFrame = 1000;
    uint32_t totalSizeInFrame = spanSizeInFrame - 1;
    uint32_t byteSizePerFrame = 1000;
    buffer = OHAudioBuffer::CreateFromLocal(totalSizeInFrame, spanSizeInFrame, byteSizePerFrame);

    ret=audioProcessProxy->ResolveBuffer(buffer);
    EXPECT_LT(ret, TEST_RET_NUM);

    ret = audioProcessProxy->Start();
    EXPECT_LT(ret, TEST_RET_NUM);

    bool isFlush = true;
    ret = audioProcessProxy->Pause(isFlush);
    EXPECT_LT(ret, TEST_RET_NUM);

    ret = audioProcessProxy->Resume();
    EXPECT_LT(ret, TEST_RET_NUM);

    ret = audioProcessProxy->Stop();
    EXPECT_LT(ret, TEST_RET_NUM);

    ret = audioProcessProxy->RequestHandleInfo();
    EXPECT_EQ(ret, SUCCESS);

    ret = audioProcessProxy->Release();
    EXPECT_LT(ret, TEST_RET_NUM);
}

/**
 * @tc.name  : Test AudioManagerProxy API
 * @tc.type  : FUNC
 * @tc.number: AudioManagerProxy_001
 * @tc.desc  : Test AudioManagerProxy interface.
 */
HWTEST(AudioServiceUnitTest, AudioManagerProxy_001, TestSize.Level1)
{
    int32_t ret = -1;

    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    EXPECT_NE(nullptr, samgr);
    sptr<IRemoteObject> object = samgr->GetSystemAbility(AUDIO_DISTRIBUTED_SERVICE_ID);
    EXPECT_NE(nullptr, object);

    audioManagerProxy = std::make_unique<AudioManagerProxy>(object);

    bool isMute = true;
    ret = audioManagerProxy->SetMicrophoneMute(isMute);
    if (ret == ERR_PERMISSION_DENIED) {
        return ;
    }
    EXPECT_EQ(ret, SUCCESS);

    ret = audioManagerProxy->RegiestPolicyProvider(object);
    EXPECT_EQ(SUCCESS, ret);

    EXPECT_TRUE(ret == ERROR_62980101 || ret == SUCCESS);

    bool result = audioManagerProxy->CreatePlaybackCapturerManager();
    EXPECT_EQ(result, true);

    int32_t deviceType = 1;
    std::string sinkName = "test";
    audioManagerProxy->SetOutputDeviceSink(deviceType, sinkName);
}

/**
 * @tc.name  : Test AudioManagerProxy API
 * @tc.type  : FUNC
 * @tc.number: AudioManagerProxy_002
 * @tc.desc  : Test AudioManagerProxy interface.
 */
HWTEST(AudioServiceUnitTest, AudioManagerProxy_002, TestSize.Level1)
{
    int32_t ret = -1;

    float volume = 0.1;
    ret = audioManagerProxy->SetVoiceVolume(volume);

    const std::string networkId = "LocalDevice";
    const AudioParamKey key = AudioParamKey::VOLUME;
    AudioVolumeType volumeType =AudioVolumeType::STREAM_MEDIA;
    int32_t groupId = 0;
    std::string condition = "EVENT_TYPE=1;VOLUME_GROUP_ID=" + std::to_string(groupId) + ";AUDIO_VOLUME_TYPE="
        + std::to_string(volumeType) + ";";
    std::string value = std::to_string(volume);
    audioManagerProxy->SetAudioParameter(networkId, key, condition, value);
    const std::string retStr = audioManagerProxy->GetAudioParameter(networkId, key, condition);
    EXPECT_NE(retStr, value);

    bool connected = true;
    audioManagerProxy->NotifyDeviceInfo(networkId, connected);
    ret = audioManagerProxy->CheckRemoteDeviceState(networkId, DeviceRole::OUTPUT_DEVICE, true);
    EXPECT_LT(ret, TEST_RET_NUM);
}

/**
 * @tc.name  : Test AudioManagerProxy API
 * @tc.type  : FUNC
 * @tc.number: AudioManagerProxy_004
 * @tc.desc  : Test AudioManagerProxy interface.
 */
HWTEST(AudioServiceUnitTest, AudioManagerProxy_004, TestSize.Level1)
{
    std::vector<Library> libraries;
    Library library = {};
    library.name = "testname";
    library.path ="test.so";
    libraries.push_back(library);

    std::vector<Effect> effects;
    Effect effect = {};
    effect.name = "test";
    effect.libraryName = "test";
    effects.push_back(effect);

    std::vector<Effect> successEffects;
    bool ret = audioManagerProxy->LoadAudioEffectLibraries(libraries, effects, successEffects);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test AudioManagerListenerStub API
 * @tc.type  : FUNC
 * @tc.number: AudioManagerListenerStub_001
 * @tc.desc  : Test AudioManagerListenerStub interface.
 */
HWTEST(AudioServiceUnitTest, AudioManagerListenerStub_001, TestSize.Level1)
{
    std::unique_ptr<AudioManagerListenerStub> audioManagerListenerStub = std::make_unique<AudioManagerListenerStub>();

    const std::weak_ptr<AudioParameterCallback> callback = std::make_shared<AudioParameterCallbackTest>();
    audioManagerListenerStub->SetParameterCallback(callback);
    float volume = 0.1;
    const std::string networkId = "LocalDevice";
    const AudioParamKey key = AudioParamKey::VOLUME;
    AudioVolumeType volumeType =AudioVolumeType::STREAM_MEDIA;
    int32_t groupId = 0;
    std::string condition = "EVENT_TYPE=1;VOLUME_GROUP_ID=" + std::to_string(groupId) + ";AUDIO_VOLUME_TYPE="
        + std::to_string(volumeType) + ";";
    std::string value = std::to_string(volume);
    audioManagerListenerStub->OnAudioParameterChange(networkId, key, condition, value);
    EXPECT_NE(value, "");
}


/**
 * @tc.name  : Test AudioProcessInClientInner API
 * @tc.type  : FUNC
 * @tc.number: AudioProcessInClientInner_001
 * @tc.desc  : Test AudioProcessInClientInner interface using unsupported parameters.
 */
HWTEST(AudioServiceUnitTest, AudioProcessInClientInner_001, TestSize.Level1)
{
    AudioProcessConfig config;
    config.appInfo.appPid = getpid();
    config.appInfo.appUid = getuid();

    config.audioMode = AUDIO_MODE_PLAYBACK;

    config.rendererInfo.contentType = CONTENT_TYPE_MUSIC;
    config.rendererInfo.streamUsage = STREAM_USAGE_MEDIA;
    config.rendererInfo.rendererFlags = RENDERER_FLAGS;

    config.streamInfo.channels = STEREO;
    config.streamInfo.encoding = ENCODING_PCM;
    config.streamInfo.format = SAMPLE_S16LE;
    config.streamInfo.samplingRate = SAMPLE_RATE_64000;

    fastAudioStream_ = std::make_shared<FastAudioStream>(config.streamType,
        AUDIO_MODE_PLAYBACK, config.appInfo.appUid);
    processClient_ = AudioProcessInClient::Create(config, fastAudioStream_);
    EXPECT_EQ(processClient_, nullptr);
}

/**
 * @tc.name  : Test AudioDeviceDescriptor API
 * @tc.type  : FUNC
 * @tc.number: AudioDeviceDescriptor_001
 * @tc.desc  : Test AudioDeviceDescriptor interface.
 */
HWTEST(AudioServiceUnitTest, AudioDeviceDescriptor_001, TestSize.Level1)
{
    DeviceType type = DeviceType::DEVICE_TYPE_SPEAKER;
    DeviceRole role = DeviceRole::OUTPUT_DEVICE;
    int32_t interruptGroupId = 1;
    int32_t volumeGroupId = 1;
    std::string networkId = "LocalDevice";
    std::shared_ptr<AudioDeviceDescriptor> audioDeviceDescriptor =
        std::make_shared<AudioDeviceDescriptor>(type, role, interruptGroupId, volumeGroupId, networkId);
    EXPECT_NE(audioDeviceDescriptor, nullptr);

    AudioDeviceDescriptor deviceDescriptor;
    deviceDescriptor.deviceType_ = type;
    deviceDescriptor.deviceRole_ = role;
    audioDeviceDescriptor = std::make_shared<AudioDeviceDescriptor>(deviceDescriptor);
    EXPECT_NE(audioDeviceDescriptor, nullptr);

    std::string deviceName = "";
    std::string macAddress = "";
    audioDeviceDescriptor->SetDeviceInfo(deviceName, macAddress);

    DeviceStreamInfo audioStreamInfo = {
        SAMPLE_RATE_48000,
        ENCODING_PCM,
        SAMPLE_S16LE,
        STEREO
    };
    int32_t channelMask = 1;
    audioDeviceDescriptor->SetDeviceCapability(audioStreamInfo, channelMask);

    DeviceStreamInfo streamInfo = audioDeviceDescriptor->audioStreamInfo_;
    EXPECT_EQ(streamInfo.channels, audioStreamInfo.channels);
    EXPECT_EQ(streamInfo.encoding, audioStreamInfo.encoding);
    EXPECT_EQ(streamInfo.format, audioStreamInfo.format);
    EXPECT_EQ(streamInfo.samplingRate, audioStreamInfo.samplingRate);
}

/**
 * @tc.name  : Test UpdateMuteControlSet API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceUpdateMuteControlSet_001
 * @tc.desc  : Test UpdateMuteControlSet interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceUpdateMuteControlSet_001, TestSize.Level1)
{
    AudioService::GetInstance()->UpdateMuteControlSet(1, true);
    AudioService::GetInstance()->UpdateMuteControlSet(MAX_STREAMID + 1, true);
    AudioService::GetInstance()->UpdateMuteControlSet(MAX_STREAMID - 1, false);
    AudioService::GetInstance()->UpdateMuteControlSet(MAX_STREAMID - 1, true);
    AudioService::GetInstance()->UpdateMuteControlSet(MAX_STREAMID - 1, false);
    AudioService::GetInstance()->UpdateMuteControlSet(MAX_STREAMID - 1, true);
    AudioService::GetInstance()->RemoveIdFromMuteControlSet(MAX_STREAMID - 1);
}

#ifdef HAS_FEATURE_INNERCAPTURER
/**
 * @tc.name  : Test ShouldBeInnerCap API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceShouldBeInnerCap_001
 * @tc.desc  : Test ShouldBeInnerCap interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceShouldBeInnerCap_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    bool ret = AudioService::GetInstance()->ShouldBeInnerCap(config, 0);
    EXPECT_FALSE(ret);
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PRIVATE;
    ret = AudioService::GetInstance()->ShouldBeInnerCap(config, 0);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test ShouldBeDualTone API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceShouldBeDualTone_001
 * @tc.desc  : Test ShouldBeDualTone interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceShouldBeDualTone_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_RECORD;
    config.rendererInfo.streamUsage = STREAM_USAGE_ALARM;
    bool ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_FALSE(ret);
    config.audioMode = AUDIO_MODE_PLAYBACK;
    ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test OnInitInnerCapList API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceOnInitInnerCapList_001
 * @tc.desc  : Test OnInitInnerCapList interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceOnInitInnerCapList_001, TestSize.Level1)
{
    int32_t floatRet = 0;

    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);
    EXPECT_EQ(0, floatRet);
    config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PRIVATE;
    config.audioMode = AUDIO_MODE_RECORD;
    config.streamType = STREAM_VOICE_CALL;
    config.streamInfo.channels = AudioChannel::MONO;
    config.streamInfo.samplingRate = SAMPLE_RATE_48000;
    config.streamInfo.format = SAMPLE_S16LE;
    config.streamInfo.encoding = ENCODING_PCM;
    auto audioProcess = AudioService::GetInstance()->GetAudioProcess(config);
    EXPECT_NE(audioProcess, nullptr);

    AudioService::GetInstance()->OnInitInnerCapList(1);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(false);
    EXPECT_EQ(0, floatRet);
    int32_t ret = AudioService::GetInstance()->EnableDualToneList(MAX_STREAMID - 1);
    EXPECT_EQ(SUCCESS, ret);
    ret = AudioService::GetInstance()->DisableDualToneList(MAX_STREAMID - 1);
    EXPECT_EQ(SUCCESS, ret);
    AudioService::GetInstance()->ResetAudioEndpoint();
    ret = AudioService::GetInstance()->OnProcessRelease(audioProcess, false);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test IsEndpointTypeVoip API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceIsEndpointTypeVoip_001
 * @tc.desc  : Test IsEndpointTypeVoip interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceIsEndpointTypeVoip_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    AudioDeviceDescriptor info(AudioDeviceDescriptor::DEVICE_INFO);
    config.rendererInfo.streamUsage = STREAM_USAGE_INVALID;
    config.capturerInfo.sourceType = SOURCE_TYPE_VOICE_COMMUNICATION;
    config.rendererInfo.originalFlag = AUDIO_FLAG_VOIP_FAST;
    bool ret = AudioService::GetInstance()->IsEndpointTypeVoip(config, info);
    EXPECT_EQ(true, ret);

    config.capturerInfo.sourceType = SOURCE_TYPE_INVALID;
    ret = AudioService::GetInstance()->IsEndpointTypeVoip(config, info);
    EXPECT_FALSE(ret);

    config.rendererInfo.streamUsage = STREAM_USAGE_VIDEO_COMMUNICATION;
    ret = AudioService::GetInstance()->IsEndpointTypeVoip(config, info);
    EXPECT_TRUE(ret);

    config.rendererInfo.streamUsage = STREAM_USAGE_VOICE_COMMUNICATION;
    ret = AudioService::GetInstance()->IsEndpointTypeVoip(config, info);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : Test GetCapturerBySessionID API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceGetCapturerBySessionID_001
 * @tc.desc  : Test GetCapturerBySessionID interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceGetCapturerBySessionID_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_RECORD;
    config.streamInfo.channels = STEREO;
    config.streamInfo.channelLayout = CH_LAYOUT_STEREO;
    config.streamType = STREAM_MUSIC;

    int32_t result;
    AudioService::GetInstance()->RemoveCapturer(-1);
    sptr<OHOS::AudioStandard::IpcStreamInServer> server = AudioService::GetInstance()->GetIpcStream(config, result);
    EXPECT_EQ(server, nullptr);

    auto ret = AudioService::GetInstance()->GetCapturerBySessionID(0);
    EXPECT_EQ(nullptr, ret);
}

/**
 * @tc.name  : Test ShouldBeDualTone API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceShouldBeDualTone_002
 * @tc.desc  : Test ShouldBeDualTone interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceShouldBeDualTone_002, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_RECORD;
    bool ret;
    ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_EQ(ret, false);
    config.audioMode = AUDIO_MODE_PLAYBACK;
    config.rendererInfo.streamUsage = STREAM_USAGE_RINGTONE;
    ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test FilterAllFastProcess API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceFilterAllFastProcess_001
 * @tc.desc  : Test FilterAllFastProcess interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceFilterAllFastProcess_001, TestSize.Level1)
{
    int32_t floatRet = 0;
    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);
    EXPECT_EQ(0, floatRet);
    AudioService::GetInstance()->FilterAllFastProcess();
}

/**
 * @tc.name  : Test GetDeviceInfoForProcess API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceGetDeviceInfoForProcess_001
 * @tc.desc  : Test GetDeviceInfoForProcess interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceGetDeviceInfoForProcess_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_PLAYBACK;
    AudioDeviceDescriptor deviceinfo(AudioDeviceDescriptor::DEVICE_INFO);
    deviceinfo = AudioService::GetInstance()->GetDeviceInfoForProcess(config);
    EXPECT_NE(deviceinfo.deviceRole_, INPUT_DEVICE);
    config.audioMode = AUDIO_MODE_RECORD;
    deviceinfo = AudioService::GetInstance()->GetDeviceInfoForProcess(config);
    EXPECT_NE(deviceinfo.deviceRole_, OUTPUT_DEVICE);
}

/**
 * @tc.name  : Test Dump API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceDump_001
 * @tc.desc  : Test Dump interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceDump_001, TestSize.Level1)
{
    int32_t floatRet = 0;
    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);
    EXPECT_EQ(0, floatRet);
    std::string dumpString = "This is Dump string";
    AudioService::GetInstance()->Dump(dumpString);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceSetNonInterruptMute_001
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceSetNonInterruptMute_001, TestSize.Level1)
{
    int32_t floatRet = 0;
    bool muteFlag = true;
    uint32_t sessionId = 0;

    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    AudioService::GetInstance()->SetNonInterruptMute(sessionId, muteFlag);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);
    AudioService::GetInstance()->SetNonInterruptMute(MAX_STREAMID - 1, muteFlag);
    EXPECT_EQ(0, floatRet);
}

/**
 * @tc.name  : Test OnProcessRelease API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceOnProcessRelease_001
 * @tc.desc  : Test OnProcessRelease interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceOnProcessRelease_001, TestSize.Level1)
{
    bool isSwitchStream = false;
    int32_t floatRet = 0;
    bool muteFlag = true;
    uint32_t sessionId = 0;

    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    AudioService::GetInstance()->SetNonInterruptMute(sessionId, muteFlag);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioDeviceDescriptor deviceInfo(AudioDeviceDescriptor::DEVICE_INFO);
    sptr<AudioProcessInServer> audioprocess =  AudioProcessInServer::Create(config, AudioService::GetInstance());
    EXPECT_NE(audioprocess, nullptr);
    audioprocess->Start();
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);

    int32_t ret = 0;
    ret = AudioService::GetInstance()->OnProcessRelease(audioprocess, isSwitchStream);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test OnProcessRelease API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceOnProcessRelease_002
 * @tc.desc  : Test OnProcessRelease interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceOnProcessRelease_002, TestSize.Level1)
{
    bool isSwitchStream = false;
    int32_t floatRet = 0;
    bool muteFlag = true;
    uint32_t sessionId = 0;

    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    AudioService::GetInstance()->SetNonInterruptMute(sessionId, muteFlag);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    config.rendererInfo.isLoopback = true;
    AudioDeviceDescriptor deviceInfo(AudioDeviceDescriptor::DEVICE_INFO);
    sptr<AudioProcessInServer> audioprocess =  AudioProcessInServer::Create(config, AudioService::GetInstance());
    EXPECT_NE(audioprocess, nullptr);
    audioprocess->Start();
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);

    int32_t ret = 0;
    ret = AudioService::GetInstance()->OnProcessRelease(audioprocess, isSwitchStream);
    EXPECT_EQ(ret, 0);
}


/**
 * @tc.name  : Test OnProcessRelease API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceOnProcessRelease_003
 * @tc.desc  : Test OnProcessRelease interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceOnProcessRelease_003, TestSize.Level1)
{
    bool isSwitchStream = false;
    int32_t floatRet = 0;
    bool muteFlag = true;
    uint32_t sessionId = 0;

    AudioService::GetInstance()->FilterAllFastProcess();
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->ResetAudioEndpoint();
    AudioService::GetInstance()->SetNonInterruptMute(sessionId, muteFlag);
    floatRet = AudioService::GetInstance()->GetMaxAmplitude(true);
    EXPECT_EQ(0, floatRet);

    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    config.audioMode = AUDIO_MODE_RECORD;
    config.capturerInfo.isLoopback = true;
    AudioDeviceDescriptor deviceInfo(AudioDeviceDescriptor::DEVICE_INFO);
    sptr<AudioProcessInServer> audioprocess =  AudioProcessInServer::Create(config, AudioService::GetInstance());
    EXPECT_NE(audioprocess, nullptr);
    audioprocess->Start();
    AudioService::GetInstance()->GetAudioProcess(config);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->workingConfig_.filterOptions.usages.emplace_back(STREAM_USAGE_MEDIA);
    AudioService::GetInstance()->OnInitInnerCapList(1);

    AudioService::GetInstance()->workingConfig_.filterOptions.pids.emplace_back(1);
    AudioService::GetInstance()->OnInitInnerCapList(1);
    AudioService::GetInstance()->OnUpdateInnerCapList(1);

    int32_t ret = 0;
    ret = AudioService::GetInstance()->OnProcessRelease(audioprocess, isSwitchStream);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test DelayCallReleaseEndpoint API
 * @tc.type  : FUNC
 * @tc.number: DelayCallReleaseEndpoint_001
 * @tc.desc  : Test DelayCallReleaseEndpoint interface.
 */
HWTEST(AudioServiceUnitTest, DelayCallReleaseEndpoint_001, TestSize.Level1)
{
    std::string endpointName;
    int32_t delayInMs = 1;
    AudioService *audioService = AudioService::GetInstance();
    audioService->DelayCallReleaseEndpoint(endpointName, delayInMs);
}

/**
 * @tc.name  : Test GetAudioEndpointForDevice API
 * @tc.type  : FUNC
 * @tc.number: GetAudioEndpointForDevice_001
 * @tc.desc  : Test GetAudioEndpointForDevice interface.
 */
HWTEST(AudioServiceUnitTest, GetAudioEndpointForDevice_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    AudioProcessConfig clientConfig;
    clientConfig.rendererInfo.streamUsage = STREAM_USAGE_VOICE_COMMUNICATION;
    clientConfig.rendererInfo.originalFlag = AUDIO_FLAG_VOIP_FAST;
    audioService->GetAudioProcess(clientConfig);
}

/**
 * @tc.name  : Test Dump API
 * @tc.type  : FUNC
 * @tc.number: Dump_001
 * @tc.desc  : Test Dump interface.
 */
HWTEST(AudioServiceUnitTest, Dump_001, TestSize.Level1)
{
    std::string dumpString = "abcdefg";
    AudioService *audioService = AudioService::GetInstance();
    audioService->Dump(dumpString);

    AudioProcessConfig processConfig;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<RendererInServer> rendererInServer =
        std::make_shared<RendererInServer>(processConfig, streamListener);

    std::shared_ptr<RendererInServer> renderer = rendererInServer;

    audioService->InsertRenderer(1, renderer);
    audioService->workingConfigs_[1];
    audioService->Dump(dumpString);
    audioService->RemoveRenderer(1);
}

/**
 * @tc.name  : Test GetMaxAmplitude API
 * @tc.type  : FUNC
 * @tc.number: GetMaxAmplitude_001
 * @tc.desc  : Test GetMaxAmplitude interface.
 */
HWTEST(AudioServiceUnitTest, GetMaxAmplitude_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    int ret = audioService->GetMaxAmplitude(true);
    EXPECT_EQ(0, ret);
}

/**
 * @tc.name  : Test GetCapturerBySessionID API
 * @tc.type  : FUNC
 * @tc.number: GetCapturerBySessionID_001
 * @tc.desc  : Test GetCapturerBySessionID interface.
 */
HWTEST(AudioServiceUnitTest, GetCapturerBySessionID_001, TestSize.Level1)
{
    uint32_t sessionID = 2;
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<CapturerInServer> renderer = nullptr;
    audioService->InsertCapturer(1, renderer);
    std::shared_ptr<CapturerInServer> ret = audioService->GetCapturerBySessionID(sessionID);
    EXPECT_EQ(nullptr, ret);
    audioService->RemoveCapturer(1);
}

/**
 * @tc.name  : Test GetCapturerBySessionID API
 * @tc.type  : FUNC
 * @tc.number: GetCapturerBySessionID_002
 * @tc.desc  : Test GetCapturerBySessionID interface.
 */
HWTEST(AudioServiceUnitTest, GetCapturerBySessionID_002, TestSize.Level1)
{
    AudioProcessConfig processConfig;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<CapturerInServer> capturerInServer =
        std::make_shared<CapturerInServer>(processConfig, streamListener);

    std::shared_ptr<CapturerInServer> capturer = capturerInServer;
    uint32_t sessionID = 1;
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<CapturerInServer> renderer = nullptr;
    audioService->InsertCapturer(1, renderer);
    std::shared_ptr<CapturerInServer> ret = audioService->GetCapturerBySessionID(sessionID);
    EXPECT_EQ(nullptr, ret);
    audioService->RemoveCapturer(1);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: SetNonInterruptMute_001
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, SetNonInterruptMute_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<RendererInServer> renderer = nullptr;
    audioService->InsertRenderer(1, renderer);
    audioService->SetNonInterruptMute(1, true);
    audioService->RemoveRenderer(1);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: SetNonInterruptMute_002
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, SetNonInterruptMute_002, TestSize.Level1)
{
    AudioProcessConfig processConfig;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<RendererInServer> rendererInServer =
        std::make_shared<RendererInServer>(processConfig, streamListener);

    std::shared_ptr<RendererInServer> renderer = rendererInServer;

    AudioService *audioService = AudioService::GetInstance();
    audioService->InsertRenderer(1, renderer);
    audioService->SetNonInterruptMute(1, true);
    audioService->RemoveRenderer(1);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: SetNonInterruptMute_003
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, SetNonInterruptMute_003, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<CapturerInServer> capturer = nullptr;
    audioService->InsertCapturer(1, capturer);
    audioService->SetNonInterruptMute(1, true);
    audioService->RemoveCapturer(1);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: SetNonInterruptMute_004
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, SetNonInterruptMute_004, TestSize.Level1)
{
    AudioProcessConfig processConfig;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<CapturerInServer> capturerInServer =
        std::make_shared<CapturerInServer>(processConfig, streamListener);

    std::shared_ptr<CapturerInServer> capturer = capturerInServer;

    AudioService *audioService = AudioService::GetInstance();
    audioService->InsertCapturer(1, capturer);
    audioService->SetNonInterruptMute(1, true);
    audioService->RemoveCapturer(1);
}

/**
 * @tc.name  : Test SetNonInterruptMute API
 * @tc.type  : FUNC
 * @tc.number: SetNonInterruptMute_005
 * @tc.desc  : Test SetNonInterruptMute interface.
 */
HWTEST(AudioServiceUnitTest, SetNonInterruptMute_005, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->SetNonInterruptMute(1, true);
    EXPECT_EQ(1, audioService->muteSwitchStreams_.count(1));
    audioService->SetNonInterruptMute(1, false);
    EXPECT_EQ(0, audioService->muteSwitchStreams_.count(1));
    audioService->mutedSessions_.insert(1);
    audioService->SetNonInterruptMute(1, true);
    EXPECT_EQ(1, audioService->mutedSessions_.count(1));
    audioService->SetNonInterruptMute(1, false);
    EXPECT_EQ(0, audioService->mutedSessions_.count(1));
}

/**
 * @tc.name  : Test SetOffloadMode API
 * @tc.type  : FUNC
 * @tc.number: SetOffloadMode_001
 * @tc.desc  : Test SetOffloadMode interface.
 */
HWTEST(AudioServiceUnitTest, SetOffloadMode_001, TestSize.Level1)
{
    uint32_t sessionId = 2;
    int32_t state = 1;
    bool isAppBack = true;
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<CapturerInServer> capturer = nullptr;
    audioService->InsertCapturer(1, capturer);
    int32_t ret = audioService->SetOffloadMode(sessionId, state, isAppBack);
    EXPECT_EQ(ERR_INVALID_INDEX, ret);
    audioService->RemoveCapturer(1);
}

/**
 * @tc.name  : Test CheckRenderSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckRenderSessionMuteState_001
 * @tc.desc  : Test CheckRenderSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckRenderSessionMuteState_001, TestSize.Level1)
{
    uint32_t sessionId = 2;
    AudioService *audioService = AudioService::GetInstance();
    audioService->UpdateMuteControlSet(sessionId, true);

    std::shared_ptr<RendererInServer> renderer = nullptr;
    audioService->CheckRenderSessionMuteState(sessionId, renderer);

    std::shared_ptr<CapturerInServer> capturer = nullptr;
    audioService->CheckCaptureSessionMuteState(sessionId, capturer);

    sptr<AudioProcessInServer> audioprocess = nullptr;
    audioService->CheckFastSessionMuteState(sessionId, audioprocess);

    audioService->RemoveIdFromMuteControlSet(sessionId);
    audioService->RemoveIdFromMuteControlSet(1);

    bool ret = audioService->IsExceedingMaxStreamCntPerUid(MEDIA_SERVICE_UID, 1, 0);
    EXPECT_EQ(ret, true);
    ret = audioService->IsExceedingMaxStreamCntPerUid(1, 1, 3);
    EXPECT_EQ(ret, false);
    int32_t mostAppUid = 1;
    int32_t mostAppNum = 1;
    audioService->GetCreatedAudioStreamMostUid(mostAppUid, mostAppNum);
}

/**
 * @tc.name  : Test CheckInnerCapForRenderer API
 * @tc.type  : FUNC
 * @tc.number: CheckInnerCapForRenderer_001
 * @tc.desc  : Test CheckInnerCapForRenderer interface.
 */
HWTEST(AudioServiceUnitTest, CheckInnerCapForRenderer_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    AudioProcessConfig processConfig;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<RendererInServer> rendererInServer =
        std::make_shared<RendererInServer>(processConfig, streamListener);

    std::shared_ptr<RendererInServer> renderer = rendererInServer;
    audioService->CheckInnerCapForRenderer(1, renderer);
    audioService->workingConfigs_[1];
    audioService->CheckInnerCapForRenderer(1, renderer);
    int32_t ret = audioService->OnCapturerFilterRemove(1, 1);
    EXPECT_EQ(SUCCESS, ret);
    audioService->workingConfigs_.clear();
    ret = audioService->OnCapturerFilterRemove(1, 1);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test OnCapturerFilterChange API
 * @tc.type  : FUNC
 * @tc.number: OnCapturerFilterChange_001
 * @tc.desc  : Test OnCapturerFilterChange interface.
 */
HWTEST(AudioServiceUnitTest, OnCapturerFilterChange_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    AudioPlaybackCaptureConfig newConfig;
    int32_t ret = audioService->OnCapturerFilterChange(1, newConfig, 1);
    EXPECT_EQ(ret, 0);
    audioService->workingConfigs_[1];
    ret = audioService->OnCapturerFilterChange(1, newConfig, 1);
    EXPECT_EQ(ret, 0);
    audioService->workingConfigs_.clear();
}

/**
 * @tc.name  : Test ShouldBeInnerCap API
 * @tc.type  : FUNC
 * @tc.number: ShouldBeInnerCap_001
 * @tc.desc  : Test ShouldBeInnerCap interface.
 */
HWTEST(AudioServiceUnitTest, ShouldBeInnerCap_001, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    AudioService *audioService = AudioService::GetInstance();
    int32_t ret = audioService->ShouldBeInnerCap(config, 0);
    EXPECT_FALSE(ret);
    audioService->workingConfig_.filterOptions.usages.push_back(STREAM_USAGE_MUSIC);
    ret = audioService->ShouldBeInnerCap(config, 0);
    EXPECT_FALSE(ret);
    audioService->workingConfig_.filterOptions.pids.push_back(1);
    ret = audioService->ShouldBeInnerCap(config, 0);
    EXPECT_FALSE(ret);
}
#endif

/**
 * @tc.name  : Test DelayCallReleaseEndpoint API
 * @tc.type  : FUNC
 * @tc.number: DelayCallReleaseEndpoint_002
 * @tc.desc  : Test DelayCallReleaseEndpoint interface.
 */
HWTEST(AudioServiceUnitTest, DelayCallReleaseEndpoint_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::string endpointName = "endpoint";
    std::shared_ptr<AudioEndpoint> audioEndpoint = nullptr;
    int32_t delayInMs = 1;
    audioService->endpointList_[endpointName] = audioEndpoint;
    audioService->DelayCallReleaseEndpoint(endpointName, delayInMs);
    EXPECT_EQ(audioService->endpointList_.count(endpointName), 1);
    audioService->endpointList_.erase(endpointName);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);

    audioService->currentRendererStreamCnt_ = 0;
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test CheckRenderSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckRenderSessionMuteState_002
 * @tc.desc  : Test CheckRenderSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckRenderSessionMuteState_002, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();
    audioService->UpdateMuteControlSet(sessionId, true);

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;
    std::shared_ptr<RendererInServer> rendererInServer =
        std::make_shared<RendererInServer>(processConfig, streamListener);
    std::shared_ptr<RendererInServer> renderer = rendererInServer;
    audioService->CheckRenderSessionMuteState(sessionId, renderer);

    audioService->RemoveIdFromMuteControlSet(sessionId);

    bool ret = audioService->IsExceedingMaxStreamCntPerUid(MEDIA_SERVICE_UID, 1, 0);
    EXPECT_EQ(ret, true);
    ret = audioService->IsExceedingMaxStreamCntPerUid(1, 1, 3);
    EXPECT_EQ(ret, false);
    int32_t mostAppUid = 1;
    int32_t mostAppNum = 1;
    audioService->GetCreatedAudioStreamMostUid(mostAppUid, mostAppNum);
}
/**
 * @tc.name  : Test CheckRenderSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: heckRenderSessionMuteState_003
 * @tc.desc  : Test CheckRenderSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckRenderSessionMuteState_003, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();
    audioService->UpdateMuteControlSet(sessionId, true);

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;

    std::shared_ptr<CapturerInServer> capturerInServer =
        std::make_shared<CapturerInServer>(processConfig, streamListener);
    std::shared_ptr<CapturerInServer> capturer = capturerInServer;
    audioService->CheckCaptureSessionMuteState(sessionId, capturer);

    audioService->RemoveIdFromMuteControlSet(sessionId);

    bool ret = audioService->IsExceedingMaxStreamCntPerUid(MEDIA_SERVICE_UID, 1, 0);
    EXPECT_EQ(ret, true);
    ret = audioService->IsExceedingMaxStreamCntPerUid(1, 1, 3);
    EXPECT_EQ(ret, false);
    int32_t mostAppUid = 1;
    int32_t mostAppNum = 1;
    audioService->GetCreatedAudioStreamMostUid(mostAppUid, mostAppNum);
}
/**
 * @tc.name  : Test CheckRenderSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckRenderSessionMuteState_004
 * @tc.desc  : Test CheckRenderSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckRenderSessionMuteState_004, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();
    audioService->UpdateMuteControlSet(sessionId, true);

    sptr<AudioProcessInServer> audioprocess = AudioProcessInServer::Create(processConfig, AudioService::GetInstance());;
    audioService->CheckFastSessionMuteState(sessionId, audioprocess);

    audioService->RemoveIdFromMuteControlSet(sessionId);

    bool ret = audioService->IsExceedingMaxStreamCntPerUid(MEDIA_SERVICE_UID, 1, 0);
    EXPECT_EQ(ret, true);
    ret = audioService->IsExceedingMaxStreamCntPerUid(1, 1, 3);
    EXPECT_EQ(ret, true);
    int32_t mostAppUid = 1;
    int32_t mostAppNum = 1;
    audioService->GetCreatedAudioStreamMostUid(mostAppUid, mostAppNum);
}

/**
 * @tc.name  : Test CheckRenderSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckRenderSessionMuteState_005
 * @tc.desc  : Test CheckRenderSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckRenderSessionMuteState_005, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;
    std::shared_ptr<RendererInServer> rendererInServer =
        std::make_shared<RendererInServer>(processConfig, streamListener);
    std::shared_ptr<RendererInServer> renderer = rendererInServer;
    audioService->muteSwitchStreams_.insert(sessionId);
    audioService->CheckRenderSessionMuteState(sessionId, renderer);
    EXPECT_EQ(audioService->mutedSessions_.count(sessionId), 0);
    audioService->RemoveIdFromMuteControlSet(sessionId);
}

/**
 * @tc.name  : Test CheckCapturerSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckCapturerSessionMuteState_006
 * @tc.desc  : Test CheckCapturerSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckCapturerSessionMuteState_001, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();

    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;
    std::shared_ptr<CapturerInServer> capturerInServer =
        std::make_shared<CapturerInServer>(processConfig, streamListener);
    std::shared_ptr<CapturerInServer> capturer = capturerInServer;
    audioService->muteSwitchStreams_.insert(sessionId);
    audioService->CheckCaptureSessionMuteState(sessionId, capturer);
    EXPECT_EQ(audioService->mutedSessions_.count(sessionId), 0);
    audioService->RemoveIdFromMuteControlSet(sessionId);
}

/**
 * @tc.name  : Test CheckFastSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: CheckFastSessionMuteState_006
 * @tc.desc  : Test CheckFastSessionMuteState interface.
 */
HWTEST(AudioServiceUnitTest, CheckFastSessionMuteState_001, TestSize.Level1)
{
    AudioProcessConfig processConfig;
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();

    sptr<AudioProcessInServer> audioprocess = AudioProcessInServer::Create(processConfig, AudioService::GetInstance());;
    audioService->CheckFastSessionMuteState(sessionId, audioprocess);

    audioService->muteSwitchStreams_.insert(sessionId);
    audioService->CheckFastSessionMuteState(sessionId, audioprocess);
    EXPECT_EQ(audioService->mutedSessions_.count(sessionId), 0);
    audioService->RemoveIdFromMuteControlSet(sessionId);
}

/**
 * @tc.name  : Test GetStandbyStatus API
 * @tc.type  : FUNC
 * @tc.number: GetStandbyStatus_001
 * @tc.desc  : Test GetStandbyStatus interface.
 */
HWTEST(AudioServiceUnitTest, GetStandbyStatus_001, TestSize.Level1)
{
    uint32_t sessionId = 100001;
    AudioService *audioService = AudioService::GetInstance();
    audioService->UpdateMuteControlSet(sessionId, true);
    bool isStandby = false;
    int64_t enterStandbyTime = 100000;
    int ret = audioService->GetStandbyStatus(sessionId, isStandby, enterStandbyTime);
    EXPECT_EQ(ret, ERR_INVALID_PARAM);
}
/**
 * @tc.name  : Test OnUpdateInnerCapList API
 * @tc.type  : FUNC
 * @tc.number: OnUpdateInnerCapList_001
 * @tc.desc  : Test OnUpdateInnerCapList interface.
 */
HWTEST(AudioServiceUnitTest, OnUpdateInnerCapList_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<RendererInServer> renderer = nullptr;
    std::vector<std::weak_ptr<RendererInServer>> rendererVector;
    rendererVector.push_back(renderer);
    int32_t innerCapId = 1;
    audioService->filteredRendererMap_.insert(std::make_pair(innerCapId, rendererVector));
    int32_t ret = audioService->OnUpdateInnerCapList(innerCapId);
    EXPECT_EQ(ret, SUCCESS);
}
/**
 * @tc.name  : Test DelayCallReleaseEndpoint API
 * @tc.type  : FUNC
 * @tc.number: DelayCallReleaseEndpoint_003
 * @tc.desc  : Test DelayCallReleaseEndpoint interface.
 */
HWTEST(AudioServiceUnitTest, DelayCallReleaseEndpoint_003, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->DelayCallReleaseEndpoint("endponit", 0);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test DelayCallReleaseEndpoint API
 * @tc.type  : FUNC
 * @tc.number: DelayCallReleaseEndpoint_004
 * @tc.desc  : Test DelayCallReleaseEndpoint interface.
 */
HWTEST(AudioServiceUnitTest, DelayCallReleaseEndpoint_004, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->releasingEndpointSet_.insert("endponit");
    audioService->DelayCallReleaseEndpoint("endponit", 1);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test EnableDualToneList API
 * @tc.type  : FUNC
 * @tc.number: EnableDualToneList_001
 * @tc.desc  : Test EnableDualToneList interface.
 */
HWTEST(AudioServiceUnitTest, EnableDualToneList_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<RendererInServer> renderer = nullptr;
    int32_t sessionId = 1;
    audioService->allRendererMap_.insert(std::make_pair(sessionId, renderer));
    int32_t ret = audioService->EnableDualToneList(sessionId);
    EXPECT_EQ(ret, SUCCESS);
}
/**
 * @tc.name  : Test DisableDualToneList API
 * @tc.type  : FUNC
 * @tc.number: DisableDualToneList_001
 * @tc.desc  : Test DisableDualToneList interface.
 */
HWTEST(AudioServiceUnitTest, DisableDualToneList_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    std::shared_ptr<RendererInServer> renderer = nullptr;
    audioService->filteredDualToneRendererMap_.push_back(renderer);
    int32_t sessionId = 1;
    int32_t ret = audioService->DisableDualToneList(sessionId);
    EXPECT_EQ(ret, SUCCESS);
}
/**
 * @tc.name  : Test UpdateAudioSinkState API
 * @tc.type  : FUNC
 * @tc.number: UpdateAudioSinkState_001
 * @tc.desc  : Test UpdateAudioSinkState interface.
 */
HWTEST(AudioServiceUnitTest, UpdateAudioSinkState_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->UpdateAudioSinkState(1, false);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test UpdateAudioSinkState API
 * @tc.type  : FUNC
 * @tc.number: UpdateAudioSinkState_002
 * @tc.desc  : Test UpdateAudioSinkState interface.
 */
HWTEST(AudioServiceUnitTest, UpdateAudioSinkState_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->UpdateAudioSinkState(1, true);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test ShouldBeDualTone API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceShouldBeDualTone_003
 * @tc.desc  : Test ShouldBeDualTone interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceShouldBeDualTone_003, TestSize.Level1)
{
    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_PLAYBACK;
    bool ret;
    ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_EQ(ret, false);
    config.audioMode = AUDIO_MODE_PLAYBACK;
    config.rendererInfo.streamUsage = STREAM_USAGE_RINGTONE;
    ret = AudioService::GetInstance()->ShouldBeDualTone(config);
    EXPECT_FALSE(ret);
}
/**
 * @tc.name  : Test CheckHibernateState API
 * @tc.type  : FUNC
 * @tc.number: CheckHibernateState_001
 * @tc.desc  : Test CheckHibernateState interface.
 */
HWTEST(AudioServiceUnitTest, CheckHibernateState_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->CheckHibernateState(true);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test CheckHibernateState API
 * @tc.type  : FUNC
 * @tc.number: CheckHibernateState_002
 * @tc.desc  : Test CheckHibernateState interface.
 */
HWTEST(AudioServiceUnitTest, CheckHibernateState_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->currentRendererStreamCnt_ = 0;
    audioService->CheckHibernateState(false);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test CheckHibernateState API
 * @tc.type  : FUNC
 * @tc.number: CheckHibernateState_003
 * @tc.desc  : Test CheckHibernateState interface.
 */
HWTEST(AudioServiceUnitTest, CheckHibernateState_003, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->allRunningSinks_.insert(1);
    audioService->currentRendererStreamCnt_ = 0;
    audioService->CheckHibernateState(true);

    AudioMode audioMode = AUDIO_MODE_PLAYBACK;
    audioService->SetIncMaxRendererStreamCnt(audioMode);
    int32_t res = audioService->GetCurrentRendererStreamCnt();
    EXPECT_EQ(res, 1);
}
/**
 * @tc.name  : Test UnsetOffloadMode API
 * @tc.type  : FUNC
 * @tc.number: UnsetOffloadMode_001
 * @tc.desc  : Test UnsetOffloadMode interface.
 */
HWTEST(AudioServiceUnitTest, UnsetOffloadMode_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    audioService->allRunningSinks_.insert(1);
    int ret = audioService->UnsetOffloadMode(1);
    EXPECT_EQ(ret, ERROR);
}

/**
 * @tc.name  : Test ReleaseProcess API
 * @tc.type  : FUNC
 * @tc.number: ReleaseProcess_001
 * @tc.desc  : Test ReleaseProcess interface.
 */
HWTEST(AudioServiceUnitTest, ReleaseProcess_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    std::string endpointName = "invalid_endpoint";
    int32_t delayTime = 0;
    audioService->ReleaseProcess(endpointName, delayTime);
}

/**
 * @tc.name  : Test GetReleaseDelayTime API
 * @tc.type  : FUNC
 * @tc.number: GetReleaseDelayTime_001
 * @tc.desc  : Test GetReleaseDelayTime interface.
 */
HWTEST(AudioServiceUnitTest, GetReleaseDelayTime_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpoint> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_VOIP_MMAP,
        123, clientConfig);
    EXPECT_NE(nullptr, endpoint);

    bool isSwitchStream = false;
    int ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, false);
    EXPECT_EQ(ret, VOIP_ENDPOINT_RELEASE_DELAY_TIME);
    ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, true);
    EXPECT_EQ(ret, VOIP_REC_ENDPOINT_RELEASE_DELAY_TIME);
}

/**
 * @tc.name  : Test GetReleaseDelayTime API
 * @tc.type  : FUNC
 * @tc.number: GetReleaseDelayTime_002
 * @tc.desc  : Test GetReleaseDelayTime interface.
 */
HWTEST(AudioServiceUnitTest, GetReleaseDelayTime_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpoint> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_MMAP,
        123, clientConfig);
    EXPECT_NE(nullptr, endpoint);

    bool isSwitchStream = false;
    int ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, false);
    EXPECT_EQ(ret, NORMAL_ENDPOINT_RELEASE_DELAY_TIME_MS);
    ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, true);
    EXPECT_EQ(ret, NORMAL_ENDPOINT_RELEASE_DELAY_TIME_MS);
}

/**
 * @tc.name  : Test GetReleaseDelayTime API
 * @tc.type  : FUNC
 * @tc.number: GetReleaseDelayTime_003
 * @tc.desc  : Test GetReleaseDelayTime interface.
 */
HWTEST(AudioServiceUnitTest, GetReleaseDelayTime_003, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpointInner> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_MMAP,
        123, clientConfig);
    EXPECT_NE(nullptr, endpoint);

    endpoint->deviceInfo_.deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;

    bool isSwitchStream = false;
    int ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, false);
    EXPECT_EQ(ret, A2DP_ENDPOINT_RELEASE_DELAY_TIME);
    ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, true);
    EXPECT_EQ(ret, A2DP_ENDPOINT_RELEASE_DELAY_TIME);
    isSwitchStream = true;
    ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, false);
    EXPECT_EQ(ret, A2DP_ENDPOINT_RE_CREATE_RELEASE_DELAY_TIME);
    ret = audioService->GetReleaseDelayTime(endpoint, isSwitchStream, true);
    EXPECT_EQ(ret, A2DP_ENDPOINT_RE_CREATE_RELEASE_DELAY_TIME);
}

/**
 * @tc.name  : Test GetStandbyStatus API
 * @tc.type  : FUNC
 * @tc.number: GetStandbyStatus_002
 * @tc.desc  : Test GetStandbyStatus interface.
 */
HWTEST(AudioServiceUnitTest, GetStandbyStatus_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig processConfig;

    std::shared_ptr<RendererInServer> rendererInServer1 = nullptr;

    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();
    EXPECT_NE(streamListenerHolder, nullptr);
    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;
    std::shared_ptr<RendererInServer> rendererInServer2 =
        std::make_shared<RendererInServer>(processConfig, streamListener);
    EXPECT_NE(rendererInServer2, nullptr);

    audioService->allRendererMap_.clear();
    audioService->allRendererMap_.insert(std::make_pair(0, rendererInServer1));
    audioService->allRendererMap_.insert(std::make_pair(1, rendererInServer2));

    uint32_t sessionId = 0;
    bool isStandby = true;
    int64_t enterStandbyTime = 0;
    int ret = audioService->GetStandbyStatus(sessionId, isStandby, enterStandbyTime);
    EXPECT_EQ(ret, ERR_INVALID_PARAM);
    sessionId = 1;
    ret = audioService->GetStandbyStatus(sessionId, isStandby, enterStandbyTime);
    EXPECT_EQ(ret, SUCCESS);

    audioService->allRendererMap_.clear();
}

/**
 * @tc.name  : Test RemoveRenderer API
 * @tc.type  : FUNC
 * @tc.number: RemoveRenderer_001
 * @tc.desc  : Test RemoveRenderer interface.
 */
HWTEST(AudioServiceUnitTest, RemoveRenderer_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->allRendererMap_.clear();

    uint32_t sessionId = 0;
    audioService->RemoveRenderer(sessionId);
}

/**
 * @tc.name  : Test AddFilteredRender API
 * @tc.type  : FUNC
 * @tc.number: AddFilteredRender_001
 * @tc.desc  : Test AddFilteredRender interface.
 */
HWTEST(AudioServiceUnitTest, AddFilteredRender_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->filteredRendererMap_.clear();

    int32_t innerCapId = 0;
    std::shared_ptr<RendererInServer> renderer = nullptr;
    audioService->AddFilteredRender(innerCapId, renderer);
}

/**
 * @tc.name  : Test CheckInnerCapForRenderer API
 * @tc.type  : FUNC
 * @tc.number: CheckInnerCapForRenderer_002
 * @tc.desc  : Test CheckInnerCapForRenderer interface.
 */
HWTEST(AudioServiceUnitTest, CheckInnerCapForRenderer_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->workingConfigs_.clear();

    uint32_t sessionId = 0;
    std::shared_ptr<RendererInServer> renderer = nullptr;
    audioService->CheckInnerCapForRenderer(sessionId, renderer);
}

/**
 * @tc.name  : Test ShouldBeInnerCap API
 * @tc.type  : FUNC
 * @tc.number: ShouldBeInnerCap_002
 * @tc.desc  : Test ShouldBeInnerCap interface.
 */
HWTEST(AudioServiceUnitTest, ShouldBeInnerCap_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->workingConfigs_.clear();

    AudioProcessConfig rendererConfig;
    rendererConfig.privacyType = AudioPrivacyType::PRIVACY_TYPE_PUBLIC;
    int32_t innerCapId = 1;
    bool ret = audioService->ShouldBeInnerCap(rendererConfig, innerCapId);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test ShouldBeInnerCap API
 * @tc.type  : FUNC
 * @tc.number: ShouldBeInnerCap_003
 * @tc.desc  : Test ShouldBeInnerCap interface.
 */
HWTEST(AudioServiceUnitTest, ShouldBeInnerCap_003, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig rendererConfig;
    rendererConfig.privacyType = AudioPrivacyType::PRIVACY_TYPE_PRIVATE;
    std::set<int32_t> beCapIds;
    bool ret = audioService->ShouldBeInnerCap(rendererConfig, beCapIds);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test CheckShouldCap API
 * @tc.type  : FUNC
 * @tc.number: CheckShouldCap_001
 * @tc.desc  : Test CheckShouldCap interface.
 */
HWTEST(AudioServiceUnitTest, CheckShouldCap_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->filteredRendererMap_.clear();

    AudioPlaybackCaptureConfig audioPlaybackCaptureConfig;
    audioPlaybackCaptureConfig.filterOptions.usages.push_back(STREAM_USAGE_MEDIA);
    audioService->workingConfigs_.clear();
    audioService->workingConfigs_.insert(std::make_pair(1, audioPlaybackCaptureConfig));

    int32_t innerCapId = 0;
    AudioProcessConfig rendererConfig;
    bool ret = audioService->CheckShouldCap(rendererConfig, innerCapId);
    EXPECT_EQ(ret, false);
    innerCapId = 1;
    ret = audioService->CheckShouldCap(rendererConfig, innerCapId);
    EXPECT_EQ(ret, false);
    audioPlaybackCaptureConfig.filterOptions.pids.push_back(1);
    ret = audioService->CheckShouldCap(rendererConfig, innerCapId);
    EXPECT_EQ(ret, false);

    audioService->workingConfigs_.clear();
}

/**
 * @tc.name  : Test FilterAllFastProcess API
 * @tc.type  : FUNC
 * @tc.number: FilterAllFastProcess_002
 * @tc.desc  : Test FilterAllFastProcess interface.
 */
HWTEST(AudioServiceUnitTest, FilterAllFastProcess_002, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioProcessConfig config = {};
    config.audioMode = AUDIO_MODE_PLAYBACK;
    sptr<AudioProcessInServer> audioprocess =  AudioProcessInServer::Create(config, AudioService::GetInstance());
    EXPECT_NE(audioprocess, nullptr);

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpointInner> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_VOIP_MMAP,
        123, clientConfig);
    EXPECT_NE(endpoint, nullptr);
    endpoint->deviceInfo_.deviceRole_ = OUTPUT_DEVICE;

    audioService->linkedPairedList_.clear();
    audioService->linkedPairedList_.push_back(std::make_pair(audioprocess, endpoint));

    audioService->endpointList_.clear();
    audioService->endpointList_.insert(std::make_pair("endpoint", endpoint));

    audioService->FilterAllFastProcess();

    audioService->linkedPairedList_.clear();
    audioService->endpointList_.clear();
}

/**
 * @tc.name  : Test CheckDisableFastInner API
 * @tc.type  : FUNC
 * @tc.number: CheckDisableFastInner_001
 * @tc.desc  : Test CheckDisableFastInner interface.
 */
HWTEST(AudioServiceUnitTest, CheckDisableFastInner_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    AudioPlaybackCaptureConfig audioPlaybackCaptureConfig;
    audioService->workingConfigs_.clear();
    audioService->workingConfigs_.insert(std::make_pair(1, audioPlaybackCaptureConfig));

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpointInner> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_VOIP_MMAP,
        123, clientConfig);
    int32_t ret = audioService->CheckDisableFastInner(endpoint);
    EXPECT_EQ(ret, SUCCESS);

    audioService->workingConfigs_.clear();
}

/**
 * @tc.name  : Test HandleFastCapture API
 * @tc.type  : FUNC
 * @tc.number: HandleFastCapture_001
 * @tc.desc  : Test HandleFastCapture interface.
 */
HWTEST(AudioServiceUnitTest, HandleFastCapture_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    audioService->filteredRendererMap_.clear();

    std::set<int32_t> captureIds = {1};
    AudioProcessConfig config = {};
    sptr<AudioProcessInServer> audioprocess =  AudioProcessInServer::Create(config, AudioService::GetInstance());
    EXPECT_NE(audioprocess, nullptr);

    AudioProcessConfig clientConfig = {};
    std::shared_ptr<AudioEndpointInner> endpoint = std::make_shared<AudioEndpointInner>(AudioEndpoint::TYPE_VOIP_MMAP,
        123, clientConfig);
    EXPECT_NE(endpoint, nullptr);

    int32_t ret = audioService->HandleFastCapture(captureIds, audioprocess, endpoint);
    EXPECT_EQ(ret, SUCCESS);
}

/**
 * @tc.name  : Test OnInitInnerCapList API
 * @tc.type  : FUNC
 * @tc.number: OnInitInnerCapList_001
 * @tc.desc  : Test OnInitInnerCapList interface.
 */
HWTEST(AudioServiceUnitTest, OnInitInnerCapList_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);

    std::shared_ptr<RendererInServer> rendererInServer1 = nullptr;

    AudioProcessConfig processConfig;
    std::shared_ptr<StreamListenerHolder> streamListenerHolder =
        std::make_shared<StreamListenerHolder>();
    EXPECT_NE(streamListenerHolder, nullptr);
    std::weak_ptr<IStreamListener> streamListener = streamListenerHolder;
    std::shared_ptr<RendererInServer> rendererInServer2 =
        std::make_shared<RendererInServer>(processConfig, streamListener);
    EXPECT_NE(rendererInServer2, nullptr);

    audioService->allRendererMap_.clear();
    audioService->allRendererMap_.insert(std::make_pair(0, rendererInServer1));
    audioService->allRendererMap_.insert(std::make_pair(1, rendererInServer2));

    int32_t innerCapId = 0;
    int32_t ret = audioService->OnInitInnerCapList(innerCapId);
    EXPECT_EQ(ret, SUCCESS);

    audioService->allRendererMap_.clear();
}

/**
 * @tc.name  : Test SetDefaultAdapterEnable API
 * @tc.type  : FUNC
 * @tc.number: SetDefaultAdapterEnable_001
 * @tc.desc  : Test SetDefaultAdapterEnable interface.
 */
HWTEST(AudioServiceUnitTest, SetDefaultAdapterEnable_001, TestSize.Level1)
{
    AudioService *audioService = AudioService::GetInstance();
    EXPECT_NE(audioService, nullptr);
    bool isEnable = false;
    audioService->SetDefaultAdapterEnable(isEnable);
    bool result = audioService->GetDefaultAdapterEnable();
    EXPECT_EQ(result, isEnable);
}

/*
 * @tc.name  : Test SetSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceSetSessionMuteState_001
 * @tc.desc  : Test RegisterMuteStateChangeCallback whether callback can invoke.
 */
HWTEST(AudioServiceUnitTest, AudioServiceSetSessionMuteState_001, TestSize.Level1)
{
    bool muteFlag = true;
    uint32_t sessionId = 0;
    uint32_t sessionId2 = 1;
    uint32_t sessionId3 = 3;
    bool testFlag = false;

    auto service = AudioService::GetInstance();
    ASSERT_NE(service, nullptr);
    service->SetSessionMuteState(sessionId, true, muteFlag);
    service->RegisterMuteStateChangeCallback(sessionId2, [&](bool flag) {
        testFlag = flag;
    });
    EXPECT_EQ(testFlag, false);
    service->RegisterMuteStateChangeCallback(sessionId, [&](bool flag) {
        testFlag = flag;
    });
    EXPECT_EQ(testFlag, false);

    testFlag = false;
    service->SetLatestMuteState(sessionId3, muteFlag);
    EXPECT_FALSE(testFlag);

    service->RegisterMuteStateChangeCallback(sessionId3, [&](bool flag) {
        testFlag = flag;
    });
    service->SetSessionMuteState(sessionId3, true, muteFlag);
    service->SetLatestMuteState(sessionId3, muteFlag);
    EXPECT_EQ(testFlag, muteFlag);
}

/**
 * @tc.name  : Test SetSessionMuteState API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceSetSessionMuteState_002
 * @tc.desc  : Test SetSessionMuteState interface .
 */
HWTEST(AudioServiceUnitTest, AudioServiceSetSessionMuteState_002, TestSize.Level1)
{
    bool muteFlag = true;
    uint32_t sessionId = 0;
    auto service = AudioService::GetInstance();
    ASSERT_NE(service, nullptr);
    service->SetSessionMuteState(sessionId, true, muteFlag);
    EXPECT_TRUE(service->muteStateMap_.count(sessionId) != 0);
    service->SetSessionMuteState(sessionId, false, muteFlag);
    EXPECT_TRUE(service->muteStateMap_.count(sessionId) == 0);
}

/**
 * @tc.name  : Test GetCurrentLoopbackStreamCnt API
 * @tc.type  : FUNC
 * @tc.number: AudioServiceLoopbackStreamCnt_001,
 * @tc.desc  : Test GetCurrentLoopbackStreamCnt interface.
 */
HWTEST(AudioServiceUnitTest, AudioServiceLoopbackStreamCnt_001, TestSize.Level1)
{
    int32_t rendererCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_PLAYBACK);
    int32_t capturerCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_RECORD);
    EXPECT_EQ(rendererCnt, 0);
    EXPECT_EQ(capturerCnt, 0);
    AudioService::GetInstance()->SetIncMaxLoopbackStreamCnt(AUDIO_MODE_PLAYBACK);
    AudioService::GetInstance()->SetIncMaxLoopbackStreamCnt(AUDIO_MODE_RECORD);
    rendererCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_PLAYBACK);
    capturerCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_RECORD);
    EXPECT_EQ(rendererCnt, 1);
    EXPECT_EQ(capturerCnt, 1);
    AudioService::GetInstance()->SetDecMaxLoopbackStreamCnt(AUDIO_MODE_PLAYBACK);
    AudioService::GetInstance()->SetDecMaxLoopbackStreamCnt(AUDIO_MODE_RECORD);
    rendererCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_PLAYBACK);
    capturerCnt = AudioService::GetInstance()->GetCurrentLoopbackStreamCnt(AUDIO_MODE_RECORD);
    EXPECT_EQ(rendererCnt, 0);
    EXPECT_EQ(capturerCnt, 0);
}
} // namespace AudioStandard
} // namespace OHOS