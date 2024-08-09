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

#undef LOG_TAG
#define LOG_TAG "AudioEnhanceChain"

#include "audio_enhance_chain.h"

#include <chrono>

#include "securec.h"
#include "audio_log.h"
#include "audio_errors.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {

const uint32_t BITLENGTH = 8;
const uint32_t MILLISECOND = 1000;
const uint32_t DEFAULT_FRAMELENGTH = 20;
const uint32_t DEFAULT_SAMPLE_RATE = 48000;

AudioEnhanceChain::AudioEnhanceChain(const std::string &scene, const AudioEnhanceParamAdapter &algoParam,
    const AudioEnhanceDeviceAttr &deviceAttr, const bool defaultFlag)
{
    sceneType_ = scene;
    algoParam_ = algoParam;
    defaultFlag_ = defaultFlag;
    deviceAttr_ = deviceAttr;
    
    InitAudioEnhanceChain();
    if (deviceAttr_.needEc) {
        UpdateAlgoConfig();
    }
    InitDump();
}

void AudioEnhanceChain::InitAudioEnhanceChain()
{
    setConfigFlag_ = false;
    needEcFlag_ = false;
    needMicRefFlag_ = false;
    enhanceLibHandles_.clear();
    standByEnhanceHandles_.clear();

    algoSupportedConfig_ = {DEFAULT_FRAMELENGTH, deviceAttr_.micRate, deviceAttr_.micFormat * BITLENGTH,
        deviceAttr_.micChannels, deviceAttr_.ecChannels, deviceAttr_.micRefChannels, deviceAttr_.micChannels};
    
    uint32_t byteLenPerFrame = DEFAULT_FRAMELENGTH * (deviceAttr_.micRate / MILLISECOND) * deviceAttr_.micFormat;
    algoAttr_ = {deviceAttr_.micFormat, deviceAttr_.micChannels, byteLenPerFrame};

    algoCache_.input.resize(algoAttr_.byteLenPerFrame * algoAttr_.batchLen);
    algoCache_.output.resize(algoAttr_.byteLenPerFrame * deviceAttr_.micChannels);
    AUDIO_INFO_LOG("micNum: %{public}u byteLenPerFrame: %{public}u inputsize:%{public}zu outputsize:%{public}zu",
        algoSupportedConfig_.micNum, byteLenPerFrame, algoCache_.input.size(), algoCache_.output.size());
    
    if (count(NEED_EC_SCENE.begin(), NEED_EC_SCENE.end(), sceneType_)) {
        needEcFlag_ = true;
    }

    if (count(NEED_MICREF_SCENE.begin(), NEED_MICREF_SCENE.end(), sceneType_)) {
        needMicRefFlag_ = true;
    }
}

void AudioEnhanceChain::InitDump()
{
    std::string dumpFileName = "Enhance_";
    std::string dumpFileInName = dumpFileName + sceneType_ + "_" + GetTime() + "_In.pcm";
    std::string dumpFileOutName = dumpFileName + sceneType_ + "_" + GetTime() + "_Out.pcm";
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileInName, &dumpFileIn_);
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileOutName, &dumpFileOut_);

    if (deviceAttr_.needEc) {
        std::string dumpFileEcName = dumpFileName + sceneType_ + "_" + GetTime() + "_EC.pcm";
        DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileEcName, &dumpFileEc_);
    }
    if (deviceAttr_.needMicRef) {
        std::string dumpFileMicRefName = dumpFileName + sceneType_ + "_" + GetTime() + "_MicRef.pcm";
        DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileMicRefName, &dumpFileMicRef_);
    }
}

AudioEnhanceChain::~AudioEnhanceChain()
{
    ReleaseEnhanceChain();
    DumpFileUtil::CloseDumpFile(&dumpFileIn_);
    DumpFileUtil::CloseDumpFile(&dumpFileOut_);
    DumpFileUtil::CloseDumpFile(&dumpFileEc_);
    DumpFileUtil::CloseDumpFile(&dumpFileMicRef_);
}

void AudioEnhanceChain::ReleaseEnhanceChain()
{
    for (uint32_t i = 0; i < standByEnhanceHandles_.size() && i < enhanceLibHandles_.size(); i++) {
        if (!enhanceLibHandles_[i]) {
            continue;
        }
        if (!standByEnhanceHandles_[i]) {
            continue;
        }
        if (!enhanceLibHandles_[i]->releaseEffect) {
            continue;
        }
        enhanceLibHandles_[i]->releaseEffect(standByEnhanceHandles_[i]);
    }
    standByEnhanceHandles_.clear();
    enhanceLibHandles_.clear();
}

int32_t AudioEnhanceChain::SetInputDevice(const std::string &inputDevice)
{
    if (inputDevice.size() == 0) {
        return SUCCESS;
    }
    algoParam_.preDevice = inputDevice;
    AUDIO_INFO_LOG("update input device %{public}s", inputDevice.c_str());
    std::lock_guard<std::mutex> lock(chainMutex_);
    int32_t size = standByEnhanceHandles_.size();
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};
    for (int32_t index = 0; index < size; index++) {
        auto &handle = standByEnhanceHandles_[index];
        CHECK_AND_RETURN_RET_LOG(SetEnhanceParamToHandle(handle) == SUCCESS, ERROR,
            "[%{public}s] effect EFFECT_CMD_SET_PARAM fail", sceneType_.c_str());
        CHECK_AND_RETURN_RET_LOG((*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo) == 0, ERROR,
            "[%{public}s] effect EFFECT_CMD_INIT fail", sceneType_.c_str());
    }
    return SUCCESS;
}

int32_t AudioEnhanceChain::SetEnhanceParamToHandle(AudioEffectHandle handle)
{
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    AudioEnhanceParam setParam = {algoParam_.muteInfo, algoParam_.volumeInfo, algoParam_.preDevice.c_str(),
        algoParam_.postDevice.c_str(), algoParam_.sceneType.c_str()};

    cmdInfo.data = static_cast<void *>(&setParam);
    cmdInfo.size = sizeof(setParam);

    int32_t ret = (*handle)->command(handle, EFFECT_CMD_SET_PARAM, &cmdInfo, &replyInfo);
    if (ret != 0) {
        return ERROR;
    }
    AUDIO_DEBUG_LOG("%{public}s: EFFECT_CMD_SET_PARAM success", sceneType_.c_str());
    return SUCCESS;
}

void AudioEnhanceChain::AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle,
    const std::string &enhance, const std::string &property)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    int32_t ret = 0;
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    uint32_t maxSampleRate = DEFAULT_SAMPLE_RATE;
    replyInfo.data = &maxSampleRate;
    replyInfo.size = sizeof(maxSampleRate);
    ret = (*handle)->command(handle, EFFECT_CMD_GET_CONFIG, &cmdInfo, &replyInfo);
    if (ret) {
        AUDIO_ERR_LOG("get algo maxSampleRate failed!");
    }
    algoSupportedConfig_.sampleRate = maxSampleRate;

    cmdInfo.data = static_cast<void *>(&algoSupportedConfig_);
    cmdInfo.size = sizeof(algoSupportedConfig_);
    
    ret = (*handle)->command(handle, EFFECT_CMD_SET_CONFIG, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s], either one of libs EFFECT_CMD_SET_CONFIG fail", sceneType_.c_str());

    CHECK_AND_RETURN_LOG(SetEnhanceParamToHandle(handle) == 0, "[%{public}s] %{public}s lib EFFECT_CMD_SET_PARAM fail",
        sceneType_.c_str(), libHandle->name);

    if (SetPropertyToHandle(handle, property) != SUCCESS) {
        AUDIO_INFO_LOG("[%{public}s] %{public}s effect EFFECT_CMD_SET_PROPERTY fail",
            sceneType_.c_str(), enhance.c_str());
    }

    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s], either one of libs EFFECT_CMD_INIT fail", sceneType_.c_str());
    
    setConfigFlag_ = true;
    enhanceNames_.emplace_back(enhance);
    standByEnhanceHandles_.emplace_back(handle);
    enhanceLibHandles_.emplace_back(libHandle);
}

bool AudioEnhanceChain::IsEmptyEnhanceHandles()
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    return standByEnhanceHandles_.size() == 0;
}

int32_t AudioEnhanceChain::UpdateAlgoConfig()
{
    algoSupportedConfig_.dataFormat = deviceAttr_.micFormat * BITLENGTH;
    algoSupportedConfig_.micNum = deviceAttr_.micChannels;
    algoSupportedConfig_.ecNum = deviceAttr_.ecChannels;
    algoSupportedConfig_.micRefNum = deviceAttr_.micRefChannels;
    algoSupportedConfig_.outNum = deviceAttr_.micChannels;

    uint32_t batchLen = deviceAttr_.micChannels + deviceAttr_.ecChannels + deviceAttr_.micRefChannels;
    uint32_t bitDepth = deviceAttr_.micFormat;
    uint32_t byteLenPerFrame = algoSupportedConfig_.frameLength * (algoSupportedConfig_.sampleRate / MILLISECOND)
        * bitDepth;
    algoAttr_ = {bitDepth, batchLen, byteLenPerFrame};

    algoCache_.input.resize(algoAttr_.byteLenPerFrame * algoAttr_.batchLen);
    algoCache_.output.resize(algoAttr_.byteLenPerFrame * algoSupportedConfig_.outNum);
    AUDIO_INFO_LOG("micNum: %{public}u ecNum: %{public}u micRefNum: %{public}u outNum: %{public}u \
        byteLenPerFrame: %{public}u inputsize:%{public}zu outputsize:%{public}zu",
        algoSupportedConfig_.micNum, algoSupportedConfig_.ecNum, algoSupportedConfig_.micRefNum,
        algoSupportedConfig_.outNum, byteLenPerFrame, algoCache_.input.size(), algoCache_.output.size());
    return SUCCESS;
}

void AudioEnhanceChain::GetAlgoConfig(AudioBufferConfig &algoConfig)
{
    algoConfig.samplingRate = algoSupportedConfig_.sampleRate;
    algoConfig.channels = algoSupportedConfig_.micNum;
    algoConfig.format = static_cast<uint8_t>(algoSupportedConfig_.dataFormat);
    return;
}

void AudioEnhanceChain::GetEcAndMicRefFlag(bool &needEcFlag, bool &needMicRefFlag)
{
    needEcFlag = needEcFlag_;
    needMicRefFlag = needMicRefFlag_;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSize()
{
    uint32_t byteLenPerFrame = DEFAULT_FRAMELENGTH * (algoSupportedConfig_.sampleRate / MILLISECOND) *
        deviceAttr_.micFormat;
    return byteLenPerFrame * deviceAttr_.micChannels;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSizeEc()
{
    uint32_t byteLenPerFrame = DEFAULT_FRAMELENGTH * (algoSupportedConfig_.sampleRate / MILLISECOND) *
        deviceAttr_.ecFormat;
    return byteLenPerFrame * deviceAttr_.ecChannels;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSizeMicRef()
{
    uint32_t byteLenPerFrame = DEFAULT_FRAMELENGTH * (algoSupportedConfig_.sampleRate / MILLISECOND) *
        deviceAttr_.micRefFormat;
    return byteLenPerFrame * deviceAttr_.micRefChannels;
}

int32_t AudioEnhanceChain::DeinterleaverData(uint8_t *src, uint32_t channel, uint8_t *dst, uint32_t offset)
{
    CHECK_AND_RETURN_RET_LOG(src != nullptr, ERROR, "src is nullptr");
    CHECK_AND_RETURN_RET_LOG(dst != nullptr, ERROR, "dst is nullptr");
    int32_t ret = 0;
    uint32_t idx = 0;
    for (uint32_t i = 0; i < algoAttr_.byteLenPerFrame / algoAttr_.bitDepth; ++i) {
        for (uint32_t j = 0; j < channel; ++j) {
            ret = memcpy_s(dst + j * algoAttr_.byteLenPerFrame + i * algoAttr_.bitDepth,
                algoCache_.input.size() - (j * algoAttr_.byteLenPerFrame + i * algoAttr_.bitDepth + offset),
                src + idx, algoAttr_.bitDepth);
            CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy in deinterleaver error");
            idx += algoAttr_.bitDepth;
        }
    }
    return SUCCESS;
}

int32_t AudioEnhanceChain::GetOneFrameInputData(std::unique_ptr<EnhanceBuffer> &enhanceBuffer)
{
    CHECK_AND_RETURN_RET_LOG(enhanceBuffer != nullptr, ERROR, "enhance buffer is null");

    uint32_t offset = 0;
    int32_t ret = 0;
    if (enhanceBuffer->ecBuffer.size() != 0) {
        ret = DeinterleaverData(enhanceBuffer->ecBuffer.data(), deviceAttr_.ecChannels,
            &algoCache_.input[offset], offset);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy error in ec channel memcpy");
        offset += algoAttr_.byteLenPerFrame * deviceAttr_.ecChannels;
    }

    if (enhanceBuffer->micBufferIn.size() != 0) {
        ret = DeinterleaverData(enhanceBuffer->micBufferIn.data(), deviceAttr_.micChannels,
            &algoCache_.input[offset], offset);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy error in mic channel memcpy");
        offset += algoAttr_.byteLenPerFrame * deviceAttr_.micChannels;
    }

    if (enhanceBuffer->micRefBuffer.size() != 0) {
        ret = DeinterleaverData(enhanceBuffer->micRefBuffer.data(), deviceAttr_.micRefChannels,
            &algoCache_.input[offset], offset);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy error in mic ref channel memcpy");
    }
    return SUCCESS;
}

int32_t AudioEnhanceChain::ApplyEnhanceChain(std::unique_ptr<EnhanceBuffer> &enhanceBuffer, uint32_t length)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    CHECK_AND_RETURN_RET_LOG(enhanceBuffer != nullptr, ERROR, "enhance buffer is null");
    DumpFileUtil::WriteDumpFile(dumpFileIn_, enhanceBuffer->micBufferIn.data(), (uint64_t)length);
    if (deviceAttr_.needEc) {
        DumpFileUtil::WriteDumpFile(dumpFileEc_, enhanceBuffer->ecBuffer.data(),
            (uint64_t)enhanceBuffer->ecBuffer.size());
    }
    if (deviceAttr_.needMicRef) {
        DumpFileUtil::WriteDumpFile(dumpFileMicRef_, enhanceBuffer->micRefBuffer.data(),
            (uint64_t)enhanceBuffer->micRefBuffer.size());
    }

    uint32_t inputLen = algoAttr_.byteLenPerFrame * algoAttr_.batchLen;
    uint32_t outputLen = algoAttr_.byteLenPerFrame * algoSupportedConfig_.outNum;
    AUDIO_DEBUG_LOG("inputLen = %{public}u outputLen = %{public}u", inputLen, outputLen);

    if (standByEnhanceHandles_.size() == 0) {
        AUDIO_DEBUG_LOG("audioEnhanceChain->standByEnhanceHandles is empty");
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), enhanceBuffer->micBufferOut.size(),
            enhanceBuffer->micBufferIn.data(), length) == 0, ERROR, "memcpy error in IsEmptyEnhanceHandles");
        return ERROR;
    }
    if (GetOneFrameInputData(enhanceBuffer) != SUCCESS) {
        AUDIO_ERR_LOG("GetOneFrameInputData failed");
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), enhanceBuffer->micBufferOut.size(),
            enhanceBuffer->micBufferIn.data(), length) == 0, ERROR, "memcpy error in GetOneFrameInputData");
        return ERROR;
    }
    AudioBuffer audioBufIn_ = {};
    AudioBuffer audioBufOut_ = {};
    audioBufIn_.frameLength = inputLen;
    audioBufOut_.frameLength = outputLen;
    audioBufIn_.raw = static_cast<void *>(algoCache_.input.data());
    audioBufOut_.raw = static_cast<void *>(algoCache_.output.data());

    for (AudioEffectHandle handle : standByEnhanceHandles_) {
        int32_t ret = (*handle)->process(handle, &audioBufIn_, &audioBufOut_);
        CHECK_AND_CONTINUE_LOG(ret == 0, "[%{publc}s] either one of libs process fail", sceneType_.c_str());
    }
    CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), enhanceBuffer->micBufferOut.size(),
        audioBufOut_.raw, outputLen) == 0,
        ERROR, "memcpy error in audioBufOut_ to enhanceBuffer->output");
    DumpFileUtil::WriteDumpFile(dumpFileOut_, enhanceBuffer->micBufferOut.data(), (uint64_t)length);
    return SUCCESS;
}

int32_t AudioEnhanceChain::SetEnhanceProperty(const std::string &enhance, const std::string &property)
{
    if (property.empty()) { return SUCCESS; }
    std::lock_guard<std::mutex> lock(chainMutex_);
    int32_t size = standByEnhanceHandles_.size();
    AudioEffectTransInfo cmdInfo{};
    AudioEffectTransInfo replyInfo{};
    for (int32_t index = 0; index < size; index++) {
        auto &handle = standByEnhanceHandles_[index];
        auto const &enhanceName = enhanceNames_[index];
        if (enhance == enhanceName) {
            CHECK_AND_RETURN_RET_LOG(SetPropertyToHandle(handle, property) == SUCCESS, ERROR,
                "[%{public}s] %{public}s effect EFFECT_CMD_SET_PROPERTY fail",
                sceneType_.c_str(), enhance.c_str());
            CHECK_AND_RETURN_RET_LOG(
                (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo) == SUCCESS, ERROR,
                "[%{public}s] %{public}s effect EFFECT_CMD_INIT fail",
                sceneType_.c_str(), enhance.c_str());
        }
    }
    return SUCCESS;
}

int32_t AudioEnhanceChain::SetPropertyToHandle(AudioEffectHandle handle, const std::string &property)
{
    if (property.empty()) { return SUCCESS; }
    int32_t replyData = 0;
    const char *propCstr = property.c_str();
    AudioEffectTransInfo cmdInfo = {sizeof(const char *), reinterpret_cast<void*>(&propCstr)};
    AudioEffectTransInfo replyInfo = {sizeof(int32_t), &replyData};
    return (*handle)->command(handle, EFFECT_CMD_SET_PROPERTY, &cmdInfo, &replyInfo);
}

bool AudioEnhanceChain::IsDefaultChain()
{
    return defaultFlag_;
}
} // namespace AudioStandard
} // namespace OHOS