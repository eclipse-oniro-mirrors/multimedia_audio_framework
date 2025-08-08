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
#include <gtest/gtest.h>
#include "audio_renderer_sink.h"
#include "audio_errors.h"

using namespace testing::ext;
namespace OHOS {
namespace AudioStandard {
const std::string DIRECT_SINK_NAME = "direct";
const std::string VOIP_SINK_NAME = "voip";
const std::string USB_SINK_NAME = "usb";
const char *SINK_ADAPTER_NAME = "primary";
class AudioDirectSinkUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

protected:
    AudioRendererSink *sink;
    AudioRendererSink *voipSink;
    AudioRendererSink *usbSink;
};

void AudioDirectSinkUnitTest::SetUpTestCase(void)
{
    // input testsuit setup step，setup invoked before all testcases
}

void AudioDirectSinkUnitTest::TearDownTestCase(void)
{
    // input testsuit teardown step，teardown invoked after all testcases
}

void AudioDirectSinkUnitTest::SetUp(void)
{
    sink = AudioRendererSink::GetInstance(DIRECT_SINK_NAME);
    voipSink = AudioRendererSink::GetInstance(VOIP_SINK_NAME);
    usbSink = AudioRendererSink::GetInstance(USB_SINK_NAME);
}

void AudioDirectSinkUnitTest::TearDown(void)
{
    if (sink && sink->IsInited()) {
        sink->DeInit();
    }
    if (voipSink && voipSink->IsInited()) {
        voipSink->DeInit();
    }
}

/**
 * @tc.name  : Test Audio Direct Sink Create
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkCreate_001
 * @tc.desc  : Test audio direct sink create success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkCreate_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
}

/**
 * @tc.name  : Test Audio Direct Sink
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkInit_001
 * @tc.desc  : Test audio direct sink init(setVolume) success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkInit_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    float volume = 1.0f;
    ret = sink->SetVolume(volume, volume);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Sink State
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkState_001
 * @tc.desc  : Test direct sink state(init->start->stop) success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkState_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Sink Init State
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkState_002
 * @tc.desc  : Test direct sink init state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkState_002, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    bool isInited = sink->IsInited();
    EXPECT_EQ(false, isInited);

    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);

    isInited = sink->IsInited();
    EXPECT_EQ(true, isInited);

    sink->DeInit();

    isInited = sink->IsInited();
    EXPECT_EQ(false, isInited);

    // Continuous execution init
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    isInited = sink->IsInited();
    EXPECT_EQ(true, isInited);
}

/**
 * @tc.name  : Test Direct Sink Start State
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkState_003
 * @tc.desc  : Test direct sink start state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkState_003, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);

    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);

    // Continuous execution start
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Sink Resume State
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkState_004
 * @tc.desc  : Test direct sink resume state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkState_004, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Resume();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Direct Sink Attribute
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkSetAttribute_001
 * @tc.desc  : Test audio direct sink attribute(sampleRate) success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkSetAttribute_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_WIRED_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Direct Sink Attribute
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkSetAttribute_002
 * @tc.desc  : Test audio direct sink attribute(deviceType) success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkSetAttribute_002, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Sink RenderFrame
 * @tc.type  : FUNC
 * @tc.number: RenderFrame_001
 * @tc.desc  : Test audio sink RenderFrame when audioMonoState_ is false, audioBalanceState_ is false
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkRenderFrame_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    uint64_t written = 0;
    std::vector<char> audioBuffer{'8', '8', '8', '8', '8', '8', '8', '8'};
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ('8', audioBuffer[0]);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Sink RenderFrame
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkRenderFrame_002
 * @tc.desc  : Test audio sink of each format type RenderFrame when audioMonoState_ is true, audioBalanceState_ is
 *          true success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkRenderFrame_002, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    uint64_t written = 0;
    sink->SetAudioMonoState(true);
    sink->SetAudioBalanceValue(1.0f);
    sink->SetAudioBalanceValue(-1.0f);
    std::vector<char> audioBuffer{'8', '8', '8', '8', '8', '8', '8', '8'};
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    attr.format = HdiAdapterFormat::SAMPLE_U8;
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    attr.format = HdiAdapterFormat::SAMPLE_S16;
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Sink RenderFrame
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkRenderFrame_003
 * @tc.desc  : Test audio sink of each format type RenderFrame when audioMonoState_ is true, audioBalanceState_ is
 *          true success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkRenderFrame_003, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    uint64_t written = 0;
    sink->SetAudioMonoState(true);
    sink->SetAudioBalanceValue(-1.0f);
    std::vector<char> audioBuffer{'8', '8', '8', '8', '8', '8', '8', '8'};
    attr.format = HdiAdapterFormat::SAMPLE_S24;
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    attr.format = HdiAdapterFormat::SAMPLE_S32;
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    attr.format = HdiAdapterFormat::INVALID_WIDTH;
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->RenderFrame(*audioBuffer.data(), audioBuffer.size(), written);
    EXPECT_EQ(SUCCESS, ret);

    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Direct Sink SetVolume
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkSetVolume_001
 * @tc.desc  : Test audio direct sink SetVolume success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkSetVolume_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->SetVolume(0.0f, 0.0f);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->SetVolume(0.0f, 1.0f);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->SetVolume(1.0f, 0.0f);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->SetVolume(1.0f, 1.0f);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Audio Direct Sink SetVolume
 * @tc.type  : FUNC
 * @tc.number: DirectAudioSinkMix_001
 * @tc.desc  : Test audio direct sink Start, Resume, Pause, Reset, Flush, GetPresentationPosition.
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectAudioSinkMix_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->sink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 192000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S32;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_USB_HEADSET;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    uint64_t frame = 10;
    int64_t timeSec = 10;
    int64_t timeNanoSec = 10;
    int32_t ret = sink->GetPresentationPosition(frame, timeSec, timeNanoSec);
    EXPECT_EQ(ERR_INVALID_HANDLE, ret);
    ret = sink->Reset();
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
    ret = sink->Flush();
    EXPECT_EQ(ERR_OPERATION_FAILED, ret);
    ret = sink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Stop();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Resume();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->Pause();
    EXPECT_NE(SUCCESS, ret);
    ret = sink->Resume();
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->GetPresentationPosition(frame, timeSec, timeNanoSec);
    EXPECT_EQ(SUCCESS, ret);
    ret = sink->SetPaPower(1);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Create Direct Voip Sink
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkCreate_001
 * @tc.desc  : Test create direct voip sink success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkCreate_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
}

/**
 * @tc.name  : Test Init Direct Voip Sink
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkInit_001
 * @tc.desc  : Test init direct voip sink success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkInit_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_SPEAKER;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    float volume = 1.0f;
    ret = voipSink->SetVolume(volume, volume);
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Voip Sink State
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkState_001
 * @tc.desc  : Test direct voip sink state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkState_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Voip Sink init State
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkState_002
 * @tc.desc  : Test direct voip sink init state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkState_002, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    bool isInited = voipSink->IsInited();
    EXPECT_EQ(false, isInited);

    int32_t ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);

    isInited = voipSink->IsInited();
    EXPECT_EQ(true, isInited);

    voipSink->DeInit();

    isInited = voipSink->IsInited();
    EXPECT_EQ(false, isInited);

    // Continuous execution init
    ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    isInited = voipSink->IsInited();
    EXPECT_EQ(true, isInited);
}

/**
 * @tc.name  : Test Direct Voip Sink Start State
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkState_003
 * @tc.desc  : Test direct voip sink start state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkState_003, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);

    ret = voipSink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Stop();
    EXPECT_EQ(SUCCESS, ret);

    // Continuous execution start
    ret = voipSink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Stop();
    EXPECT_EQ(SUCCESS, ret);
}

/**
 * @tc.name  : Test Direct Voip Sink Resume State
 * @tc.type  : FUNC
 * @tc.number: DirectVoipAudioSinkState_004
 * @tc.desc  : Test direct voip sink resume state success
 */
HWTEST_F(AudioDirectSinkUnitTest, DirectVoipAudioSinkState_004, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->voipSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = voipSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Start();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Resume();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->Stop();
    EXPECT_EQ(SUCCESS, ret);
    ret = voipSink->SetPaPower(1);
    EXPECT_EQ(SUCCESS, ret);
}

std::string CreateUsbAttrInfo(const std::string sinkFormat)
{
    return "sink_rate:48000;sink_format:" + sinkFormat + ";";
}

/**
 * @tc.name  : Test Usb Sink Resume State
 * @tc.type  : FUNC
 * @tc.number: UsbAudioSinkPreload_001
 * @tc.desc  : Test Usb sink Preload state success
 */
HWTEST_F(AudioDirectSinkUnitTest, UsbAudioSinkPreload_001, TestSize.Level1)
{
    EXPECT_NE(nullptr, this->usbSink);
    IAudioSinkAttr attr = {};
    attr.adapterName = SINK_ADAPTER_NAME;
    attr.sampleRate = 48000;
    attr.channel = 2;
    attr.format = HdiAdapterFormat::SAMPLE_S16;
    attr.channelLayout = 3;
    attr.deviceType = DEVICE_TYPE_BLUETOOTH_SCO;
    attr.volume = 1.0f;
    attr.openMicSpeaker = 1;
    int32_t ret = usbSink->Init(attr);
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->Preload(CreateUsbAttrInfo(""));
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->Preload(CreateUsbAttrInfo("sAUDIO_FORMAT_PCM_16_BIT"));
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->Preload(CreateUsbAttrInfo("AUDIO_FORMAT_PCM_24_BIT"));
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->Preload(CreateUsbAttrInfo("AUDIO_FORMAT_PCM_32_BIT"));
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->SetPaPower(1);
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->SetPaPower(0);
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->SetPaPower(0);
    EXPECT_EQ(SUCCESS, ret);
    ret = usbSink->SetPaPower(1);
    EXPECT_EQ(SUCCESS, ret);
}
} // namespace AudioStandard
} // namespace OHOS