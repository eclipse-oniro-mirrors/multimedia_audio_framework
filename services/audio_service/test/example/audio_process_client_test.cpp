/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cinttypes>
#include <condition_variable>
#include <cstdint>
#include <ctime>
#include <ostream>
#include <sstream>
#include <iostream>
#include <thread>
#include <mutex>
#include <map>
#include <securec.h>

#include "audio_log.h"
#include "audio_errors.h"
#include "audio_utils.h"
#include "audio_process_in_client.h"
#include "parameter.h"
#include "pcm2wav.h"

using namespace std;
namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr long WAV_HEADER_SIZE = 42;
    static constexpr int64_t SECOND_TO_NANOSECOND = 1000000000;
    constexpr int32_t SAMPLE_FORMAT_U8 = 8;
    constexpr int32_t SAMPLE_FORMAT_S16LE = 16;
    constexpr int32_t SAMPLE_FORMAT_S24LE = 24;
    constexpr int32_t SAMPLE_FORMAT_S32LE = 32;
    enum OperationCode : int32_t {
        INVALID_OPERATION = -1,
        INIT_LOCAL_SPK_PROCESS = 0,
        INIT_REMOTE_SPK_PROCESS = 1,
        START_SPK_PROCESS = 2,
        PAUSE_SPK_PROCESS = 3,
        RESUME_SPK_PROCESS = 4,
        STOP_SPK_PROCESS = 5,
        CHANGE_SPK_PROCESS_VOL = 6,
        RELEASE_SPK_PROCESS = 7,

        START_LOOP_TEST = 10,
        END_LOOP_TEST = 11,

        INIT_LOCAL_MIC_PROCESS = 20,
        INIT_REMOTE_MIC_PROCESS = 21,
        START_MIC_PROCESS = 22,
        PAUSE_MIC_PROCESS = 23,
        RESUME_MIC_PROCESS = 24,
        STOP_MIC_PROCESS = 25,
        CHANGE_MIC_PROCESS_VOL = 26,
        RELEASE_MIC_PROCESS = 27,

        EXIT_INTERACTIVE_TEST = 40,
    };

    enum AudioProcessTestType : int32_t {
        INVALID_PROC_TEST = 0,
        INTERACTIVE_RUN_SPK_TEST = 1,
        AUTO_RUN_SPK_TEST = 2,
        INTERACTIVE_RUN_MIC_TEST = 3,
        AUTO_RUN_MIC_TEST = 4,
        INTERACTIVE_RUN_LOOP = 5,
        EXIT_PROC_TEST = 6,
    };
    static constexpr size_t CACHE_BUFFER_SIZE = 960;
    bool g_loopTest = false;
    int64_t g_stampTime = 0;
}

class AudioProcessTest;
shared_ptr<AudioProcessTest> g_audioProcessTest = nullptr;
std::string g_spkfilePath = "";
const std::string MIC_FILE_PATH = "/data/data/mic.pcm";
FILE *g_spkWavFile = nullptr;
FILE *g_micPcmFile = nullptr;
mutex g_autoRunMutex;
condition_variable g_autoRunCV;

unique_ptr<uint8_t[]> g_byteBuffer = nullptr;
BufferDesc g_cacheBuffer = {nullptr, 0, 0};

string ConfigSpkTest(bool isRemote);
string CallStartSpk();
string CallPauseSpk();
string CallResumeSpk();
string CallStopSpk();
string SetSpkVolume();
string CallReleaseSpk();

string ConfigMicTest(bool isRemote);
string CallStartMic();
string CallPauseMic();
string CallResumeMic();
string CallStopMic();
string SetMicVolume();
string CallReleaseMic();
using CallTestOperationFunc = string (*)();

std::map<int32_t, std::string> g_audioProcessTestType = {
    {INTERACTIVE_RUN_SPK_TEST, "Interactive run spk process test"},
    {AUTO_RUN_SPK_TEST, "Auto run spk process test"},
    {INTERACTIVE_RUN_MIC_TEST, "Interactive run mic process test"},
    {AUTO_RUN_MIC_TEST, "Auto run mic process test"},
    {INTERACTIVE_RUN_LOOP, "Roundtrip latency test"},
    {EXIT_PROC_TEST, "Exit audio process test"},
};

std::map<int32_t, std::string> g_interactiveOptStrMap = {
    {INIT_LOCAL_SPK_PROCESS, "call local spk init process"},
    {INIT_REMOTE_SPK_PROCESS, "call remote spk init process"},
    {START_SPK_PROCESS, "call start spk process"},
    {PAUSE_SPK_PROCESS, "call pause spk process"},
    {RESUME_SPK_PROCESS, "call resume spk process"},
    {STOP_SPK_PROCESS, "call stop spk process"},
    {CHANGE_SPK_PROCESS_VOL, "change spk process volume"},
    {RELEASE_SPK_PROCESS, "release spk process"},

    {START_LOOP_TEST, "start loop"},
    {END_LOOP_TEST, "end loop"},

    {INIT_LOCAL_MIC_PROCESS, "call local mic init process"},
    {INIT_REMOTE_MIC_PROCESS, "call remote mic init process"},
    {START_MIC_PROCESS, "call start mic process"},
    {PAUSE_MIC_PROCESS, "call pause mic process"},
    {RESUME_MIC_PROCESS, "call resume mic process"},
    {STOP_MIC_PROCESS, "call stop mic process"},
    {CHANGE_MIC_PROCESS_VOL, "change mic process volume"},
    {RELEASE_MIC_PROCESS, "release mic process"},

    {EXIT_INTERACTIVE_TEST, "exit interactive run test"},
};

std::map<int32_t, CallTestOperationFunc> g_interactiveOptFuncMap = {
    {START_SPK_PROCESS, CallStartSpk},
    {PAUSE_SPK_PROCESS, CallPauseSpk},
    {RESUME_SPK_PROCESS, CallResumeSpk},
    {STOP_SPK_PROCESS, CallStopSpk},
    {CHANGE_SPK_PROCESS_VOL, SetSpkVolume},
    {RELEASE_SPK_PROCESS, CallReleaseSpk},

    {START_MIC_PROCESS, CallStartMic},
    {PAUSE_MIC_PROCESS, CallPauseMic},
    {RESUME_MIC_PROCESS, CallResumeMic},
    {STOP_MIC_PROCESS, CallStopMic},
    {CHANGE_MIC_PROCESS_VOL, SetMicVolume},
    {RELEASE_MIC_PROCESS, CallReleaseMic}
};

class AudioProcessTestCallback : public AudioDataCallback {
public:
    AudioProcessTestCallback(const std::shared_ptr<AudioProcessInClient> &procClient,
        int32_t spkLoopCnt, AudioMode clientMode)
        : procClient_(procClient), loopCount_(spkLoopCnt), clientMode_(clientMode) {};
    ~AudioProcessTestCallback() = default;

    void OnHandleData(size_t length) override;
    void InitSignalBuffer(const BufferDesc &signalSoundBuffer)
    {
        int ret = memset_s(signalSoundBuffer.buffer, signalSoundBuffer.bufLength, 0, signalSoundBuffer.bufLength);
        if (ret != EOK) {
            return;
        }
        const int channels = 2; // 2 channels
        const int samplePerChannel = 96 / channels; // 96 for 1ms
        int16_t *signalData = static_cast<int16_t *>(static_cast<void *>(signalSoundBuffer.buffer));
        int16_t bound = 10;
        for (int idx = 0; idx < samplePerChannel; idx++) {
            signalData[channels * idx] = bound + static_cast<int16_t>(sinf(2.0f * static_cast<float>(M_PI) * idx /
                samplePerChannel) * (SHRT_MAX - bound));
            for (int c = 1; c < channels; c++) {
                signalData[channels * idx + c] = signalData[channels * idx];
            }
        }
    };

private:
    int32_t CaptureToFile(const BufferDesc &bufDesc);
    int32_t RenderFromFile(const BufferDesc &bufDesc);

    void HandleWriteData(const BufferDesc &bufDesc)
    {
        loopCount_++;
        int32_t periodCount = loopCount_ % 400; // 400 * 0.005 = 2s

        if (periodCount == 0) {
            InitSignalBuffer(bufDesc); // set signal data
            int64_t temp = ClockTime::GetCurNano() - g_stampTime;
            std::cout << "client read-write latency:" << (temp / AUDIO_MS_PER_SECOND) << " us" << std::endl;
            return;
        }

        int32_t keepQuiteHold = 50;
        if (periodCount > keepQuiteHold) {
            return;
        }

        // copy mic data in the cache buffer
        int ret = memcpy_s(static_cast<void *>(bufDesc.buffer), bufDesc.bufLength,
            static_cast<void *>(g_cacheBuffer.buffer), g_cacheBuffer.bufLength);
        if (ret != EOK) {
            AUDIO_WARNING_LOG("memcpy_s failed.");
        }
    };

private:
    std::shared_ptr<AudioProcessInClient> procClient_ = nullptr;
    int32_t loopCount_ = -1; // for loop
    AudioMode clientMode_ = AUDIO_MODE_PLAYBACK;
    bool renderFinish_ = false;
};

class AudioProcessTest {
public:
    AudioProcessTest() = default;
    ~AudioProcessTest() = default;

    int32_t InitSpk(int32_t loopCount, bool isRemote);
    bool StartSpk();
    bool PauseSpk();
    bool ResumeSpk();
    bool SetSpkVolume(int32_t vol);
    bool StopSpk();
    bool ReleaseSpk();

    int32_t InitMic(bool isRemote);
    bool StartMic();
    bool PauseMic();
    bool ResumeMic();
    bool SetMicVolume(int32_t vol);
    bool StopMic();
    bool ReleaseMic();

private:
    std::shared_ptr<AudioProcessInClient> spkProcessClient_ = nullptr;
    std::shared_ptr<AudioProcessInClient> micProcessClient_ = nullptr;
    std::shared_ptr<AudioProcessTestCallback> spkProcClientCb_ = nullptr;
    std::shared_ptr<AudioProcessTestCallback> micProcClientCb_ = nullptr;
    int32_t loopCount_ = -1; // for loop
};

int32_t AudioProcessTestCallback::CaptureToFile(const BufferDesc &bufDesc)
{
    CHECK_AND_RETURN_RET_LOG(g_micPcmFile != nullptr, ERR_INVALID_HANDLE,
        "%{public}s g_micPcmFile is null.", __func__);

    size_t cnt = fwrite(bufDesc.buffer, 1, bufDesc.bufLength, g_micPcmFile);
    CHECK_AND_RETURN_RET_LOG(cnt == bufDesc.bufLength, ERR_WRITE_FAILED,
        "%{public}s fwrite fail, cnt %{public}zu, bufLength %{public}zu.", __func__, cnt, bufDesc.bufLength);
    int ret = memcpy_s(static_cast<void *>(g_cacheBuffer.buffer), bufDesc.bufLength,
        static_cast<void *>(bufDesc.buffer), bufDesc.bufLength);
    if (ret != EOK) {
        AUDIO_WARNING_LOG("memcpy_s failed.");
    }
    g_stampTime = ClockTime::GetCurNano();
    return SUCCESS;
}

int32_t AudioProcessTestCallback::RenderFromFile(const BufferDesc &bufDesc)
{
    CHECK_AND_RETURN_RET_LOG(g_spkWavFile != nullptr, ERR_INVALID_HANDLE,
        "%{public}s g_spkWavFile is null.", __func__);

    if (feof(g_spkWavFile)) {
        if (loopCount_ < 0) {
            fseek(g_spkWavFile, WAV_HEADER_SIZE, SEEK_SET); // infinite loop
        } else if (loopCount_ == 0) {
            renderFinish_ = true;
            g_autoRunCV.notify_all();
        } else {
            loopCount_--;
            fseek(g_spkWavFile, WAV_HEADER_SIZE, SEEK_SET);
        }
    }
    if (renderFinish_) {
        AUDIO_INFO_LOG("%{public}s render finish.", __func__);
        return SUCCESS;
    }
    fread(bufDesc.buffer, 1, bufDesc.bufLength, g_spkWavFile);
    return SUCCESS;
}

void AudioProcessTestCallback::OnHandleData(size_t length)
{
    Trace callBack("client_n");
    CHECK_AND_RETURN_LOG(procClient_ != nullptr, "%{public}s procClient is null.", __func__);

    BufferDesc bufDesc = {nullptr, 0, 0};
    int32_t ret = procClient_->GetBufferDesc(bufDesc);
    if (ret != SUCCESS || bufDesc.buffer == nullptr || bufDesc.bufLength ==0) {
        cout << "GetBufferDesc failed." << endl;
        return;
    }

    if (clientMode_ == AUDIO_MODE_RECORD) {
        ret = CaptureToFile(bufDesc);
        CHECK_AND_RETURN_LOG(ret == SUCCESS, "%{public}s capture to file fail, ret %{public}d.",
            __func__, ret);
    } else {
        if (!g_loopTest) {
            ret = RenderFromFile(bufDesc);
            CHECK_AND_RETURN_LOG(ret == SUCCESS, "%{public}s render from file fail, ret %{public}d.", __func__, ret);
        } else {
            HandleWriteData(bufDesc);
        }
    }
    ret = procClient_->Enqueue(bufDesc);
    CHECK_AND_RETURN_LOG(ret == SUCCESS, "%{public}s enqueue buf fail, clientMode %{public}d, ret %{public}d.",
        __func__, clientMode_, ret);

    callBack.End();
}

inline AudioSampleFormat GetSampleFormat(int32_t wavSampleFormat)
{
    switch (wavSampleFormat) {
        case SAMPLE_FORMAT_U8:
            return AudioSampleFormat::SAMPLE_U8;
        case SAMPLE_FORMAT_S16LE:
            return AudioSampleFormat::SAMPLE_S16LE;
        case SAMPLE_FORMAT_S24LE:
            return AudioSampleFormat::SAMPLE_S24LE;
        case SAMPLE_FORMAT_S32LE:
            return AudioSampleFormat::SAMPLE_S32LE;
        default:
            return AudioSampleFormat::INVALID_WIDTH;
    }
}

int32_t AudioProcessTest::InitSpk(int32_t loopCount, bool isRemote)
{
    if (loopCount < 0) {
        loopCount_ = 1; // loop once
    } else if (loopCount == 0) {
        loopCount_ = -1; // infinite loop
    } else {
        loopCount_ = loopCount;
    }

    AudioProcessConfig config;
    config.appInfo.appPid = getpid();
    config.appInfo.appUid = getuid();

    config.audioMode = AUDIO_MODE_PLAYBACK;

    config.rendererInfo.contentType = CONTENT_TYPE_MUSIC;
    config.rendererInfo.streamUsage = STREAM_USAGE_MEDIA;
    config.rendererInfo.rendererFlags = 4; // 4 for test

    config.streamInfo.channels = STEREO;
    config.streamInfo.encoding = ENCODING_PCM;
    config.streamInfo.format = SAMPLE_S16LE;
    config.streamInfo.samplingRate = SAMPLE_RATE_48000;

    config.isRemote = isRemote;

    if (!g_loopTest) {
        wav_hdr wavHeader;
        size_t headerSize = sizeof(wav_hdr);
        size_t bytesRead = fread(&wavHeader, 1, headerSize, g_spkWavFile);
        if (bytesRead != headerSize) {
            AUDIO_ERR_LOG("RenderCallbackTest: File header reading error");
        }

        config.streamInfo.samplingRate = static_cast<AudioSamplingRate>(wavHeader.SamplesPerSec);
        config.streamInfo.format = GetSampleFormat(wavHeader.bitsPerSample);
        config.streamInfo.channels = static_cast<AudioChannel>(wavHeader.NumOfChan);

        cout << endl << "samplingRate:" << config.streamInfo.samplingRate << endl;
        cout << "format:" << config.streamInfo.format << endl;
        cout << "channels:" << config.streamInfo.channels << endl;
    }

    spkProcessClient_ = AudioProcessInClient::Create(config);
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, ERR_INVALID_HANDLE,
        "Client test creat process client fail.");

    spkProcClientCb_ = std::make_shared<AudioProcessTestCallback>(spkProcessClient_, loopCount_, config.audioMode);
    int32_t ret = spkProcessClient_->SaveDataCallback(spkProcClientCb_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    return SUCCESS;
}

bool AudioProcessTest::StartSpk()
{
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::PauseSpk()
{
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::ResumeSpk()
{
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Resume();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::SetSpkVolume(int32_t vol)
{
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->SetVolume(vol);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::StopSpk()
{
    CHECK_AND_RETURN_RET_LOG(spkProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = spkProcessClient_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::ReleaseSpk()
{
    if (spkProcessClient_ == nullptr) {
        AUDIO_INFO_LOG("%{public}s process client is already released.", __func__);
        return true;
    }
    int32_t ret = spkProcessClient_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test release fail, ret %{public}d.", ret);
    spkProcessClient_ = nullptr;
    AUDIO_INFO_LOG("client test set nullptr!");
    return true;
}

int32_t AudioProcessTest::InitMic(bool isRemote)
{
    AudioProcessConfig config;
    config.appInfo.appPid = getpid();
    config.appInfo.appUid = getuid();

    config.audioMode = AUDIO_MODE_RECORD;
    config.capturerInfo.sourceType = SOURCE_TYPE_MIC;
    config.capturerInfo.capturerFlags = 7; // 7 for test

    config.streamInfo.channels = STEREO;
    config.streamInfo.encoding = ENCODING_PCM;
    config.streamInfo.format = SAMPLE_S16LE;
    config.streamInfo.samplingRate = SAMPLE_RATE_48000;

    config.isRemote = isRemote;

    micProcessClient_ = AudioProcessInClient::Create(config);
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, ERR_INVALID_HANDLE,
        "Client test creat process client fail.");

    micProcClientCb_ = std::make_shared<AudioProcessTestCallback>(micProcessClient_, 0, config.audioMode);
    int32_t ret = micProcessClient_->SaveDataCallback(micProcClientCb_);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Client test save data callback fail, ret %{public}d.", ret);
    return SUCCESS;
}

bool AudioProcessTest::StartMic()
{
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = micProcessClient_->Start();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::PauseMic()
{
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = micProcessClient_->Pause();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::ResumeMic()
{
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = micProcessClient_->Resume();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::SetMicVolume(int32_t vol)
{
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = micProcessClient_->SetVolume(vol);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::StopMic()
{
    CHECK_AND_RETURN_RET_LOG(micProcessClient_ != nullptr, false, "%{public}s process client is null.", __func__);
    int32_t ret = micProcessClient_->Stop();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test stop fail, ret %{public}d.", ret);
    return true;
}

bool AudioProcessTest::ReleaseMic()
{
    if (micProcessClient_ == nullptr) {
        AUDIO_INFO_LOG("%{public}s process client is already released.", __func__);
        return true;
    }
    int32_t ret = micProcessClient_->Release();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, false, "Client test release fail, ret %{public}d.", ret);
    micProcessClient_ = nullptr;
    AUDIO_INFO_LOG("client test set nullptr!");
    return true;
}

bool OpenSpkFile()
{
    if (g_spkWavFile != nullptr) {
        AUDIO_ERR_LOG("Spk file has been opened, g_spkfilePath %{public}s", g_spkfilePath.c_str());
        return true;
    }

    char path[PATH_MAX] = { 0x00 };
    if ((strlen(g_spkfilePath.c_str()) > PATH_MAX) || (realpath(g_spkfilePath.c_str(), path) == nullptr)) {
        return false;
    }
    AUDIO_INFO_LOG("spk path = %{public}s", path);
    g_spkWavFile = fopen(path, "rb");
    if (g_spkWavFile == nullptr) {
        AUDIO_ERR_LOG("Unable to open wave file");
        return false;
    }
    return true;
}

void CloseSpkFile()
{
    if (g_spkWavFile != nullptr) {
        fclose(g_spkWavFile);
        g_spkWavFile = nullptr;
    }
}

bool OpenMicFile()
{
    if (g_micPcmFile != nullptr) {
        AUDIO_ERR_LOG("Mic file has been opened, MIC_FILE_PATH %{public}s", MIC_FILE_PATH.c_str());
        return true;
    }

    AUDIO_INFO_LOG("mic path = %{public}s", MIC_FILE_PATH.c_str());
    g_micPcmFile = fopen(MIC_FILE_PATH.c_str(), "ab+");
    if (g_micPcmFile == nullptr) {
        AUDIO_ERR_LOG("Unable to open wave file");
        return false;
    }
    return true;
}

void CloseMicFile()
{
    if (g_micPcmFile != nullptr) {
        fclose(g_micPcmFile);
        g_micPcmFile = nullptr;
    }
}

inline int32_t GetArgs(const std::string &args)
{
    int32_t value = 0;
    stringstream valueStr;
    valueStr << args;
    valueStr >> value;
    return value;
}

void PrintInteractiveUsage()
{
    cout << endl << "======================= InteractiveRunTestSelect ============================" << endl;
    cout << "You can respond to instructions for corresponding option:" << endl;
    for (auto it = g_interactiveOptStrMap.begin(); it != g_interactiveOptStrMap.end(); it ++) {
        cout << "\t enter " << it->first << " : " << it->second << endl;
    }
}

void PrintProcTestUsage()
{
    cout << endl << "========================== ProcessTestSelect ================================" << endl;
    cout << "You can respond to instructions for corresponding test:" << endl;
    for (auto it = g_audioProcessTestType.begin(); it != g_audioProcessTestType.end(); it ++) {
        cout << it->first << ". " << it->second << endl;
    }
}

void PrintUsage()
{
    cout << "[Audio Process Client Test App]" << endl << endl;
    cout << "Supported Functionalities:" << endl;
    cout << "  a) Auto run local spk test." << endl;
    cout << "  b) Interactive run local/remote spk test." << endl;
    cout << "  c) Auto run remote mic test." << endl;
    cout << "  d) Interactive run remote mic test." << endl;
    cout << "================================Usage=======================================" << endl << endl;

    cout << "-a\n\tAuto run local spk process test, pelese input the following after select." << endl;
    cout << "\tUsage : <wav-file-path> <play-loop-count>" << endl;
    cout << "\t       if <play-loop-count> equals to 0, it will loop infinitely." << endl;
    cout << "\tExample 1 : /data/data/48kHz_16bit.wav 0" << endl;
    cout << "\tExample 2 : /data/data/48kHz_16bit.wav 2" << endl << endl;

    cout << "-b\n\tInteractive run local/remote spk test, pelese input the following after select." << endl;
    cout << "\tUsage : <wav-file-path>" << endl;

    cout << "-c\n\tAuto run remote mic process test, pelese input the following after select." << endl;
    cout << "\tUsage : <record-time-in-seconds>" << endl;
    cout << "\tGenerate the specified time span record file, path : /data/data/mic.pcm" << endl;

    cout << "-d\n\tInteractive run remote mic test." << endl;
    cout << "\tGenerate record file from start to stop, path : /data/data/mic.pcm" << endl;
}

int32_t GetUserInput()
{
    int32_t res = -1; // result
    size_t count = 3; // try three time
    cout << ">>";
    cin >> res;
    while (cin.fail() && count-- > 0) {
        cin.clear();
        cin.ignore();
        cout << "invalid input, not a number! Please retry with a number." << endl;
        cout << ">>";
        cin >> res;
    }
    return res;
}

void AutoRunSpk()
{
    cout << "Auto run spk process test enter, please input loopCount and path:" << endl;
    int32_t loopCount = GetUserInput();
    std::string palyFilePath;
    cin >> palyFilePath;
    g_spkfilePath = palyFilePath;

    if (!OpenSpkFile()) {
        cout << "open spk file path failed!" << g_spkfilePath << endl;
        return;
    }
    if (g_audioProcessTest->InitSpk(loopCount, false) != SUCCESS) {
        cout << "Spk init failed!" << endl;
        return;
    }

    do {
        if (!g_audioProcessTest->StartSpk()) {
            cout << "Spk start failed!" << endl;
            break;
        }
        int volShift = 15; // helf of 1 << 16
        if (!g_audioProcessTest->SetSpkVolume(1 << volShift)) {
            cout << "Spk set volume " << volShift << " failed!" << endl;
            break;
        }

        unique_lock<mutex> lock(g_autoRunMutex);
        g_autoRunCV.wait(lock);
        cout << "AutoRunSpk end" << endl;

        if (!g_audioProcessTest->StopSpk()) {
            cout << "Spk stop failed!" << endl;
            break;
        }
    } while (false);

    if (!g_audioProcessTest->ReleaseSpk()) {
        cout << "Spk release failed!" << endl;
    }
    CloseSpkFile();
}

void AutoRunMic()
{
    cout << "Auto run mic process test enter, please input recordTimeS:" << endl;
    int32_t recordTimeS = GetUserInput();
    if (!OpenMicFile()) {
        cout << "open mic file path failed!" << g_spkfilePath << endl;
        return;
    }

    if (g_audioProcessTest->InitMic(false) != SUCCESS) {
        cout << "Mic init failed!" << endl;
        return;
    }

    do {
        if (!g_audioProcessTest->StartMic()) {
            cout << "Mic start failed!" << endl;
            break;
        }
        int volShift = 15; // helf of 1 << 16
        if (!g_audioProcessTest->SetMicVolume(1 << volShift)) {
            cout << "Mic set volume " << volShift << " failed!" << endl;
            break;
        }

        cout << "wait " << recordTimeS << "s for capture frame..." << endl;
        ClockTime::RelativeSleep(recordTimeS * SECOND_TO_NANOSECOND);
        cout << "AutoRunMic end" << endl;

        if (!g_audioProcessTest->StopMic()) {
            cout << "Mic stop failed!" << endl;
            break;
        }
    } while (false);

    if (!g_audioProcessTest->ReleaseMic()) {
        cout << "Mic release failed!" << endl;
    }
    CloseMicFile();
}

string ConfigSpkTest(bool isRemote)
{
    cout << "Please input spk file path:" << endl;
    std::string palyFilePath;
    cin >> palyFilePath;
    g_spkfilePath = palyFilePath;

    if (!OpenSpkFile()) {
        cout << "Open spk file path failed!" << g_spkfilePath << endl;
        return "Open spk wav file fail";
    }
    int32_t ret = g_audioProcessTest->InitSpk(0, isRemote);
    if (ret != SUCCESS) {
        return "Spk init failed";
    }
    return "Spk init SUCCESS";
}

string CallStartSpk()
{
    if (!g_audioProcessTest->StartSpk()) {
        return "Spk start failed";
    }
    return "Spk start SUCCESS";
}

string CallPauseSpk()
{
    if (!g_audioProcessTest->PauseSpk()) {
        return "Spk pause failed";
    }
    return "Spk pause SUCCESS";
}

string CallResumeSpk()
{
    if (!g_audioProcessTest->ResumeSpk()) {
        return "Spk resume failed";
    }
    return "Spk resume SUCCESS";
}

string CallStopSpk()
{
    if (!g_audioProcessTest->StopSpk()) {
        return "Spk stop failed";
    }
    return "Spk stop SUCCESS";
}

string SetSpkVolume()
{
    int32_t vol = GetUserInput();
    if (!g_audioProcessTest->SetSpkVolume(vol)) {
        return "Spk set volume failed";
    }
    return "Spk set volume SUCCESS";
}

string CallReleaseSpk()
{
    if (!g_audioProcessTest->ReleaseSpk()) {
        return "Spk release failed";
    }
    CloseSpkFile();
    return "Spk release SUCCESS";
}

string ConfigMicTest(bool isRemote)
{
    if (!OpenMicFile()) {
        cout << "Open mic file path failed!" << g_spkfilePath << endl;
        return "Open mic pcm file fail";
    }

    int32_t ret = g_audioProcessTest->InitMic(isRemote);
    if (ret != SUCCESS) {
        return "Mic init failed";
    }
    return "Mic init SUCCESS";
}

string CallStartMic()
{
    if (!g_audioProcessTest->StartMic()) {
        return "Mic start failed";
    }
    return "Mic start SUCCESS";
}

string CallPauseMic()
{
    if (!g_audioProcessTest->PauseMic()) {
        return "Mic pause failed";
    }
    return "Mic pause SUCCESS";
}

string CallResumeMic()
{
    if (!g_audioProcessTest->ResumeMic()) {
        return "Mic resume failed";
    }
    return "Mic resume SUCCESS";
}

string CallStopMic()
{
    if (!g_audioProcessTest->StopMic()) {
        return "Mic stop failed";
    }
    return "Mic stop SUCCESS";
}

string SetMicVolume()
{
    int32_t vol = GetUserInput();
    if (!g_audioProcessTest->SetMicVolume(vol)) {
        return "Mic set volume failed";
    }
    return "Mic set volume SUCCESS";
}

string CallReleaseMic()
{
    if (!g_audioProcessTest->ReleaseMic()) {
        return "Mic release failed";
    }
    CloseMicFile();
    return "Mic release SUCCESS";
}

string StartLoopTest()
{
    std::cout << ConfigMicTest(false);
    std::cout << CallStartMic();
    std::cout << endl;

    int32_t ret = g_audioProcessTest->InitSpk(0, false);
    if (ret != SUCCESS) {
        CallReleaseMic();
        return "init spk failed";
    } else {
        std::cout << "init spk success" << endl;
    }

    std::cout << CallStartSpk();
    std::cout << endl;
    return "StartLoopTest success!";
}

string EndLoopTest()
{
    std::cout << CallReleaseSpk();
    std::cout << endl;
    std::cout << CallStopMic();
    std::cout << endl;
    std::cout << CallReleaseMic();
    std::cout << endl;
    return "EndLooptest";
}

void InitCachebuffer()
{
    g_byteBuffer = std::make_unique<uint8_t []>(CACHE_BUFFER_SIZE);
    g_cacheBuffer.buffer = g_byteBuffer.get();
    g_cacheBuffer.bufLength = CACHE_BUFFER_SIZE;
    g_cacheBuffer.dataLength = CACHE_BUFFER_SIZE;
}

void InteractiveRun()
{
    if (g_loopTest) {
        InitCachebuffer();
    }
    cout << "Interactive run process test enter." << endl;
    bool isInteractiveRun = true;
    while (isInteractiveRun) {
        PrintInteractiveUsage();
        OperationCode optCode = INVALID_OPERATION;
        int32_t res = GetUserInput();
        if (g_interactiveOptStrMap.count(res)) {
            optCode = static_cast<OperationCode>(res);
        }
        switch (optCode) {
            case EXIT_INTERACTIVE_TEST:
                isInteractiveRun = false;
                break;
            case INIT_LOCAL_SPK_PROCESS:
                cout << ConfigSpkTest(false) << endl;
                break;
            case INIT_REMOTE_SPK_PROCESS:
                cout << ConfigSpkTest(true) << endl;
                break;
            case START_LOOP_TEST:
                cout << StartLoopTest() << endl;
                break;
            case END_LOOP_TEST:
                cout << EndLoopTest() << endl;
                break;
            case INIT_LOCAL_MIC_PROCESS:
                cout << ConfigMicTest(false) << endl;
                break;
            case INIT_REMOTE_MIC_PROCESS:
                cout << ConfigMicTest(true) << endl;
                break;
            default:
                auto it = g_interactiveOptFuncMap.find(optCode);
                if (it != g_interactiveOptFuncMap.end() && it->second != nullptr) {
                    CallTestOperationFunc &func = it->second;
                    cout << (*func)() << endl;
                    break;
                }
                cout << "Invalid input :" << optCode << endl;
                break;
        }
    }
    cout << "Interactive run process test end." << endl;
}

bool SetSysPara(const std::string key, int32_t &value)
{
    auto res = SetParameter(key.c_str(), std::to_string(value).c_str());
    if (res < 0) {
        AUDIO_WARNING_LOG("SetSysPara fail, key:%{public}s res:%{public}d", key.c_str(), res);
        return false;
    }
    AUDIO_INFO_LOG("SetSysPara success.");
    return true;
}
} // namespace AudioStandard
} // namespace OHOS

using namespace OHOS::AudioStandard;
int main()
{
    AUDIO_INFO_LOG("AudioProcessClientTest test enter.");
    int32_t val = 1;
    SetSysPara("persist.multimedia.audio.mmap.enable", val);

    PrintUsage();
    g_audioProcessTest = make_shared<AudioProcessTest>();

    bool isProcTestRun = true;
    while (isProcTestRun) {
        PrintProcTestUsage();
        AudioProcessTestType procTestType = INVALID_PROC_TEST;
        int32_t res = GetUserInput();
        if (g_audioProcessTestType.count(res)) {
            procTestType = static_cast<AudioProcessTestType>(res);
        }
        switch (procTestType) {
            case INTERACTIVE_RUN_SPK_TEST:
            case INTERACTIVE_RUN_MIC_TEST:
                InteractiveRun();
                break;
            case INTERACTIVE_RUN_LOOP:
                g_loopTest = true;
                InteractiveRun();
                break;
            case AUTO_RUN_SPK_TEST:
                AutoRunSpk();
                break;
            case AUTO_RUN_MIC_TEST:
                AutoRunMic();
                break;
            case EXIT_PROC_TEST:
                isProcTestRun = false;
                break;
            default:
                cout << "invalid input, procTestType: " << procTestType << endl;
                break;
        }
    }
    AUDIO_INFO_LOG("AudioProcessClientTest test end.");
    return 0;
}
