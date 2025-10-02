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
#define LOG_TAG "HpaeRemoteSinkOutputNode"
#endif

#include <iostream>
#include "hpae_remote_sink_output_node.h"
#include "audio_errors.h"
#include "hpae_format_convert.h"
#include "audio_utils.h"
#include "hpae_node_common.h"
#include "audio_engine_log.h"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
namespace {
const std::string STREAM_TYPE_CHANGE = "stream_type_change";
const std::string STREAM_USAGE_CHANGE = "stream_usage_change";
}
HpaeRemoteSinkOutputNode::HpaeRemoteSinkOutputNode(HpaeNodeInfo &nodeInfo, HpaeSinkInfo &sinkInfo)
    : HpaeNode(nodeInfo),
      renderFrameData_(nodeInfo.frameLen * nodeInfo.channels * GetSizeFromFormat(nodeInfo.format)),
      interleveData_(nodeInfo.frameLen * nodeInfo.channels),
      needEmptyChunk_(sinkInfo.needEmptyChunk)
{
#ifdef ENABLE_HOOK_PCM
    outputMediaPcmDumper_ = std::make_unique<HpaePcmDumper>("HpaeRemoteSinkOutputNode_Out_Media_bit_" +
        std::to_string(GetBitWidth()) + "_ch_" + std::to_string(GetChannelCount()) + "_rate_" +
        std::to_string(GetSampleRate()) + ".pcm");
    outputNavigationPcmDumper_ = std::make_unique<HpaePcmDumper>("HpaeRemoteSinkOutputNode_Out_Navigation_bit_" +
        std::to_string(GetBitWidth()) + "_ch_" + std::to_string(GetChannelCount()) + "_rate_" +
        std::to_string(GetSampleRate()) + ".pcm");
    outputCommunicationPcmDumper_ = std::make_unique<HpaePcmDumper>("HpaeRemoteSinkOutputNode_Out_Communication_bit_" +
        std::to_string(GetBitWidth()) + "_ch_" + std::to_string(GetChannelCount()) + "_rate_" +
        std::to_string(GetSampleRate()) + ".pcm");
    AUDIO_INFO_LOG("name is %{public}s", sinkOutAttr_.adapterName.c_str());
#endif
#ifdef ENABLE_HIDUMP_DFX
    SetNodeName("hpaeRemoteSinkOutputNode");
    if (auto callback = GetNodeStatusCallback().lock()) {
        callback->OnNotifyDfxNodeInfo(true, 0, GetNodeInfo());
    }
#endif
}

HpaeRemoteSinkOutputNode::~HpaeRemoteSinkOutputNode()
{
#ifdef ENABLE_HIDUMP_DFX
    AUDIO_INFO_LOG("NodeId: %{public}u NodeName: %{public}s destructed.",
        GetNodeId(), GetNodeName().c_str());
#endif
}

void HpaeRemoteSinkOutputNode::HandleRemoteTiming()
{
    auto now = std::chrono::high_resolution_clock::now();
    remoteTimePoint_ += std::chrono::milliseconds(20);  // 20ms frameLen, need optimize
    if (remoteTimePoint_ > now) {
        remoteSleepTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(remoteTimePoint_ - now);
    } else {
        remoteSleepTime_ = std::chrono::milliseconds(0);
    }
    std::this_thread::sleep_for(remoteSleepTime_);
    AUDIO_DEBUG_LOG("remoteSleepTime_ %{public}lld", remoteSleepTime_.count());
}

void HpaeRemoteSinkOutputNode::HandlePcmDumping(HpaeSplitStreamType streamType, char* data, size_t size)
{
    auto handleDump = [&](auto& dumper) {
        if (dumper) {
            dumper->CheckAndReopenHandle();
            dumper->Dump(reinterpret_cast<int8_t*>(data), size);
        }
    };

    switch (streamType) {
        case HpaeSplitStreamType::STREAM_TYPE_MEDIA:
            handleDump(outputMediaPcmDumper_);
            break;
        case HpaeSplitStreamType::STREAM_TYPE_NAVIGATION:
            handleDump(outputNavigationPcmDumper_);
            break;
        default:
            handleDump(outputCommunicationPcmDumper_);
            break;
    }
}

void HpaeRemoteSinkOutputNode::NotifyHdiSetParamEvent(const std::string &key, uint32_t renderId, uint32_t typeOrUsage)
{
    HdiAdapterManager &manager = HdiAdapterManager::GetInstance();
    std::shared_ptr<IDeviceManager> deviceManager = manager.GetDeviceManager(HDI_DEVICE_MANAGER_TYPE_REMOTE);
    if (deviceManager == nullptr) {
        AUDIO_ERR_LOG("device manager is nullptr");
        return;
    }
    std::string val = std::to_string(renderId) + '-' + std::to_string(typeOrUsage);
    deviceManager->SetAudioParameter(this->GetNodeInfo().deviceNetId, AudioParamKey::PARAM_KEY_STATE, key, val);
}

void HpaeRemoteSinkOutputNode::NotifyStreamTypeChange(AudioStreamType type, HpaeSplitStreamType splitStreamType)
{
    if (splitStreamType != STREAM_TYPE_MEDIA) {
        return;
    }
    HpaeNodeInfo nodeInfo = GetNodeInfo();
    if (type == nodeInfo.streamType) {
        return;
    }
    int32_t renderId = audioRendererSink_->GetHdiRenderId(splitStreamType);
    if (renderId < 0) {
        AUDIO_ERR_LOG("the render id is less then zero");
        return;
    }
    this->NotifyHdiSetParamEvent(STREAM_TYPE_CHANGE, static_cast<uint32_t>(renderId), static_cast<uint32_t>(type));
    nodeInfo.streamType = type;
    SetNodeInfo(nodeInfo);
}

void HpaeRemoteSinkOutputNode::NotifyStreamUsageChange(StreamUsage usage, HpaeSplitStreamType splitStreamType)
{
    if (usage == this->usageMap_[splitStreamType]) {
        return;
    }
    int32_t renderId = audioRendererSink_->GetHdiRenderId(splitStreamType);
    if (renderId < 0) {
        AUDIO_ERR_LOG("the render id is less then zero");
        return;
    }
    this->NotifyHdiSetParamEvent(STREAM_USAGE_CHANGE, static_cast<uint32_t>(renderId), static_cast<uint32_t>(usage));
    this->usageMap_[splitStreamType] = usage;
}

void HpaeRemoteSinkOutputNode::DoProcess()
{
    auto rate = "rate[" + std::to_string(GetSampleRate()) + "]_";
    auto ch = "ch[" + std::to_string(GetChannelCount()) + "]_";
    auto len = "len[" + std::to_string(GetFrameLen()) + "]_";
    auto format = "bit[" + std::to_string(GetBitWidth()) + "]";
    Trace trace("HpaeRemoteSinkOutputNode::DoProcess " + rate + ch + len + format);
    if (audioRendererSink_ == nullptr) {
        AUDIO_WARNING_LOG("audioRendererSink_ is nullptr sessionId: %{public}u", GetSessionId());
        return;
    }
    std::vector<HpaePcmBuffer *> &outputVec = inputStream_.ReadPreOutputData();
    for (auto &outputData : outputVec) {
        if (outputData == nullptr || (!outputData->IsValid() && !needEmptyChunk_)) {
            continue;
        }
        HpaeSplitStreamType splitStreamType = outputData->GetSplitStreamType();
        AudioStreamType type = outputData->GetAudioStreamType();
        NotifyStreamTypeChange(type, splitStreamType);
        NotifyStreamUsageChange(outputData->IsValid() ? outputData->GetAudioStreamUsage() : STREAM_USAGE_UNKNOWN,
            splitStreamType);
        ConvertFromFloat(
            GetBitWidth(), GetChannelCount() * GetFrameLen(), outputData->GetPcmDataBuffer(), renderFrameData_.data());
        uint64_t writeLen = 0;
        char *renderFrameData = (char *)renderFrameData_.data();
#ifdef ENABLE_HOOK_PCM
        HandlePcmDumping(splitStreamType, renderFrameData, renderFrameData_.size());
#endif
        auto ret = audioRendererSink_->SplitRenderFrame(*renderFrameData, renderFrameData_.size(),
            writeLen, splitStreamType);
        if (ret != SUCCESS || writeLen != renderFrameData_.size()) {
            AUDIO_ERR_LOG("RenderFrame failed, SplitStreamType %{public}d", splitStreamType);
        }
    }
    HandleRemoteTiming(); // used to control remote RenderFrame tempo.
    return;
}

const char *HpaeRemoteSinkOutputNode::GetRenderFrameData(void)
{
    return renderFrameData_.data();
}

bool HpaeRemoteSinkOutputNode::Reset()
{
    const auto preOutputMap = inputStream_.GetPreOutputMap();
    for (const auto &preOutput : preOutputMap) {
        OutputPort<HpaePcmBuffer *> *output = preOutput.first;
        inputStream_.DisConnect(output);
    }
    return true;
}

bool HpaeRemoteSinkOutputNode::ResetAll()
{
    const auto preOutputMap = inputStream_.GetPreOutputMap();
    for (const auto &preOutput : preOutputMap) {
        OutputPort<HpaePcmBuffer *> *output = preOutput.first;
        std::shared_ptr<HpaeNode> hpaeNode = preOutput.second;
        if (hpaeNode->ResetAll()) {
            inputStream_.DisConnect(output);
        }
    }
    return true;
}

void HpaeRemoteSinkOutputNode::Connect(const std::shared_ptr<OutputNode<HpaePcmBuffer *>> &preNode)
{
    inputStream_.Connect(preNode->GetSharedInstance(), preNode->GetOutputPort());
#ifdef ENABLE_HIDUMP_DFX
    if (auto callback = GetNodeStatusCallback().lock()) {
        callback->OnNotifyDfxNodeInfo(true, GetNodeId(), preNode->GetSharedInstance()->GetNodeInfo());
    }
#endif
}

void HpaeRemoteSinkOutputNode::DisConnect(const std::shared_ptr<OutputNode<HpaePcmBuffer *>> &preNode)
{
    inputStream_.DisConnect(preNode->GetOutputPort());
#ifdef ENABLE_HIDUMP_DFX
    if (auto callback = GetNodeStatusCallback().lock()) {
        auto preNodeReal = preNode->GetSharedInstance();
        callback->OnNotifyDfxNodeInfo(false, preNodeReal->GetNodeId(), preNodeReal->GetNodeInfo());
    }
#endif
}

int32_t HpaeRemoteSinkOutputNode::GetRenderSinkInstance(const std::string &deviceClass, const std::string &deviceNetId)
{
    if (deviceNetId.empty()) {
        renderId_ = HdiAdapterManager::GetInstance().GetRenderIdByDeviceClass(deviceClass, HDI_ID_INFO_DEFAULT, true);
    } else {
        renderId_ = HdiAdapterManager::GetInstance().GetRenderIdByDeviceClass(deviceClass, deviceNetId, true);
    }
    audioRendererSink_ = HdiAdapterManager::GetInstance().GetRenderSink(renderId_, true);
    if (audioRendererSink_ == nullptr) {
        AUDIO_ERR_LOG("get sink fail, deviceClass: %{public}s, deviceNetId: %{public}s, renderId_: %{public}u",
            deviceClass.c_str(),
            deviceNetId.c_str(),
            renderId_);
        HdiAdapterManager::GetInstance().ReleaseId(renderId_);
        return ERROR;
    }
    return SUCCESS;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkInit(IAudioSinkAttr &attr)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }

    sinkOutAttr_ = attr;
    if (audioRendererSink_->IsInited()) {
        AUDIO_WARNING_LOG("audioRenderSink already inited");
        SetSinkState(STREAM_MANAGER_IDLE);
        return SUCCESS;
    }
#ifdef ENABLE_HOOK_PCM
    HighResolutionTimer timer;
    timer.Start();
#endif
    int32_t ret = audioRendererSink_->Init(attr);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
        "audioRendererSink_ init failed, errCode is %{public}d", ret);
    SetSinkState(STREAM_MANAGER_IDLE);
#ifdef ENABLE_HOOK_PCM
    timer.Stop();
    int64_t interval = timer.Elapsed();
    AUDIO_INFO_LOG("name %{public}s, RenderSinkInit Elapsed: %{public}" PRId64 " ms ret: %{public}d",
        sinkOutAttr_.adapterName.c_str(), interval, ret);
    std::string adapterName = sinkOutAttr_.adapterName;
    outputMediaPcmDumper_ = std::make_unique<HpaePcmDumper>(
        "HpaeRemoteSinkOutputNode_Media_" + adapterName + "_bit_" + std::to_string(GetBitWidth()) + "_ch_" +
        std::to_string(GetChannelCount()) + "_rate_" + std::to_string(GetSampleRate()) + ".pcm");
    outputNavigationPcmDumper_ = std::make_unique<HpaePcmDumper>(
        "HpaeRemoteSinkOutputNode_Navigation_" + adapterName + "_bit_" + std::to_string(GetBitWidth()) + "_ch_" +
        std::to_string(GetChannelCount()) + "_rate_" + std::to_string(GetSampleRate()) + ".pcm");
    outputCommunicationPcmDumper_ = std::make_unique<HpaePcmDumper>(
        "HpaeRemoteSinkOutputNode_Communication_" + adapterName + "_bit_" + std::to_string(GetBitWidth()) + "_ch_" +
        std::to_string(GetChannelCount()) + "_rate_" + std::to_string(GetSampleRate()) + ".pcm");
#endif
    return ret;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkDeInit(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    SetSinkState(STREAM_MANAGER_RELEASED);
#ifdef ENABLE_HOOK_PCM
    HighResolutionTimer timer;
    timer.Start();
#endif
    audioRendererSink_->DeInit();
    audioRendererSink_ = nullptr;
    HdiAdapterManager::GetInstance().ReleaseId(renderId_);
#ifdef ENABLE_HOOK_PCM
    timer.Stop();
    int64_t interval = timer.Elapsed();
    AUDIO_INFO_LOG("name %{public}s, Elapsed: %{public}" PRId64 " ms", sinkOutAttr_.adapterName.c_str(), interval);
#endif
    return SUCCESS;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkFlush(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    return audioRendererSink_->Flush();
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkPause(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    audioRendererSink_->Pause();
    SetSinkState(STREAM_MANAGER_SUSPENDED);
    return SUCCESS;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkReset(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    return audioRendererSink_->Reset();
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkResume(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    int32_t ret = audioRendererSink_->Resume();
    if (ret != SUCCESS) {
        return ret;
    }
    SetSinkState(STREAM_MANAGER_RUNNING);
    return SUCCESS;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkStart(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }

    int32_t ret;
#ifdef ENABLE_HOOK_PCM
    HighResolutionTimer timer;
    timer.Start();
#endif
    ret = audioRendererSink_->Start();
    if (ret != SUCCESS) {
        return ERROR;
    }
#ifdef ENABLE_HOOK_PCM
    timer.Stop();
    int64_t interval = timer.Elapsed();
    AUDIO_INFO_LOG("name %{public}s, Elapsed: %{public}" PRId64 " ms", sinkOutAttr_.adapterName.c_str(), interval);
#endif
    SetSinkState(STREAM_MANAGER_RUNNING);
    remoteTimePoint_ = std::chrono::high_resolution_clock::now();
    return SUCCESS;
}

int32_t HpaeRemoteSinkOutputNode::RenderSinkStop(void)
{
    if (audioRendererSink_ == nullptr) {
        return ERROR;
    }
    SetSinkState(STREAM_MANAGER_SUSPENDED);
    int32_t ret;
#ifdef ENABLE_HOOK_PCM
    HighResolutionTimer timer;
    timer.Start();
#endif
    ret = audioRendererSink_->Stop();
    if (ret != SUCCESS) {
        return ret;
    }
#ifdef ENABLE_HOOK_PCM
    timer.Stop();
    int64_t interval = timer.Elapsed();
    AUDIO_INFO_LOG("name %{public}s, Elapsed: %{public}" PRId64 " ms", sinkOutAttr_.adapterName.c_str(), interval);
#endif
    return SUCCESS;
}

StreamManagerState HpaeRemoteSinkOutputNode::GetSinkState(void)
{
    return state_;
}

int32_t HpaeRemoteSinkOutputNode::SetSinkState(StreamManagerState sinkState)
{
    HILOG_COMM_INFO("Sink[%{public}s] state change:[%{public}s]-->[%{public}s]",
        GetDeviceClass().c_str(), ConvertStreamManagerState2Str(state_).c_str(),
        ConvertStreamManagerState2Str(sinkState).c_str());
        state_ = sinkState;
        return SUCCESS;
}

size_t HpaeRemoteSinkOutputNode::GetPreOutNum()
{
    return inputStream_.GetPreOutputNum();
}

int32_t HpaeRemoteSinkOutputNode::UpdateAppsUid(const std::vector<int32_t> &appsUid)
{
    CHECK_AND_RETURN_RET_LOG(audioRendererSink_ != nullptr, ERROR, "audioRendererSink_ is nullptr");
    CHECK_AND_RETURN_RET_LOG(audioRendererSink_->IsInited(), ERR_ILLEGAL_STATE, "audioRendererSink_ not init");
    return audioRendererSink_->UpdateAppsUid(appsUid);
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS
