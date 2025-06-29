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

#ifndef LOG_TAG
#define LOG_TAG "HpaeCaptureEffectNode"
#endif

#include "hpae_capture_effect_node.h"
#include <iostream>
#include "hpae_pcm_buffer.h"
#include "audio_engine_log.h"
#include "audio_errors.h"
#include "hpae_format_convert.h"
#include "audio_enhance_chain_manager.h"
#include "audio_effect_map.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
HpaeCaptureEffectNode::HpaeCaptureEffectNode(HpaeNodeInfo &nodeInfo)
    : HpaeNode(nodeInfo), HpaePluginNode(nodeInfo)
{
    const std::unordered_map<AudioEnhanceScene, std::string> &audioEnhanceSupportedSceneTypes =
        GetEnhanceSupportedSceneType();
    auto item = audioEnhanceSupportedSceneTypes.find(nodeInfo.effectInfo.enhanceScene);
    if (item != audioEnhanceSupportedSceneTypes.end()) {
        sceneType_ = item->second;
        AUDIO_INFO_LOG("HpaeCaptureEffectNode created scenetype: [%{public}s]", sceneType_.c_str());
    } else {
        AUDIO_ERR_LOG("scenetype: %{public}u not supported", nodeInfo.effectInfo.enhanceScene);
    }
#ifdef ENABLE_HOOK_PCM
    outputPcmDumper_ = std::make_unique<HpaePcmDumper>(
        "HpaeCaptureEffectNode_id_" + std::to_string(GetNodeId()) + "_" + sceneType_ + "_Out.pcm");
#endif
}

bool HpaeCaptureEffectNode::Reset()
{
    return HpaePluginNode::Reset();
}

HpaePcmBuffer *HpaeCaptureEffectNode::SignalProcess(const std::vector<HpaePcmBuffer *> &inputs)
{
    Trace trace("[" + sceneType_ + "]HpaeRenderEffectNode::SignalProcess inputs num[" +
        std::to_string(inputs.size()) + "]");
    if (inputs.empty()) {
        AUDIO_WARNING_LOG("HpaeCaptureEffectNode inputs size is empty, SessionId:%{public}d", GetSessionId());
        return nullptr;
    }

    AudioEnhanceChainManager *audioEnhanceChainManager = AudioEnhanceChainManager::GetInstance();
    uint32_t processLength = 0;
    uint32_t outputIndex = 0;
    for (uint32_t i = 0; i < inputs.size(); i++) {
        if (inputs[i]->GetSourceBufferType() == HPAE_SOURCE_BUFFER_TYPE_MIC) {
            ConvertFromFloat(SAMPLE_S16LE, micBufferLength_ / GetSizeFromFormat(SAMPLE_S16LE),
                inputs[i]->GetPcmDataBuffer(), static_cast<void *>(cacheDataIn_.data()));
            audioEnhanceChainManager->CopyToEnhanceBuffer(static_cast<void *>(cacheDataIn_.data()), micBufferLength_);
            processLength = micBufferLength_;
            outputIndex = i;
            AUDIO_DEBUG_LOG("CopyToEnhanceBuffer size:%{public}u", processLength);
        } else if (inputs[i]->GetSourceBufferType() == HPAE_SOURCE_BUFFER_TYPE_EC) {
            ConvertFromFloat(SAMPLE_S16LE, ecBufferLength_ / GetSizeFromFormat(SAMPLE_S16LE),
                inputs[i]->GetPcmDataBuffer(), static_cast<void *>(cacheDataIn_.data()));
            audioEnhanceChainManager->CopyEcToEnhanceBuffer(static_cast<void *>(cacheDataIn_.data()), ecBufferLength_);
            AUDIO_DEBUG_LOG("CopyEcToEnhanceBuffer size:%{public}u", ecBufferLength_);
        } else if (inputs[i]->GetSourceBufferType() == HPAE_SOURCE_BUFFER_TYPE_MICREF) {
            ConvertFromFloat(SAMPLE_S16LE, micrefBufferLength_ / GetSizeFromFormat(SAMPLE_S16LE),
                inputs[i]->GetPcmDataBuffer(), static_cast<void *>(cacheDataIn_.data()));
            audioEnhanceChainManager->CopyMicRefToEnhanceBuffer(static_cast<void *>(cacheDataIn_.data()),
                micrefBufferLength_);
            AUDIO_DEBUG_LOG("CopyMicRefToEnhanceBuffer size:%{public}u", micrefBufferLength_);
        }
        cacheDataIn_.clear();
    }
    int32_t ret = audioEnhanceChainManager->ApplyAudioEnhanceChain(sceneKeyCode_, processLength);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, inputs[outputIndex], "effect apply failed, ret:%{public}d", ret);
    audioEnhanceChainManager->CopyFromEnhanceBuffer(static_cast<void *>(cacheDataOut_.data()), processLength);
    ConvertToFloat(SAMPLE_S16LE, micBufferLength_ / GetSizeFromFormat(SAMPLE_S16LE),
        static_cast<void *>(cacheDataOut_.data()), inputs[outputIndex]->GetPcmDataBuffer());
#ifdef ENABLE_HOOK_PCM
    if (outputPcmDumper_) {
        outputPcmDumper_->Dump((int8_t *)inputs[outputIndex]->GetPcmDataBuffer(),
            micBufferLength_ / GetSizeFromFormat(SAMPLE_S16LE) * sizeof(float));
    }
#endif
    return inputs[outputIndex];
}

void HpaeCaptureEffectNode::ConnectWithInfo(const std::shared_ptr<OutputNode<HpaePcmBuffer*>> &preNode,
    HpaeNodeInfo &nodeInfo)
{
    std::shared_ptr<HpaeNode> realPreNode = preNode->GetSharedInstance(nodeInfo);
    CHECK_AND_RETURN_LOG(realPreNode != nullptr, "realPreNode is nullptr");
    inputStream_.Connect(realPreNode, preNode->GetOutputPort(nodeInfo));
#ifdef ENABLE_HIDUMP_DFX
    if (auto callback = GetNodeStatusCallback().lock()) {
        callback->OnNotifyDfxNodeInfo(
            true, realPreNode->GetNodeId(), GetNodeInfo());
    }
#endif
}

void HpaeCaptureEffectNode::DisConnectWithInfo(const std::shared_ptr<OutputNode<HpaePcmBuffer*>>& preNode,
    HpaeNodeInfo &nodeInfo)
{
    CHECK_AND_RETURN_LOG(!inputStream_.CheckIfDisConnected(preNode->GetOutputPort(nodeInfo)),
        "HpaeCaptureEffectNode[%{public}u] has disconnected with preNode", GetNodeId());
    inputStream_.DisConnect(preNode->GetOutputPort(nodeInfo, true));
#ifdef ENABLE_HIDUMP_DFX
    if (auto callback = GetNodeStatusCallback().lock()) {
        callback->OnNotifyDfxNodeInfo(false, GetNodeId(), GetNodeInfo());
    }
#endif
}

bool HpaeCaptureEffectNode::GetCapturerEffectConfig(HpaeNodeInfo& nodeInfo, HpaeSourceBufferType type)
{
    CHECK_AND_RETURN_RET_LOG(capturerEffectConfigMap_.find(type) != capturerEffectConfigMap_.end(),
        false, "not need resample node, type:%{public}u", type);
    nodeInfo = capturerEffectConfigMap_[type];
    return true;
}

void HpaeCaptureEffectNode::SetCapturerEffectConfig(AudioBufferConfig micConfig, AudioBufferConfig ecConfig,
    AudioBufferConfig micrefConfig)
{
    HpaeNodeInfo micInfo = {};
    HpaeNodeInfo ecInfo = {};
    HpaeNodeInfo micrefInfo = {};
    micInfo.sourceBufferType = HPAE_SOURCE_BUFFER_TYPE_MIC;
    micInfo.frameLen = FRAME_LEN * (micConfig.samplingRate / MILLISECOND_PER_SECOND);
    micInfo.samplingRate = static_cast<AudioSamplingRate>(micConfig.samplingRate);
    micInfo.channels = static_cast<AudioChannel>(micConfig.channels);
    micInfo.format = static_cast<AudioSampleFormat>(micConfig.format / BITLENGTH - 1);
    ecInfo.sourceBufferType = HPAE_SOURCE_BUFFER_TYPE_EC;
    ecInfo.frameLen = FRAME_LEN * (ecConfig.samplingRate / MILLISECOND_PER_SECOND);
    ecInfo.samplingRate = static_cast<AudioSamplingRate>(ecConfig.samplingRate);
    ecInfo.channels = static_cast<AudioChannel>(ecConfig.channels);
    ecInfo.format = static_cast<AudioSampleFormat>(ecConfig.format / BITLENGTH - 1);
    micrefInfo.sourceBufferType = HPAE_SOURCE_BUFFER_TYPE_MICREF;
    micrefInfo.frameLen = FRAME_LEN * (micrefConfig.samplingRate / MILLISECOND_PER_SECOND);
    micrefInfo.samplingRate = static_cast<AudioSamplingRate>(micrefConfig.samplingRate);
    micrefInfo.channels = static_cast<AudioChannel>(micrefConfig.channels);
    micrefInfo.format = static_cast<AudioSampleFormat>(micrefConfig.format / BITLENGTH - 1);
    capturerEffectConfigMap_.emplace(HPAE_SOURCE_BUFFER_TYPE_MIC, micInfo);
    capturerEffectConfigMap_.emplace(HPAE_SOURCE_BUFFER_TYPE_EC, ecInfo);
    capturerEffectConfigMap_.emplace(HPAE_SOURCE_BUFFER_TYPE_MICREF, micrefInfo);
}

int32_t HpaeCaptureEffectNode::CaptureEffectCreate(uint64_t sceneKeyCode, CaptureEffectAttr attr)
{
    sceneKeyCode_ = sceneKeyCode;
    AudioEnhanceChainManager *audioEnhanceChainManager = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainManager, ERR_ILLEGAL_STATE, "audioEnhanceChainManager is nullptr");
    AudioEnhanceDeviceAttr enhanceAttr = {};
    enhanceAttr.micChannels = attr.micChannels;
    enhanceAttr.ecChannels = attr.ecChannels;
    enhanceAttr.micRefChannels = attr.micRefChannels;
    int32_t ret = audioEnhanceChainManager->CreateAudioEnhanceChainDynamic(sceneKeyCode, enhanceAttr);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR, "CreateAudioEnhanceChainDynamic failed, ret:%{public}d", ret);
    audioEnhanceChainManager->InitEnhanceBuffer();

    AudioBufferConfig micConfig = {};
    AudioBufferConfig ecConfig = {};
    AudioBufferConfig micrefConfig = {};
    ret = audioEnhanceChainManager->AudioEnhanceChainGetAlgoConfig(sceneKeyCode, micConfig, ecConfig, micrefConfig);
    CHECK_AND_RETURN_RET_LOG(ret == 0 && micConfig.samplingRate != 0, ERROR,
        "get algo config failed, ret:%{public}d", ret);
    SetCapturerEffectConfig(micConfig, ecConfig, micrefConfig);
    micBufferLength_ = FRAME_LEN * micConfig.channels * (micConfig.samplingRate / MILLISECOND_PER_SECOND) *
        (micConfig.format / BITLENGTH);
    ecBufferLength_ = FRAME_LEN * ecConfig.channels * (ecConfig.samplingRate / MILLISECOND_PER_SECOND) *
        (ecConfig.format / BITLENGTH);
    micrefBufferLength_ = FRAME_LEN * micrefConfig.channels * (micrefConfig.samplingRate / MILLISECOND_PER_SECOND) *
        (micrefConfig.format / BITLENGTH);
    uint32_t maxLength = (micBufferLength_ > ecBufferLength_) ?
        (micBufferLength_ > micrefBufferLength_ ? micBufferLength_ : micrefBufferLength_) :
        (ecBufferLength_ > micrefBufferLength_ ? ecBufferLength_ : micrefBufferLength_);
    AUDIO_INFO_LOG("micLength: %{public}u, ecLength: %{public}u, micrefLength: %{public}u, maxLength:%{public}u",
        micBufferLength_, ecBufferLength_, micrefBufferLength_, maxLength);
    cacheDataIn_.resize(maxLength);
    cacheDataOut_.resize(maxLength);
    return ret;
}

int32_t HpaeCaptureEffectNode::CaptureEffectRelease(uint64_t sceneKeyCode)
{
    AudioEnhanceChainManager *audioEnhanceChainManager = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainManager, ERR_ILLEGAL_STATE, "audioEnhanceChainManager is nullptr");
    return audioEnhanceChainManager->ReleaseAudioEnhanceChainDynamic(sceneKeyCode);
}

}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS
