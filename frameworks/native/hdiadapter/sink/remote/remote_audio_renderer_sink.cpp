/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "remote_audio_renderer_sink.h"

#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <list>
#include <string>
#include <sstream>
#include <unistd.h>
#include <map>
#include "securec.h"

#include "audio_manager.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_utils.h"

using namespace std;

namespace OHOS {
namespace AudioStandard {
namespace {
const int32_t HALF_FACTOR = 2;
const float DEFAULT_VOLUME_LEVEL = 1.0f;
const uint32_t AUDIO_CHANNELCOUNT = 2;
const uint32_t AUDIO_SAMPLE_RATE_48K = 48000;
const uint32_t DEEP_BUFFER_RENDER_PERIOD_SIZE = 4096;
const uint32_t INT_32_MAX = 0x7fffffff;
const uint32_t PCM_8_BIT = 8;
const uint32_t PCM_16_BIT = 16;
const uint32_t INTERNAL_OUTPUT_STREAM_ID = 0;
constexpr int32_t PARAMS_RENDER_STATE_NUM = 2;
constexpr int32_t EVENT_DES_SIZE = 60;
constexpr int32_t RENDER_STATE_CONTENT_DES_SIZE = 60;
#ifdef FEATURE_DISTRIBUTE_AUDIO
const uint32_t PARAM_VALUE_LENTH = 20;
#endif
}
class RemoteAudioRendererSinkInner : public RemoteAudioRendererSink {
public:
    explicit RemoteAudioRendererSinkInner(const std::string &deviceNetworkId);
    ~RemoteAudioRendererSinkInner();

    int32_t Init(IAudioSinkAttr attr) override;
    bool IsInited(void) override;
    void DeInit(void) override;

    int32_t Start(void) override;
    int32_t Stop(void) override;
    int32_t Flush(void) override;
    int32_t Reset(void) override;
    int32_t Pause(void) override;
    int32_t Resume(void) override;

    int32_t RenderFrame(char &data, uint64_t len, uint64_t &writeLen) override;
    int32_t SetVolume(float left, float right) override;
    int32_t GetVolume(float &left, float &right) override;
    int32_t SetVoiceVolume(float volume) override;
    int32_t GetTransactionId(uint64_t *transactionId) override;
    int32_t GetLatency(uint32_t *latency) override;
    int32_t SetAudioScene(AudioScene audioScene, DeviceType activeDevice) override;
    int32_t SetOutputRoute(DeviceType deviceType) override;
    void SetAudioParameter(const AudioParamKey key, const std::string& condition, const std::string& value) override;
    std::string GetAudioParameter(const AudioParamKey key, const std::string& condition) override;
    void SetAudioMonoState(bool audioMono) override;
    void SetAudioBalanceValue(float audioBalance) override;
    void RegisterParameterCallback(IAudioSinkCallback* callback) override;

    int32_t OpenOutput(DeviceType outputDevice);
    static int32_t ParamEventCallback(AudioExtParamKey key, const char *condition, const char *value, void *reserved,
        void *cookie);
    std::string GetNetworkId();
    IAudioSinkCallback* GetParamCallback();

private:
    IAudioSinkAttr attr_;
    std::string deviceNetworkId_;
    std::atomic<bool> rendererInited_ = false;
    std::atomic<bool> isRenderCreated_ = false;
    std::atomic<bool> started_ = false;
    std::atomic<bool> paused_ = false;
    float leftVolume_ = DEFAULT_VOLUME_LEVEL;
    float rightVolume_ = DEFAULT_VOLUME_LEVEL;
    int32_t routeHandle_ = -1;
    int32_t openSpeaker_ = -1;
    std::string adapterNameCase_;
    struct AudioManager *audioManager_;
    struct AudioAdapter *audioAdapter_ = nullptr;
    struct AudioRender *audioRender_ = nullptr;
    struct AudioPort audioPort_;
    IAudioSinkCallback* callback_ = nullptr;
    bool paramCallbackRegistered_ = false;

    int32_t GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size, const char *networkId);
    int32_t CreateRender(const struct AudioPort &renderPort);
    AudioFormat ConverToHdiFormat(AudioSampleFormat format);
    struct AudioManager *GetAudioManager();
#ifdef DEBUG_DUMP_FILE
    FILE *pfd;
#endif // DEBUG_DUMP_FILE
};

RemoteAudioRendererSinkInner::RemoteAudioRendererSinkInner(const std::string &deviceNetworkId)
{
    AUDIO_INFO_LOG("Constract.");
    attr_ = {};
    this->deviceNetworkId_ = deviceNetworkId;
    audioManager_ = GetAudioManager();
#ifdef DEBUG_DUMP_FILE
    pfd = nullptr;
#endif // DEBUG_DUMP_FILE
}

RemoteAudioRendererSinkInner::~RemoteAudioRendererSinkInner()
{
    if (rendererInited_.load()) {
        RemoteAudioRendererSinkInner::DeInit();
    } else {
        AUDIO_INFO_LOG("RemoteAudioRendererSink has already DeInit.");
    }
}

std::map<std::string, RemoteAudioRendererSinkInner *> allsinks;
RemoteAudioRendererSink *RemoteAudioRendererSink::GetInstance(const char *deviceNetworkId)
{
    AUDIO_INFO_LOG("GetInstance.");
    RemoteAudioRendererSinkInner *audioRenderer = nullptr;
    if (deviceNetworkId == nullptr) {
        return audioRenderer;
    }
    // check if it is in our map
    std::string deviceName = deviceNetworkId;
    if (allsinks.count(deviceName)) {
        return allsinks[deviceName];
    } else {
        audioRenderer = new(std::nothrow) RemoteAudioRendererSinkInner(deviceName);
        AUDIO_DEBUG_LOG("new Daudio device sink:[%{public}s]", deviceNetworkId);
        allsinks[deviceName] = audioRenderer;
    }
    CHECK_AND_RETURN_RET_LOG((audioRenderer != nullptr), nullptr, "null audioRenderer!");
    return audioRenderer;
}

void RemoteAudioRendererSinkInner::RegisterParameterCallback(IAudioSinkCallback* callback)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: register params callback");
    callback_ = callback;
    if (paramCallbackRegistered_) {
        return;
    }
#ifdef FEATURE_DISTRIBUTE_AUDIO
    // register to adapter
    ParamCallback adapterCallback = &RemoteAudioRendererSinkInner::ParamEventCallback;
    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::RegisterParameterCallback audioAdapter_ is null");
        return;
    }
    
    int32_t ret = audioAdapter_->RegExtraParamObserver(audioAdapter_, adapterCallback, this);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::RegisterParameterCallback failed, error code: %d", ret);
    } else {
        paramCallbackRegistered_ = true;
    }
#endif
}

void RemoteAudioRendererSinkInner::SetAudioParameter(const AudioParamKey key, const std::string& condition,
    const std::string& value)
{
#ifdef FEATURE_DISTRIBUTE_AUDIO
    AUDIO_INFO_LOG("RemoteAudioRendererSink::SetParameter: key %{public}d, condition: %{public}s, value: %{public}s",
        key, condition.c_str(), value.c_str());
    enum AudioExtParamKey hdiKey = AudioExtParamKey(key);
    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetAudioParameter audioAdapter_ is null");
        return;
    }
    int32_t ret = audioAdapter_->SetExtraParams(audioAdapter_, hdiKey, condition.c_str(), value.c_str());
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetAudioParameter failed, error code: %d", ret);
    }
#endif
}

std::string RemoteAudioRendererSinkInner::GetAudioParameter(const AudioParamKey key, const std::string& condition)
{
#ifdef FEATURE_DISTRIBUTE_AUDIO
    AUDIO_INFO_LOG("RemoteAudioRendererSink::GetParameter: key %{public}d, condition: %{public}s", key,
        condition.c_str());
    enum AudioExtParamKey hdiKey = AudioExtParamKey(key);
    char value[PARAM_VALUE_LENTH];
    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::GetAudioParameter audioAdapter_ is null");
        return "";
    }
    int32_t ret = audioAdapter_->GetExtraParams(audioAdapter_, hdiKey, condition.c_str(), value, PARAM_VALUE_LENTH);
    if (ret !=SUCCESS) {
        AUDIO_ERR_LOG("AudioRendererSink::GetAudioParameter failed, error code: %d", ret);
        return "";
    }
    return value;
#else
    return "";
#endif
}

int32_t RemoteAudioRendererSinkInner::ParamEventCallback(AudioExtParamKey key, const char* condition, const char* value,
    void* reserved, void* cookie)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink::ParamEventCallback:key:%{public}d, condition:%{public}s, value:%{public}s",
        key, condition, value);
    RemoteAudioRendererSinkInner* sink = reinterpret_cast<RemoteAudioRendererSinkInner*>(cookie);
    std::string networkId = sink->GetNetworkId();
    AudioParamKey audioKey = AudioParamKey(key);
    // render state change to invalid.
    if (audioKey == AudioParamKey::RENDER_STATE) {
        char eventDes[EVENT_DES_SIZE];
        char contentDes[RENDER_STATE_CONTENT_DES_SIZE];
        if (sscanf_s(condition, "%[^;];%s", eventDes, EVENT_DES_SIZE, contentDes,
            RENDER_STATE_CONTENT_DES_SIZE) < PARAMS_RENDER_STATE_NUM) {
            AUDIO_ERR_LOG("[AudioPolicyServer]: Failed parse condition");
            return 0;
        }
        if (!strcmp(eventDes, "ERR_EVENT")) {
            AUDIO_INFO_LOG("RemoteAudioRendererSink render state invalid, destroy audioRender");
            if ((sink->audioRender_ != nullptr) && (sink->audioAdapter_ != nullptr)) {
                sink->audioAdapter_->DestroyRender(sink->audioAdapter_, sink->audioRender_);
            }
            sink->audioRender_ = nullptr;
            sink->isRenderCreated_.store(false);
        }
    }
    IAudioSinkCallback* callback = sink->GetParamCallback();
    callback->OnAudioParameterChange(networkId, audioKey, condition, value);
    return 0;
}

std::string RemoteAudioRendererSinkInner::GetNetworkId()
{
    return deviceNetworkId_;
}

OHOS::AudioStandard::IAudioSinkCallback* RemoteAudioRendererSinkInner::GetParamCallback()
{
    return callback_;
}

void RemoteAudioRendererSinkInner::DeInit()
{
    AUDIO_INFO_LOG("DeInit.");
    started_.store(false);
    rendererInited_.store(false);
    if ((audioRender_ != nullptr) && (audioAdapter_ != nullptr)) {
        audioAdapter_->DestroyRender(audioAdapter_, audioRender_);
    }
    audioRender_ = nullptr;

    if ((audioManager_ != nullptr) && (audioAdapter_ != nullptr)) {
        if (routeHandle_ != -1) {
            audioAdapter_->ReleaseAudioRoute(audioAdapter_, routeHandle_);
        }
        audioManager_->UnloadAdapter(audioManager_, audioAdapter_);
    }
    audioAdapter_ = nullptr;
    audioManager_ = nullptr;
#ifdef DEBUG_DUMP_FILE
    if (pfd) {
        fclose(pfd);
        pfd = nullptr;
    }
#endif // DEBUG_DUMP_FILE
    // remove map recorder.
    RemoteAudioRendererSinkInner *temp = allsinks[this->deviceNetworkId_];
    if (temp != nullptr) {
        delete temp;
        temp = nullptr;
        allsinks.erase(this->deviceNetworkId_);
    }
}

void InitAttrs(struct AudioSampleAttributes &attrs)
{
    /* Initialization of audio parameters for playback */
    attrs.channelCount = AUDIO_CHANNELCOUNT;
    attrs.sampleRate = AUDIO_SAMPLE_RATE_48K;
    attrs.interleaved = 0;
    attrs.streamId = INTERNAL_OUTPUT_STREAM_ID;
    attrs.type = AUDIO_IN_MEDIA;
    attrs.period = DEEP_BUFFER_RENDER_PERIOD_SIZE;
    attrs.isBigEndian = false;
    attrs.isSignedData = true;
    attrs.stopThreshold = INT_32_MAX;
    attrs.silenceThreshold = 0;
}

struct AudioManager *RemoteAudioRendererSinkInner::GetAudioManager()
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: Initialize audio proxy manager");
#ifdef FEATURE_DISTRIBUTE_AUDIO
#ifdef __aarch64__
    char resolvedPath[100] = "/system/lib64/libdaudio_client.z.so";
#else
    char resolvedPath[100] = "/system/lib/libdaudio_client.z.so";
#endif
    struct AudioManager *(*GetAudioManagerFuncs)() = nullptr;

    void *handle_ = dlopen(resolvedPath, 1);
    if (handle_ == nullptr) {
        AUDIO_ERR_LOG("Open so Fail");
        return nullptr;
    }
    AUDIO_INFO_LOG("dlopen successful");

    GetAudioManagerFuncs = (struct AudioManager *(*)())(dlsym(handle_, "GetAudioManagerFuncs"));
    if (GetAudioManagerFuncs == nullptr) {
        return nullptr;
    }
    AUDIO_INFO_LOG("GetAudioManagerFuncs done");

    struct AudioManager *audioManager = GetAudioManagerFuncs();
    if (audioManager == nullptr) {
        return nullptr;
    }
    AUDIO_INFO_LOG("daudio manager created");
#else
    struct AudioManager *audioManager = nullptr;
#endif // FEATURE_DISTRIBUTE_AUDIO
    return audioManager;
}

AudioFormat RemoteAudioRendererSinkInner::ConverToHdiFormat(AudioSampleFormat format)
{
    AudioFormat hdiFormat;
    switch (format) {
        case SAMPLE_U8:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_8_BIT;
            break;
        case SAMPLE_S16LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
        case SAMPLE_S24LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_24_BIT;
            break;
        case SAMPLE_S32LE:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_32_BIT;
            break;
        default:
            hdiFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
    }

    return hdiFormat;
}

int32_t RemoteAudioRendererSinkInner::CreateRender(const struct AudioPort &renderPort)
{
    int32_t ret;
    int64_t start = ClockTime::GetCurNano();
    struct AudioSampleAttributes param;
    InitAttrs(param);
    param.sampleRate = attr_.sampleRate;
    param.channelCount = attr_.channel;
    param.format = ConverToHdiFormat(attr_.format);
    param.frameSize = PCM_16_BIT * param.channelCount / PCM_8_BIT;
    param.startThreshold = DEEP_BUFFER_RENDER_PERIOD_SIZE / (param.frameSize);
    AUDIO_INFO_LOG("RemoteAudioRendererSink Create render format: %{public}d", param.format);
    struct AudioDeviceDescriptor deviceDesc;
    deviceDesc.portId = renderPort.portId;
    deviceDesc.pins = PIN_OUT_SPEAKER;
    deviceDesc.desc = nullptr;
    ret = audioAdapter_->CreateRender(audioAdapter_, &deviceDesc, &param, &audioRender_);
    if (ret != 0 || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("AudioDeviceCreateRender failed");
        return ERR_NOT_STARTED;
    }

    isRenderCreated_.store(true);
    int64_t cost = (ClockTime::GetCurNano() - start) / AUDIO_US_PER_SECOND;
    AUDIO_INFO_LOG("CreateRender cost[%{public}" PRId64 "]ms", cost);

    return 0;
}

inline std::string printRemoteAttr(IAudioSinkAttr attr_)
{
    std::stringstream value;
    value << "adapterName[" << attr_.adapterName << "] openMicSpeaker[" << attr_.openMicSpeaker << "] ";
    value << "format[" << static_cast<int32_t>(attr_.format) << "] sampleFmt[" << attr_.sampleFmt << "] ";
    value << "sampleRate[" << attr_.sampleRate << "] channel[" << attr_.channel << "] ";
    value << "volume[" << attr_.volume << "] filePath[" << attr_.filePath << "] ";
    value << "deviceNetworkId[" << attr_.deviceNetworkId << "] device_type[" << attr_.deviceType << "]";
    return value.str();
}

int32_t RemoteAudioRendererSinkInner::GetTargetAdapterPort(struct AudioAdapterDescriptor *descs, int32_t size,
    const char *networkId)
{
    int32_t targetIdx = -1;
    for (int32_t index = 0; index < size; index++) {
        struct AudioAdapterDescriptor *desc = &descs[index];
        if (desc == nullptr || desc->adapterName == nullptr) {
            continue;
        }
        if (strcmp(desc->adapterName, networkId)) {
            AUDIO_INFO_LOG("[%{public}d] is not target adapter", index);
            continue;
        }
        targetIdx = index;
        for (uint32_t port = 0; port < desc->portNum; port++) {
            // Only find out the port of out in the sound card
            if (desc->ports[port].portId == PIN_OUT_SPEAKER) {
                audioPort_ = desc->ports[port];
                break;
            }
        }
    }
    return targetIdx;
}

bool RemoteAudioRendererSinkInner::IsInited()
{
    return rendererInited_.load();
}

int32_t RemoteAudioRendererSinkInner::Init(IAudioSinkAttr attr)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink: Init start.");
    attr_ = attr;
    adapterNameCase_ = attr_.adapterName;  // Set sound card information
    openSpeaker_ = attr_.openMicSpeaker;

    if (audioManager_ == nullptr) {
        AUDIO_ERR_LOG("Init audio manager Fail");
        return ERR_NOT_STARTED;
    }

    int32_t size = 0;
    struct AudioAdapterDescriptor *descs = nullptr;
    int32_t ret = audioManager_->GetAllAdapters(audioManager_, &descs, &size);
    if (size == 0 || descs == nullptr || ret != 0) {
        AUDIO_ERR_LOG("Get adapters Fail");
        return ERR_NOT_STARTED;
    }
    AUDIO_INFO_LOG("Get [%{publid}d]adapters", size);
    int32_t targetIdx = GetTargetAdapterPort(descs, size, attr_.deviceNetworkId);
    CHECK_AND_RETURN_RET_LOG((targetIdx >= 0), ERR_NOT_STARTED, "can not find target adapter.");

    struct AudioAdapterDescriptor *desc = &descs[targetIdx];

    if (audioManager_->LoadAdapter(audioManager_, desc, &audioAdapter_) != 0) {
        AUDIO_ERR_LOG("Load Adapter Fail");
        return ERR_NOT_STARTED;
    }
    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("Load audio device failed");
        return ERR_NOT_STARTED;
    }

    ret = audioAdapter_->InitAllPorts(audioAdapter_);
    if (ret != 0) {
        AUDIO_ERR_LOG("InitAllPorts failed");
        return ERR_NOT_STARTED;
    }

    AUDIO_INFO_LOG("RemoteAudioRendererSink: Init end.");
    rendererInited_.store(true);

#ifdef DEBUG_DUMP_FILE
    AUDIO_INFO_LOG("dump IAudioSinkAttr:%{public}s", printRemoteAttr(attr_).c_str());
    std::string fileName = attr_.filePath;
    std::string filePath = "/data/local/tmp/remote_test_001.pcm";
    const char *g_audioOutTestFilePath = filePath.c_str();
    pfd = fopen(g_audioOutTestFilePath, "a+"); // here will not create a file if not exit.
    AUDIO_ERR_LOG("init dump file[%{public}s]", g_audioOutTestFilePath);
    if (pfd == nullptr) {
        AUDIO_ERR_LOG("Error opening remote pcm file[%{public}s]", g_audioOutTestFilePath);
    }
#endif // DEBUG_DUMP_FILE

    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::RenderFrame(char &data, uint64_t len, uint64_t &writeLen)
{
    int64_t start = ClockTime::GetCurNano();
    int32_t ret;
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("Audio Render Handle is nullptr!");
        return ERR_INVALID_HANDLE;
    }

    ret = audioRender_->RenderFrame(audioRender_, static_cast<void*>(&data), len, &writeLen);
    if (ret != 0) {
        AUDIO_ERR_LOG("RenderFrame failed ret: %{public}x", ret);
        return ERR_WRITE_FAILED;
    }
    writeLen = len;
#ifdef DEBUG_DUMP_FILE
    if (pfd != nullptr) {
        size_t writeResult = fwrite((void*)&data, 1, len, pfd);
        if (writeResult != len) {
            AUDIO_ERR_LOG("Failed to write the file.");
        }
    }
#endif // DEBUG_DUMP_FILE

    int64_t cost = (ClockTime::GetCurNano() - start) / AUDIO_US_PER_SECOND;
    AUDIO_DEBUG_LOG("RenderFrame len[%{public}" PRIu64 "] cost[%{public}" PRId64 "]ms", len, cost);
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Start(void)
{
    AUDIO_INFO_LOG("Start.");
    if (!isRenderCreated_.load()) {
        if (CreateRender(audioPort_) != 0) {
            AUDIO_ERR_LOG("Create render failed, Audio Port: %{public}d", audioPort_.portId);
            return ERR_NOT_STARTED;
        }
    }

    if (!started_.load()) {
        int32_t ret = audioRender_->control.Start(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Start failed!");
            return ERR_NOT_STARTED;
        }
        started_.store(true);
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::SetVolume(float left, float right)
{
    int32_t ret;
    float volume;

    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetVolume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    leftVolume_ = left;
    rightVolume_ = right;
    if ((leftVolume_ == 0) && (rightVolume_ != 0)) {
        volume = rightVolume_;
    } else if ((leftVolume_ != 0) && (rightVolume_ == 0)) {
        volume = leftVolume_;
    } else {
        volume = (leftVolume_ + rightVolume_) / HALF_FACTOR;
    }

    ret = audioRender_->volume.SetVolume(reinterpret_cast<AudioHandle>(audioRender_), volume);
    if (ret) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Set volume failed!");
    }

    return ret;
}

int32_t RemoteAudioRendererSinkInner::GetVolume(float &left, float &right)
{
    left = leftVolume_;
    right = rightVolume_;
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::GetLatency(uint32_t *latency)
{
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed audio render null");
        return ERR_INVALID_HANDLE;
    }

    if (!latency) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed latency null");
        return ERR_INVALID_PARAM;
    }

    uint32_t hdiLatency = 0;
    if (audioRender_->GetLatency(audioRender_, &hdiLatency) != 0) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: GetLatency failed.");
        return ERR_OPERATION_FAILED;
    }

    *latency = hdiLatency;
    return SUCCESS;
}

static AudioCategory GetAudioCategory(AudioScene audioScene)
{
    AudioCategory audioCategory;
    switch (audioScene) {
        case AUDIO_SCENE_DEFAULT:
            audioCategory = AUDIO_IN_MEDIA;
            break;
        case AUDIO_SCENE_RINGING:
            audioCategory = AUDIO_IN_RINGTONE;
            break;
        case AUDIO_SCENE_PHONE_CALL:
            audioCategory = AUDIO_IN_CALL;
            break;
        case AUDIO_SCENE_PHONE_CHAT:
            audioCategory = AUDIO_IN_COMMUNICATION;
            break;
        default:
            audioCategory = AUDIO_IN_MEDIA;
            break;
    }
    AUDIO_DEBUG_LOG("RemoteAudioRendererSink: Audio category returned is: %{public}d", audioCategory);

    return audioCategory;
}

static int32_t SetOutputPortPin(DeviceType outputDevice, AudioRouteNode &sink)
{
    int32_t ret = SUCCESS;

    switch (outputDevice) {
        case DEVICE_TYPE_SPEAKER:
            sink.ext.device.type = PIN_OUT_SPEAKER;
            sink.ext.device.desc = "pin_out_speaker";
            break;
        case DEVICE_TYPE_WIRED_HEADSET:
            sink.ext.device.type = PIN_OUT_HEADSET;
            sink.ext.device.desc = "pin_out_headset";
            break;
        case DEVICE_TYPE_USB_HEADSET:
            sink.ext.device.type = PIN_OUT_USB_EXT;
            sink.ext.device.desc = "pin_out_usb_ext";
            break;
        default:
            ret = ERR_NOT_SUPPORTED;
            break;
    }

    return ret;
}

int32_t RemoteAudioRendererSinkInner::OpenOutput(DeviceType outputDevice)
{
    AudioRouteNode source = {};
    AudioRouteNode sink = {};

    int32_t ret = SetOutputPortPin(outputDevice, sink);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: OpenOutput FAILED: %{public}d", ret);
        return ret;
    }

    source.portId = 0;
    source.role = AUDIO_PORT_SOURCE_ROLE;
    source.type = AUDIO_PORT_MIX_TYPE;
    source.ext.mix.moduleId = 0;
    source.ext.mix.streamId = INTERNAL_OUTPUT_STREAM_ID;

    sink.portId = audioPort_.portId;
    sink.role = AUDIO_PORT_SINK_ROLE;
    sink.type = AUDIO_PORT_DEVICE_TYPE;
    sink.ext.device.moduleId = 0;

    AudioRoute route = {
        .sourcesNum = 1,
        .sources = &source,
        .sinksNum = 1,
        .sinks = &sink,
    };

    if (audioAdapter_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: AudioAdapter object is null.");
        return ERR_OPERATION_FAILED;
    }

    ret = audioAdapter_->UpdateAudioRoute(audioAdapter_, &route, &routeHandle_);
    AUDIO_DEBUG_LOG("RemoteAudioRendererSink: UpdateAudioRoute returns: %{public}d", ret);
    if (ret != 0) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: UpdateAudioRoute failed");
        return ERR_OPERATION_FAILED;
    }

    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::GetTransactionId(uint64_t *transactionId)
{
    (void)transactionId;
    AUDIO_ERR_LOG("RemoteAudioRendererSink: GetTransactionId not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteAudioRendererSinkInner::SetVoiceVolume(float volume)
{
    (void)volume;
    AUDIO_ERR_LOG("RemoteAudioRendererSink: SetVoiceVolume not supported");
    return ERR_NOT_SUPPORTED;
}

int32_t RemoteAudioRendererSinkInner::SetOutputRoute(DeviceType deviceType)
{
    (void)deviceType;
    AUDIO_ERR_LOG("RemoteAudioRendererSink: SetOutputRoute not supported");
    return ERR_NOT_SUPPORTED;
}

void RemoteAudioRendererSinkInner::SetAudioMonoState(bool audioMono)
{
    (void)audioMono;
    AUDIO_ERR_LOG("RemoteAudioRendererSink: SetAudioMonoState not supported");
    return;
}

void RemoteAudioRendererSinkInner::SetAudioBalanceValue(float audioBalance)
{
    (void)audioBalance;
    AUDIO_ERR_LOG("RemoteAudioRendererSink: SetAudioBalanceValue not supported");
    return;
}

int32_t RemoteAudioRendererSinkInner::SetAudioScene(AudioScene audioScene, DeviceType activeDevice)
{
    AUDIO_INFO_LOG("RemoteAudioRendererSink::SetAudioScene scene: %{public}d, device: %{public}d",
        audioScene, activeDevice);
    CHECK_AND_RETURN_RET_LOG(audioScene >= AUDIO_SCENE_DEFAULT && audioScene <= AUDIO_SCENE_PHONE_CHAT,
        ERR_INVALID_PARAM, "invalid audioScene");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::SetAudioScene failed audio render handle is null!");
        return ERR_INVALID_HANDLE;
    }

    int32_t ret = OpenOutput(DEVICE_TYPE_SPEAKER);
    if (ret < 0) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: Update route FAILED: %{public}d", ret);
    }
    struct AudioSceneDescriptor scene;
    scene.scene.id = GetAudioCategory(audioScene);
    scene.desc.pins = PIN_OUT_SPEAKER;
    if (audioRender_->scene.SelectScene == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: Select scene nullptr");
        return ERR_OPERATION_FAILED;
    }

    AUDIO_INFO_LOG("RemoteAudioRendererSink::SelectScene start");
    ret = audioRender_->scene.SelectScene((AudioHandle)audioRender_, &scene);
    AUDIO_INFO_LOG("RemoteAudioRendererSink::SelectScene over");
    if (ret < 0) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink: Select scene FAILED: %{public}d", ret);
        return ERR_OPERATION_FAILED;
    }

    AUDIO_INFO_LOG("RemoteAudioRendererSink::Select audio scene SUCCESS: %{public}d", audioScene);
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Stop(void)
{
    AUDIO_INFO_LOG("Stop.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Stop failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (started_.load()) {
        int32_t ret = audioRender_->control.Stop(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Stop failed!");
            return ERR_OPERATION_FAILED;
        }
        started_.store(false);
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Pause(void)
{
    AUDIO_INFO_LOG("Pause.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_.load()) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (!paused_.load()) {
        int32_t ret = audioRender_->control.Pause(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Pause failed!");
            return ERR_OPERATION_FAILED;
        }
        paused_.store(true);
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Resume(void)
{
    AUDIO_INFO_LOG("Pause.");
    if (audioRender_ == nullptr) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume failed audioRender_ null");
        return ERR_INVALID_HANDLE;
    }

    if (!started_.load()) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume invalid state!");
        return ERR_OPERATION_FAILED;
    }

    if (paused_.load()) {
        int32_t ret = audioRender_->control.Resume(reinterpret_cast<AudioHandle>(audioRender_));
        if (ret) {
            AUDIO_ERR_LOG("RemoteAudioRendererSink::Resume failed!");
            return ERR_OPERATION_FAILED;
        }
        paused_.store(false);
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Reset(void)
{
    AUDIO_INFO_LOG("Reset.");
    if (!started_.load() || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("%{public}s remote renderer start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    if (ret) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Reset failed, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}

int32_t RemoteAudioRendererSinkInner::Flush(void)
{
    AUDIO_INFO_LOG("Flush.");
    if (!started_.load() || audioRender_ == nullptr) {
        AUDIO_ERR_LOG("%{public}s remote renderer start state %{public}d.", __func__, started_.load());
        return ERR_OPERATION_FAILED;
    }

    int32_t ret = audioRender_->control.Flush(reinterpret_cast<AudioHandle>(audioRender_));
    if (ret) {
        AUDIO_ERR_LOG("RemoteAudioRendererSink::Flush failed, ret %{public}d.", ret);
        return ERR_OPERATION_FAILED;
    }
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
