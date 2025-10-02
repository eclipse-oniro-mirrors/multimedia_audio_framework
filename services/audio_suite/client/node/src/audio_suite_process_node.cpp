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
#define LOG_TAG "AudioSuitePorcessNode"
#endif

#include <vector>
#include <memory>
#include "audio_suite_process_node.h"

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {
AudioSuiteProcessNode::AudioSuiteProcessNode(AudioNodeType nodeType, AudioFormat audioFormat)
    : AudioNode(nodeType, audioFormat), inputStream_(std::make_shared<InputPort<AudioSuitePcmBuffer*>>())
{}

int32_t AudioSuiteProcessNode::DoProcess()
{
    if (GetAudioNodeDataFinishedFlag()) {
        AUDIO_DEBUG_LOG("Current node type = %{public}d does not have more data to process.", GetNodeType());
        return SUCCESS;
    }
    if (!outputStream_) {
        outputStream_ = std::make_shared<OutputPort<AudioSuitePcmBuffer*>>(GetSharedInstance());
    }
    if (!inputStream_) {
        AUDIO_ERR_LOG("node type = %{public}d inputstream is null!", GetNodeType());
        return ERR_INVALID_PARAM;
    }
    AudioSuitePcmBuffer* tempOut = nullptr;
    std::vector<AudioSuitePcmBuffer*>& preOutputs = ReadProcessNodePreOutputData();
    if ((GetNodeEnableStatus() == NODE_ENABLE) && !preOutputs.empty()) {
        AUDIO_DEBUG_LOG("node type = %{public}d need do SignalProcess.", GetNodeType());
        tempOut = SignalProcess(preOutputs);
        if (tempOut == nullptr) {
            AUDIO_ERR_LOG("node %{public}d do SignalProcess failed, return a nullptr", GetNodeType());
            return ERR_OPERATION_FAILED;
        }
    } else if (!preOutputs.empty()) {
        AUDIO_DEBUG_LOG("node type = %{public}d signalProcess is not enabled.", GetNodeType());
        tempOut = preOutputs[0];
        if (tempOut == nullptr) {
            AUDIO_ERR_LOG("node %{public}d get a null pcmbuffer from prenode", GetNodeType());
            return ERR_INVALID_READ;
        }
    } else {
        AUDIO_ERR_LOG("node %{public}d can't get pcmbuffer from prenodes", GetNodeType());
        return ERROR;
    }
    AUDIO_DEBUG_LOG("node type = %{public}d set "
        "pcmbuffer IsFinished: %{public}d.", GetNodeType(), GetAudioNodeDataFinishedFlag());
    tempOut->SetIsFinished(GetAudioNodeDataFinishedFlag());
    outputStream_->WriteDataToOutput(tempOut);
    HandleTapCallback(tempOut);
    return SUCCESS;
}

std::vector<AudioSuitePcmBuffer*>& AudioSuiteProcessNode::ReadProcessNodePreOutputData()
{
    if (!inputStream_) {
        AUDIO_ERR_LOG("node type = %{public}d inputstream is null! will trigger crash.", GetNodeType());
    }
    bool isFinished = true;
    auto& preOutputs = inputStream_->getInputDataRef();
    preOutputs.clear();
    auto& preOutputMap = inputStream_->GetPreOutputMap();
    for (auto& o : preOutputMap) {
        if (o.first == nullptr || !o.second) {
            AUDIO_ERR_LOG("node %{public}d has a invalid connection with prenode, "
                "node connection error.", GetNodeType());
            continue;
        }
        if (finishedPrenodeSet.find(o.second) != finishedPrenodeSet.end()) {
            AUDIO_DEBUG_LOG("current node type is %{public}d, it's prenode type = %{public}d is "
                "finished, skip this outputport.", GetNodeType(), o.second->GetNodeType());
            continue;
        }
        AudioSuitePcmBuffer* pcmData = o.first->PullOutputData();
        if (pcmData != nullptr) {
            AUDIO_DEBUG_LOG("node type = %{public}d send a pcmbuffer with isFinished: %{public}d to "
                "node type = %{public}d", o.second->GetNodeType(), pcmData->GetIsFinished(), GetNodeType());
            if (pcmData->GetIsFinished()) {
                finishedPrenodeSet.insert(o.second);
            }
            isFinished = isFinished && pcmData->GetIsFinished();
            preOutputs.emplace_back(std::move(pcmData));
        }
    }
    AUDIO_DEBUG_LOG("set node type = %{public}d isFinished status: %{public}d.", GetNodeType(), isFinished);
    SetAudioNodeDataFinishedFlag(isFinished);
    return preOutputs;
}

int32_t AudioSuiteProcessNode::Flush()
{
    finishedPrenodeSet.clear();
    return SUCCESS;
}

int32_t AudioSuiteProcessNode::Connect(const std::shared_ptr<AudioNode>& preNode, AudioNodePortType type)
{
    if (!inputStream_) {
        AUDIO_ERR_LOG("node type = %{public}d inputstream is null!", GetNodeType());
        return ERR_INVALID_PARAM;
    }
    if (!preNode) {
        AUDIO_ERR_LOG("node type = %{public}d preNode is null!", GetNodeType());
        return ERR_INVALID_PARAM;
    }
    inputStream_->Connect(preNode->GetSharedInstance(), preNode->GetOutputPort(type).get());
    return SUCCESS;
}

int32_t AudioSuiteProcessNode::DisConnect(const std::shared_ptr<AudioNode>& preNode)
{
    if (!inputStream_) {
        AUDIO_ERR_LOG("node type = %{public}d inputstream is null!", GetNodeType());
        return ERR_INVALID_PARAM;
    }
    inputStream_->DisConnect(preNode);
    return SUCCESS;
}

int32_t AudioSuiteProcessNode::InstallTap(AudioNodePortType portType,
    std::shared_ptr<SuiteNodeReadTapDataCallback> callback)
{
    tap_.SetAudioNodePortType(portType);
    tap_.SetOnReadTapDataCallback(callback);
    AUDIO_INFO_LOG("InstallTap SUCCESS, node type = %{public}d, tap portType = %{public}d",
        GetNodeType(), portType);
    return SUCCESS;
}
int32_t AudioSuiteProcessNode::RemoveTap(AudioNodePortType portType)
{
    tap_.SetOnReadTapDataCallback(nullptr);
    AUDIO_INFO_LOG("RemoveTap SUCCESS, node type = %{public}d, tap portType = %{public}d",
        GetNodeType(), portType);
    return SUCCESS;
}

void AudioSuiteProcessNode::HandleTapCallback(AudioSuitePcmBuffer* pcmBuffer)
{
    if (pcmBuffer == nullptr) {
        AUDIO_ERR_LOG("node %{public}d use a null pcmbuffer to HandleTapCallback.", GetNodeType());
        return;
    }
    if (!outputStream_) {
        outputStream_ = std::make_shared<OutputPort<AudioSuitePcmBuffer*>>(GetSharedInstance());
    }
    std::shared_ptr<SuiteNodeReadTapDataCallback> callback = tap_.GetOnReadTapDataCallback();
    CHECK_AND_RETURN(callback != nullptr);
    AudioNodePortType portType = outputStream_->GetPortType();
    AudioNodePortType tapType = tap_.GetAudioNodePortType();
    CHECK_AND_RETURN(portType == tapType);
    AUDIO_DEBUG_LOG("node type = %{public}d do OnReadTapDataCallback", GetNodeType());
    callback->OnReadTapDataCallback(static_cast<void*>(pcmBuffer->GetPcmDataBuffer()),
        pcmBuffer->GetFrameLen() * sizeof(float));
}

}
}
}