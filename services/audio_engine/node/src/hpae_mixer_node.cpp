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
#define LOG_TAG "HpaeMixerNode"
#endif
#include <iostream>
#include "hpae_mixer_node.h"
#include "hpae_pcm_buffer.h"
#include "audio_utils.h"
#include "cinttypes"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
HpaeMixerNode::HpaeMixerNode(HpaeNodeInfo &nodeInfo)
    : HpaeNode(nodeInfo), HpaePluginNode(nodeInfo),
    pcmBufferInfo_(nodeInfo.channels, nodeInfo.frameLen, nodeInfo.samplingRate, nodeInfo.channelLayout),
    mixedOutput_(pcmBufferInfo_), tmpOutput_(pcmBufferInfo_)
{
#ifdef ENABLE_HOOK_PCM
    outputPcmDumper_ =  std::make_unique<HpaePcmDumper>("HpaeMixerNodeOut_ch_" +
        std::to_string(nodeInfo.channels) + "_scenType_" +
        std::to_string(GetSceneType()) + "_rate_" + std::to_string(GetSampleRate()) + ".pcm");
    AUDIO_INFO_LOG("HpaeMixerNode scene type is %{public}d", GetSceneType());
#endif
    mixedOutput_.SetSplitStreamType(nodeInfo.GetSplitStreamType());
}

bool HpaeMixerNode::Reset()
{
    return HpaePluginNode::Reset();
}

int32_t HpaeMixerNode::SetupAudioLimiter()
{
    if (limiter_ != nullptr) {
        AUDIO_INFO_LOG("NodeId: %{public}d, limiter has already been setup!", GetNodeId());
        return ERROR;
    }
    limiter_ = std::make_unique<AudioLimiter>(GetNodeId());
    // limiter only supports float format
    int32_t ret = limiter_->SetConfig(GetFrameLen() * GetChannelCount() * sizeof(float), sizeof(float), GetSampleRate(),
        GetChannelCount());
    if (ret == SUCCESS) {
        AUDIO_INFO_LOG("NodeId: %{public}d, limiter setup sucess!", GetNodeId());
    } else {
        limiter_ = nullptr;
        AUDIO_INFO_LOG("NodeId: %{public}d, limiter setup fail!!", GetNodeId());
    }
    return ret;
}

HpaePcmBuffer *HpaeMixerNode::SignalProcess(const std::vector<HpaePcmBuffer *> &inputs)
{
    Trace trace("HpaeMixerNode::SignalProcess");
    CHECK_AND_RETURN_RET_LOG(!inputs.empty(), nullptr, "inputs is empty");

    mixedOutput_.Reset();

    uint32_t bufferState = PCM_BUFFER_STATE_INVALID | PCM_BUFFER_STATE_SILENCE;
    if (limiter_ == nullptr) {
        if (CheckUpdateInfo(inputs[0])) {
            mixedOutput_.ReConfig(pcmBufferInfo_);
        }
        for (auto input: inputs) {
            mixedOutput_ += *input;
            bufferState &= input->GetBufferState();
        }
    } else { // limiter does not support reconfigging frameLen at runtime
        tmpOutput_.Reset();
        for (auto input: inputs) {
            tmpOutput_ += *input;
            bufferState &= input->GetBufferState();
        }
        limiter_->Process(GetFrameLen() * GetChannelCount(),
            tmpOutput_.GetPcmDataBuffer(), mixedOutput_.GetPcmDataBuffer());
    }
#ifdef ENABLE_HOOK_PCM
    outputPcmDumper_->CheckAndReopenHandlde();
    outputPcmDumper_->Dump((int8_t *)(mixedOutput_.GetPcmDataBuffer()),
        mixedOutput_.GetChannelCount() * sizeof(float) * mixedOutput_.GetFrameLen());
#endif
    mixedOutput_.SetBufferState(bufferState);
    return &mixedOutput_;
}

bool HpaeMixerNode::CheckUpdateInfo(HpaePcmBuffer *input)
{
    bool isPCMBufferInfoUpdated = false;
    if (pcmBufferInfo_.ch != input->GetChannelCount()) {
        AUDIO_INFO_LOG("Update channel count: %{public}d -> %{public}d",
            pcmBufferInfo_.ch, input->GetChannelCount());
        pcmBufferInfo_.ch = input->GetChannelCount();
        isPCMBufferInfoUpdated = true;
    }
    if (pcmBufferInfo_.frameLen != input->GetFrameLen()) {
        AUDIO_INFO_LOG("Update frame len %{public}d -> %{public}d",
            pcmBufferInfo_.frameLen, input->GetFrameLen());
        pcmBufferInfo_.frameLen = input->GetFrameLen();
        isPCMBufferInfoUpdated = true;
    }
    if (pcmBufferInfo_.rate != input->GetSampleRate()) {
        AUDIO_INFO_LOG("Update sample rate %{public}d -> %{public}d",
            pcmBufferInfo_.rate, input->GetSampleRate());
        pcmBufferInfo_.rate = input->GetSampleRate();
        isPCMBufferInfoUpdated = true;
    }
    if (pcmBufferInfo_.channelLayout != input->GetChannelLayout()) {
        AUDIO_INFO_LOG("Update channel layout %{public}" PRIu64 " -> %{public}" PRIu64 "",
            pcmBufferInfo_.channelLayout, input->GetChannelLayout());
        pcmBufferInfo_.channelLayout = input->GetChannelLayout();
        isPCMBufferInfoUpdated = true;
    }

    // if other bitwidth is supported, add check here

    return isPCMBufferInfoUpdated;
}

}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS