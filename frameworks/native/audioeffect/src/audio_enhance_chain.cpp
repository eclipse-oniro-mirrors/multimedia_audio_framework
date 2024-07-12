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
const uint32_t DEFAULT_DATAFORMAT = 16;
const uint32_t DEFAULT_REF_NUM = 0;
const uint32_t DEFAULT_VOIP_REF_NUM = 8;
const uint32_t DEFAULT_MIC_NUM = 4;
const uint32_t DEFAULT_OUT_NUM = 4;

AudioEnhanceChain::AudioEnhanceChain(const std::string &scene, const std::string &mode)
{
    sceneType_ = scene;
    enhanceMode_ = mode;
    
    InitAudioEnhanceChain();
    InitDump();
}

void AudioEnhanceChain::InitAudioEnhanceChain()
{
    setConfigFlag_ = false;
    needEcFlag_ = false;
    enhanceLibHandles_.clear();
    standByEnhanceHandles_.clear();

    algoSupportedConfig_ = {DEFAULT_FRAMELENGTH, DEFAULT_SAMPLE_RATE, DEFAULT_DATAFORMAT, DEFAULT_MIC_NUM,
        DEFAULT_REF_NUM, DEFAULT_OUT_NUM};
    
    if (count(NEED_EC_SCENE.begin(), NEED_EC_SCENE.end(), sceneType_)) {
        needEcFlag_ = true;
        algoSupportedConfig_.refNum = DEFAULT_VOIP_REF_NUM;
    }

    uint32_t batchLen = algoSupportedConfig_.refNum + algoSupportedConfig_.micNum;
    uint32_t bitDepth = algoSupportedConfig_.dataFormat / BITLENGTH;
    uint32_t byteLenPerFrame = algoSupportedConfig_.frameLength * (algoSupportedConfig_.sampleRate / MILLISECOND)
        * bitDepth;
    algoAttr_ = {bitDepth, batchLen, byteLenPerFrame};
    algoCache_.cache.resize(algoAttr_.batchLen);
    for (auto &it : algoCache_.cache) {
        it.resize(algoAttr_.byteLenPerFrame);
    }
    algoCache_.input.resize(algoAttr_.byteLenPerFrame * algoAttr_.batchLen);
    algoCache_.output.resize(algoAttr_.byteLenPerFrame * algoSupportedConfig_.outNum);
}

void AudioEnhanceChain::InitDump()
{
    std::string dumpFileName = "Enhance_";
    std::string dumpFileInName = dumpFileName + sceneType_ + "_" + GetTime() + "_In.pcm";
    std::string dumpFileOutName = dumpFileName + sceneType_ + "_" + GetTime() + "_Out.pcm";
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileInName, &dumpFileIn_);
    DumpFileUtil::OpenDumpFile(DUMP_SERVER_PARA, dumpFileOutName, &dumpFileOut_);
}

AudioEnhanceChain::~AudioEnhanceChain()
{
    ReleaseEnhanceChain();
    DumpFileUtil::CloseDumpFile(&dumpFileIn_);
    DumpFileUtil::CloseDumpFile(&dumpFileOut_);
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

void AudioEnhanceChain::AddEnhanceHandle(AudioEffectHandle handle, AudioEffectLibrary *libHandle)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    int32_t ret = 0;
    AudioEffectTransInfo cmdInfo = {};
    AudioEffectTransInfo replyInfo = {};

    cmdInfo.data = static_cast<void *>(&algoSupportedConfig_);
    cmdInfo.size = sizeof(algoSupportedConfig_);

    ret = (*handle)->command(handle, EFFECT_CMD_INIT, &cmdInfo, &replyInfo);
    CHECK_AND_RETURN_LOG(ret == 0, "[%{public}s] with [%{public}s], either one of libs EFFECT_CMD_INIT fail",
        sceneType_.c_str(), enhanceMode_.c_str());

    setConfigFlag_ = true;
    standByEnhanceHandles_.emplace_back(handle);
    enhanceLibHandles_.emplace_back(libHandle);
}

bool AudioEnhanceChain::IsEmptyEnhanceHandles()
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    return standByEnhanceHandles_.size() == 0;
}

void AudioEnhanceChain::GetAlgoConfig(AudioBufferConfig &algoConfig)
{
    algoConfig.samplingRate = algoSupportedConfig_.sampleRate;
    algoConfig.channels = algoSupportedConfig_.micNum;
    algoConfig.format = static_cast<uint8_t>(algoSupportedConfig_.dataFormat);
    return;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSize()
{
    return algoAttr_.byteLenPerFrame * algoSupportedConfig_.micNum;
}

uint32_t AudioEnhanceChain::GetAlgoBufferSizeEc()
{
    return algoAttr_.byteLenPerFrame * algoSupportedConfig_.refNum;
}

int32_t AudioEnhanceChain::GetOneFrameInputData(std::unique_ptr<EnhanceBuffer> &enhanceBuffer)
{
    CHECK_AND_RETURN_RET_LOG(enhanceBuffer != nullptr, ERROR, "enhance buffer is null");

    int32_t ret = 0;
    uint32_t index = 0;
    for (uint32_t j = 0; j < algoSupportedConfig_.refNum; ++j) {
        ret = memset_s(algoCache_.cache[j].data(), algoCache_.cache[j].size(), 0, algoCache_.cache[j].size());
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memset error in ref channel memcpy");
    }

    // decross for Mic
    index = 0;
    for (uint32_t i = 0; i < algoAttr_.byteLenPerFrame / algoAttr_.bitDepth; ++i) {
        // mic channel
        for (uint32_t j = algoSupportedConfig_.refNum; j < algoAttr_.batchLen; ++j) {
            ret = memcpy_s(&algoCache_.cache[j][i * algoAttr_.bitDepth], algoAttr_.bitDepth,
                enhanceBuffer->micBufferIn.data() + index, algoAttr_.bitDepth);
            CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy error in mic channel memcpy");
            index += algoAttr_.bitDepth;
        }
    }
    for (uint32_t i = 0; i < algoAttr_.batchLen; ++i) {
        ret = memcpy_s(&algoCache_.input[i * algoAttr_.byteLenPerFrame], algoAttr_.byteLenPerFrame,
            algoCache_.cache[i].data(), algoAttr_.byteLenPerFrame);
        CHECK_AND_RETURN_RET_LOG(ret == 0, ERROR, "memcpy error in cache to input");
    }
    return SUCCESS;
}

int32_t AudioEnhanceChain::ApplyEnhanceChain(std::unique_ptr<EnhanceBuffer> &enhanceBuffer, uint32_t length)
{
    std::lock_guard<std::mutex> lock(chainMutex_);
    CHECK_AND_RETURN_RET_LOG(enhanceBuffer != nullptr, ERROR, "enhance buffer is null");
    DumpFileUtil::WriteDumpFile(dumpFileIn_, enhanceBuffer->micBufferIn.data(), (uint64_t)length);

    uint32_t inputLen = algoAttr_.byteLenPerFrame * algoAttr_.batchLen;
    uint32_t outputLen = algoAttr_.byteLenPerFrame * algoSupportedConfig_.outNum;
    AUDIO_DEBUG_LOG("inputLen = %{public}u outputLen = %{public}u", inputLen, outputLen);

    if (standByEnhanceHandles_.size() == 0) {
        AUDIO_DEBUG_LOG("audioEnhanceChain->standByEnhanceHandles is empty");
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), length,
            enhanceBuffer->micBufferIn.data(), length) == 0, ERROR, "memcpy error in IsEmptyEnhanceHandles");
        return ERROR;
    }
    if (GetOneFrameInputData(enhanceBuffer) != SUCCESS) {
        AUDIO_ERR_LOG("GetOneFrameInputData failed");
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), length,
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
        CHECK_AND_CONTINUE_LOG(ret == 0, "[%{publc}s] with mode [%{public}s], either one of libs process fail",
            sceneType_.c_str(), enhanceMode_.c_str());
    }
    CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer->micBufferOut.data(), outputLen, audioBufOut_.raw, outputLen) == 0,
        ERROR, "memcpy error in audioBufOut_ to enhanceBuffer->output");
    DumpFileUtil::WriteDumpFile(dumpFileOut_, enhanceBuffer->micBufferOut.data(), (uint64_t)length);
    return SUCCESS;
}

} // namespace AudioStandard
} // namespace OHOS