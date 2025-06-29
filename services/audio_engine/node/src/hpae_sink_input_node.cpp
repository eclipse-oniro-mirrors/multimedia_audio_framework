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
#define LOG_TAG "HpaeSinkInputNode"
#endif

#include "hpae_sink_input_node.h"
#include <iostream>
#include "hpae_format_convert.h"
#include "hpae_node_common.h"
#include "audio_engine_log.h"
#include "audio_errors.h"
#include "audio_utils.h"
#include "cinttypes"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
HpaeSinkInputNode::HpaeSinkInputNode(HpaeNodeInfo &nodeInfo)
    : HpaeNode(nodeInfo),
      pcmBufferInfo_(nodeInfo.channels, nodeInfo.frameLen, nodeInfo.samplingRate, (uint64_t)nodeInfo.channelLayout),
      emptyBufferInfo_(nodeInfo.channels, 0, nodeInfo.samplingRate, (uint64_t)nodeInfo.channelLayout),
      inputAudioBuffer_(pcmBufferInfo_), emptyAudioBuffer_(emptyBufferInfo_), outputStream_(this),
      interleveData_(nodeInfo.frameLen * nodeInfo.channels * GetSizeFromFormat(nodeInfo.format)), framesWritten_(0),
      totalFrames_(0)
{
    AUDIO_INFO_LOG("sinkinput sessionId %{public}d, channelcount %{public}d, channelLayout %{public}" PRIu64 ", "
        "frameLen %{public}d", nodeInfo.sessionId, inputAudioBuffer_.GetChannelCount(),
        inputAudioBuffer_.GetChannelLayout(), inputAudioBuffer_.GetFrameLen());

#ifdef ENABLE_HOOK_PCM
    inputPcmDumper_ = std::make_unique<HpaePcmDumper>(
        "HpaeSinkInputNode_id_" + std::to_string(GetSessionId()) + "_ch_" + std::to_string(GetChannelCount()) +
        "_rate_" + std::to_string(GetSampleRate()) + "_bit_" + std::to_string(GetBitWidth()) + ".pcm");
#endif
    if (nodeInfo.historyFrameCount > 0) {
        PcmBufferInfo pcmInfo = PcmBufferInfo{
            nodeInfo.channels, nodeInfo.frameLen, nodeInfo.samplingRate, nodeInfo.channelLayout,
                nodeInfo.historyFrameCount, true};
        historyBuffer_ = std::make_unique<HpaePcmBuffer>(pcmInfo);
        AUDIO_INFO_LOG("HpaeSinkInputNode::historybuffer created");
    } else {
        historyBuffer_ = nullptr;
    }
    if (nodeInfo.samplingRate == SAMPLE_RATE_11025) {
        pullDataFlag_ = true;
    }
}

HpaeSinkInputNode::~HpaeSinkInputNode()
{}

void HpaeSinkInputNode::CheckAndDestroyHistoryBuffer()
{
    HpaeNodeInfo nodeInfo = GetNodeInfo();
    // historyBuffer_ has no data, check if historyFrameCount is 0 and destroy it
    if (nodeInfo.historyFrameCount == 0) {
        if (historyBuffer_) {
            AUDIO_INFO_LOG("HpaeSinkInputNode::historyBuffer_ useless, destroy it");
        }
        historyBuffer_ = nullptr;
    } else if (historyBuffer_ == nullptr) {  // this case need to create historyBuffer_
        PcmBufferInfo pcmInfo = PcmBufferInfo{
            nodeInfo.channels, nodeInfo.frameLen, nodeInfo.samplingRate, nodeInfo.channelLayout,
                nodeInfo.historyFrameCount, true};
        historyBuffer_ = std::make_unique<HpaePcmBuffer>(pcmInfo);
        AUDIO_INFO_LOG("HpaeSinkInputNode::historybuffer created");
    }
}

int32_t HpaeSinkInputNode::GetDataFromSharedBuffer()
{
    streamInfo_ = {.framesWritten = framesWritten_,
        .inputData = interleveData_.data(),
        .requestDataLen = interleveData_.size(),
        .deviceClass = GetDeviceClass(),
        .deviceNetId = GetDeviceNetId(),
        .needData = !(historyBuffer_ && historyBuffer_->GetCurFrames())};
    GetCurrentPosition(streamInfo_.framePosition, streamInfo_.timestamp);
    auto writeCallback = writeCallback_.lock();
    if (writeCallback != nullptr) {
        return writeCallback->OnStreamData(streamInfo_);
    }
    AUDIO_ERR_LOG("sessionId: %{public}d, writeCallback is nullptr", GetSessionId());
    return SUCCESS;
}

bool HpaeSinkInputNode::ReadToAudioBuffer(int32_t &ret)
{
    auto nodeCallback = GetNodeStatusCallback().lock();
    if (nodeCallback) {
        nodeCallback->OnRequestLatency(GetSessionId(), streamInfo_.latency);
    }
    if (GetDeviceClass() == "offload" && !offloadEnable_) {
        ret = ERR_OPERATION_FAILED;
        AUDIO_WARNING_LOG("The session %{public}u offloadEnable is false, not request data", GetSessionId());
    } else {
        ret = GetDataFromSharedBuffer();
        if (GetSampleRate() == SAMPLE_RATE_11025) { // for 11025, skip pull data next time
            pullDataFlag_ = false;
        }
        // if historyBuffer has enough data, write to outputStream
        if (!streamInfo_.needData && historyBuffer_) {
            historyBuffer_->GetFrameData(inputAudioBuffer_);
            outputStream_.WriteDataToOutput(&inputAudioBuffer_);
            return false; // do not continue in DoProcess!
        }
        CheckAndDestroyHistoryBuffer();
        if (nodeCallback && ret) {
            nodeCallback->OnNodeStatusUpdate(GetSessionId(), OPERATION_UNDERFLOW);
            if (isDrain_) {
                AUDIO_INFO_LOG("OnNodeStatusUpdate Drain sessionId:%{public}u", GetSessionId());
                nodeCallback->OnNodeStatusUpdate(GetSessionId(), OPERATION_DRAINED);
                isDrain_ = false;
            }
        }
    }
    inputAudioBuffer_.SetBufferValid(ret ? false : true);
    return true; // continue in DoProcess!
}

void HpaeSinkInputNode::DoProcess()
{
    Trace trace("[" + std::to_string(GetSessionId()) + "]HpaeSinkInputNode::DoProcess " + GetTraceInfo());
    if (GetSampleRate() == SAMPLE_RATE_11025 && !pullDataFlag_) {
        // for 11025 input sample rate, pull 40ms data at a time, so pull once each two DoProcess()
        pullDataFlag_ = true;
        outputStream_.WriteDataToOutput(&emptyAudioBuffer_);
        return;
    }

    int32_t ret = SUCCESS;

    if (!ReadToAudioBuffer(ret)) {
        return;
    }

#ifdef ENABLE_HOOK_PCM
    if (inputPcmDumper_ != nullptr && inputAudioBuffer_.IsValid()) {
        inputPcmDumper_->CheckAndReopenHandlde();
        inputPcmDumper_->Dump(static_cast<int8_t *>(interleveData_.data()),
            GetChannelCount() * GetFrameLen() * GetSizeFromFormat(GetBitWidth()));
    }
#endif
    
    ConvertToFloat(
        GetBitWidth(), GetChannelCount() * GetFrameLen(), interleveData_.data(), inputAudioBuffer_.GetPcmDataBuffer());
    if (ret != 0) {
        AUDIO_WARNING_LOG("request data is not enough sessionId:%{public}u", GetSessionId());
        memset_s(inputAudioBuffer_.GetPcmDataBuffer(), inputAudioBuffer_.Size(), 0, inputAudioBuffer_.Size());
    } else {
        totalFrames_ = totalFrames_ + GetFrameLen();
        framesWritten_ = totalFrames_;
        if (historyBuffer_) {
            historyBuffer_->StoreFrameData(inputAudioBuffer_);
        }
    }
    outputStream_.WriteDataToOutput(&inputAudioBuffer_);
}

bool HpaeSinkInputNode::Reset()
{
    return true;
}

bool HpaeSinkInputNode::ResetAll()
{
    return true;
}

std::shared_ptr<HpaeNode> HpaeSinkInputNode::GetSharedInstance()
{
    return shared_from_this();
}

OutputPort<HpaePcmBuffer *> *HpaeSinkInputNode::GetOutputPort()
{
    return &outputStream_;
}

bool HpaeSinkInputNode::RegisterWriteCallback(const std::weak_ptr<IStreamCallback> &callback)
{
    writeCallback_ = callback;
    return true;
}
// reset historyBuffer
void HpaeSinkInputNode::Flush()
{
    if (GetNodeInfo().historyFrameCount == 0) {
        historyBuffer_ = nullptr;
    } else if (historyBuffer_ && historyBuffer_->GetFrames() == GetNodeInfo().historyFrameCount) {
        historyBuffer_->Reset();
    } else {
        HpaeNodeInfo nodeInfo = GetNodeInfo();
        PcmBufferInfo pcmInfo = PcmBufferInfo{
            nodeInfo.channels, nodeInfo.frameLen, nodeInfo.samplingRate, nodeInfo.channelLayout,
                nodeInfo.historyFrameCount, true};
        historyBuffer_ = std::make_unique<HpaePcmBuffer>(pcmInfo);
    }
}

bool HpaeSinkInputNode::Drain()
{
    isDrain_ = true;
    return true;
}

int32_t HpaeSinkInputNode::SetState(HpaeSessionState renderState)
{
    AUDIO_INFO_LOG(" Sink[%{public}s]->Session[%{public}u - %{public}d] state change:[%{public}s]-->[%{public}s]",
        GetDeviceClass().c_str(), GetSessionId(), GetStreamType(), ConvertSessionState2Str(state_).c_str(),
        ConvertSessionState2Str(renderState).c_str());
    state_ = renderState;
    return SUCCESS;
}

HpaeSessionState HpaeSinkInputNode::GetState()
{
    return state_;
}

void HpaeSinkInputNode::SetAppUid(int32_t appUid)
{
    appUid_ = appUid;
}

int32_t HpaeSinkInputNode::GetAppUid()
{
    return appUid_;
}

uint64_t HpaeSinkInputNode::GetFramesWritten()
{
    return framesWritten_;
}

int32_t HpaeSinkInputNode::GetCurrentPosition(uint64_t &framePosition, std::vector<uint64_t> &timestamp)
{
    framePosition = GetFramesWritten();
    if (historyBuffer_) {
        framePosition = framePosition > historyBuffer_->GetCurFrames() * GetFrameLen()
                            ? framePosition - historyBuffer_->GetCurFrames() * GetFrameLen()
                            : 0;
    }
    ClockTime::GetAllTimeStamp(timestamp);
    return SUCCESS;
}

int32_t HpaeSinkInputNode::RewindHistoryBuffer(uint64_t rewindTime)
{
    CHECK_AND_RETURN_RET_LOG(historyBuffer_, ERROR, "historyBuffer_ is nullptr");
    AUDIO_INFO_LOG("HpaeSinkInputNode::rewind %{public}zu frames", ConvertUsToFrameCount(rewindTime, GetNodeInfo()));
    return historyBuffer_->RewindBuffer(ConvertUsToFrameCount(rewindTime, GetNodeInfo()));
}

void HpaeSinkInputNode::SetOffloadEnabled(bool offloadEnable)
{
    offloadEnable_ = offloadEnable;
}

bool HpaeSinkInputNode::GetOffloadEnabled()
{
    return offloadEnable_;
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS