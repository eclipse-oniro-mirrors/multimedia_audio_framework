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
#define LOG_TAG "AudioSuiteManager"
#endif

#include <mutex>
#include <string>
#include <atomic>
#include <memory>
#include "audio_utils.h"
#include "audio_errors.h"
#include "audio_suite_log.h"
#include "audio_suite_engine.h"
#include "audio_suite_manager_private.h"
#include "audio_suite_manager_callback.h"

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {

namespace {
static const int32_t OPERATION_TIMEOUT_IN_MS = 10000;  // 10s
}

IAudioSuiteManager& IAudioSuiteManager::GetAudioSuiteManager()
{
    static AudioSuiteManager audioSuiteManager;
    return audioSuiteManager;
}

int32_t AudioSuiteManager::Init()
{
    AUDIO_INFO_LOG("Init enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ == nullptr, ERR_ILLEGAL_STATE, "suite engine aleay inited");

    suiteEngine_ = std::make_shared<AudioSuiteEngine>(*this);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr,
        ERR_MEMORY_ALLOC_FAILED, "Create suite engine failed, mallocl error.");

    int32_t ret = suiteEngine_->Init();
    if (ret != SUCCESS) {
        suiteEngine_ = nullptr;
        AUDIO_INFO_LOG("Aduio suite engine init failed. ret = %{public}d.", ret);
        return ret;
    }

    AUDIO_INFO_LOG("Init leave");
    return ret;
}

int32_t AudioSuiteManager::DeInit()
{
    AUDIO_INFO_LOG("DeInit enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_ILLEGAL_STATE, "suite engine not inited");

    int32_t ret = suiteEngine_->DeInit();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "suite engine deinit failed, ret = %{public}d.", ret);

    suiteEngine_ = nullptr;
    AUDIO_INFO_LOG("DeInit leave");
    return ret;
}

int32_t AudioSuiteManager::CreatePipeline(uint32_t &pipelineId)
{
    AUDIO_INFO_LOG("CreatePipeline enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_ENGINE_NOT_EXIST, "suite engine not inited");

    isFinishCreatePipeline_ = false;
    engineCreateResult_ = 0;
    int32_t ret = suiteEngine_->CreatePipeline();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine CreatePipeline failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishCreatePipeline_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "CreatePipeline timeout");

    AUDIO_INFO_LOG("CreatePipeline leave");
    pipelineId = engineCreatePipelineId_;
    engineCreatePipelineId_ = INVALID_PIPELINE_ID;
    return engineCreateResult_;
}

int32_t AudioSuiteManager::DestroyPipeline(uint32_t pipelineId)
{
    AUDIO_INFO_LOG("DestroyPipeline enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_PIPELINE_NOT_EXIST, "suite engine not inited");

    isFinishDestroyPipeline_ = false;
    int32_t ret = suiteEngine_->DestroyPipeline(pipelineId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine DestroyPipeline failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishDestroyPipeline_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "DestroyPipeline timeout");

    AUDIO_INFO_LOG("DestroyPipeline leave");
    return destroyPipelineResult_;
}

int32_t AudioSuiteManager::StartPipeline(uint32_t pipelineId)
{
    AUDIO_INFO_LOG("StartPipeline enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_PIPELINE_NOT_EXIST, "suite engine not inited");

    isFinishStartPipeline_ = false;
    int32_t ret = suiteEngine_->StartPipeline(pipelineId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine StartPipeline failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishStartPipeline_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "StartPipeline timeout");

    AUDIO_INFO_LOG("StartPipeline leave");
    return startPipelineResult_;
}

int32_t AudioSuiteManager::StopPipeline(uint32_t pipelineId)
{
    AUDIO_INFO_LOG("StopPipeline enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_PIPELINE_NOT_EXIST, "suite engine not inited");

    isFinishStopPipeline_ = false;
    int32_t ret = suiteEngine_->StopPipeline(pipelineId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine StopPipeline failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishStopPipeline_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "StopPipeline timeout");

    AUDIO_INFO_LOG("StopPipeline leave");
    return stopPipelineResult_;
}

int32_t AudioSuiteManager::GetPipelineState(uint32_t pipelineId, AudioSuitePipelineState &state)
{
    AUDIO_INFO_LOG("GetPipelineState enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_PIPELINE_NOT_EXIST, "suite engine not inited");

    isFinishGetPipelineState_ = false;
    getPipelineState_ = PIPELINE_STOPPED;
    int32_t ret = suiteEngine_->GetPipelineState(pipelineId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine GetPipelineState failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishGetPipelineState_;  // will be true when got notified.
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "GetPipelineState timeout");

    AUDIO_INFO_LOG("GetPipelineState leave");
    state = getPipelineState_;
    return SUCCESS;
}

int32_t AudioSuiteManager::CreateNode(uint32_t pipelineId, AudioNodeBuilder &builder, uint32_t &nodeId)
{
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_ENGINE_NOT_EXIST, "suite engine not inited");

    isFinishCreateNode_ = false;
    engineCreateNodeResult_ = 0;
    int32_t ret = suiteEngine_->CreateNode(pipelineId, builder);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, INVALID_NODE_ID, "engine CreateNode failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishCreateNode_;  // will be true when got notified.
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, INVALID_NODE_ID, "CreateNode timeout");

    AUDIO_INFO_LOG("CreateNode leave");
    nodeId = engineCreateNodeId_;
    engineCreateNodeId_ = INVALID_NODE_ID;
    return engineCreateNodeResult_;
}

int32_t AudioSuiteManager::DestroyNode(uint32_t nodeId)
{
    AUDIO_INFO_LOG("DestroyNode enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishDestroyNode_ = false;
    int32_t ret = suiteEngine_->DestroyNode(nodeId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine DestroyNode failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishDestroyNode_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "DestroyNode timeout");

    AUDIO_INFO_LOG("DestroyNode leave");
    return destroyNodeResult_;
}

int32_t AudioSuiteManager::EnableNode(uint32_t nodeId, AudioNodeEnable audioNodeEnable)
{
    AUDIO_INFO_LOG("EnableNode enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishEnableNode_ = false;
    int32_t ret = suiteEngine_->EnableNode(nodeId, audioNodeEnable);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine EnableNode failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishEnableNode_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "EnableNode timeout");

    AUDIO_INFO_LOG("EnableNode leave");
    return enableNodeResult_;
}

int32_t AudioSuiteManager::GetNodeEnableStatus(uint32_t nodeId, AudioNodeEnable &nodeEnable)
{
    AUDIO_INFO_LOG("GetNodeEnableStatus enter.");

    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishGetNodeEnable_ = false;
    getNodeEnable_ = NODE_DISABLE;
    int32_t ret = suiteEngine_->GetNodeEnableStatus(nodeId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine GetNodeEnableStatus failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishGetNodeEnable_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "GetNodeEnableStatus timeout");

    AUDIO_INFO_LOG("GetNodeEnableStatus leave");
    nodeEnable = getNodeEnable_;
    return SUCCESS;
}

int32_t AudioSuiteManager::SetAudioFormat(uint32_t nodeId, AudioFormat audioFormat)
{
    AUDIO_INFO_LOG("SetAudioFormat enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishSetFormat_ = false;
    int32_t ret = suiteEngine_->SetAudioFormat(nodeId, audioFormat);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine SetAudioFormat failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishSetFormat_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "SetAudioFormat timeout");

    AUDIO_INFO_LOG("SetAudioFormat leave");
    return setFormatResult_;
}

int32_t AudioSuiteManager::SetOnWriteDataCallback(uint32_t nodeId,
    std::shared_ptr<SuiteInputNodeWriteDataCallBack> callback)
{
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishSetWriteData_ = false;
    int32_t ret = suiteEngine_->SetWriteDataCallback(nodeId, callback);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine SetWriteDataCallback failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishSetWriteData_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "SetWriteDataCallback timeout");

    AUDIO_INFO_LOG("SetWriteDataCallback leave");
    return setWriteDataResult_;
}

int32_t AudioSuiteManager::ConnectNodes(uint32_t srcNodeId, uint32_t destNodeId,
    AudioNodePortType srcPortType, AudioNodePortType destPortType)
{
    AUDIO_INFO_LOG("ConnectNodes enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishConnectNodes_ = false;
    int32_t ret = suiteEngine_->ConnectNodes(srcNodeId, destNodeId, srcPortType, destPortType);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine ConnectNodes failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishConnectNodes_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "ConnectNodes timeout");

    AUDIO_INFO_LOG("ConnectNodes leave");
    return connectNodesResult_;
}

int32_t AudioSuiteManager::DisConnectNodes(uint32_t srcNodeId, uint32_t destNodeId)
{
    AUDIO_INFO_LOG("DisConnectNodes enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    isFinishDisConnectNodes_ = false;
    int32_t ret = suiteEngine_->DisConnectNodes(srcNodeId, destNodeId);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine DisConnectNodes failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishDisConnectNodes_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "DisConnectNodes timeout");

    AUDIO_INFO_LOG("DisConnectNodes leave");
    return disConnectNodesResult_;
}

int32_t AudioSuiteManager::SetEqualizerMode(uint32_t nodeId, EqualizerMode eqMode)
{
    AUDIO_INFO_LOG("SetEqualizerMode enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    // check
    std::string name = "EqualizerMode";
    std::string value = std::to_string(static_cast<int32_t>(eqMode));
    int32_t ret = suiteEngine_->SetOptions(nodeId, name, value);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine SetEqualizerMode failed, ret = %{public}d", ret);
    return ret;
}

int32_t AudioSuiteManager::SetEqualizerFrequencyBandGains(uint32_t nodeId, AudioEqualizerFrequencyBandGains gains)
{
    AUDIO_INFO_LOG("SetEqualizerFrequencyBandGains enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    // check
    std::string name = "AudioEqualizerFrequencyBandGains";
    std::string value = "";
    for (size_t idx = 0; idx < sizeof(gains.gains) / sizeof(gains.gains[0]); idx++) {
        value += std::to_string(gains.gains[idx]);
        value += ":";
    }
    int32_t ret = suiteEngine_->SetOptions(nodeId, name, value);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret,
        "engine SetEqualizerFrequencyBandGains failed, ret = %{public}d", ret);
    return ret;
}

int32_t AudioSuiteManager::SetSoundFieldType(uint32_t nodeId, SoundFieldType soundFieldType)
{
    AUDIO_INFO_LOG("SetSoundFieldType enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    // check
    std::string name = "SoundFieldType";
    std::string value = std::to_string(static_cast<int32_t>(soundFieldType));
    int32_t ret = suiteEngine_->SetOptions(nodeId, name, value);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine SetSoundFieldType failed, ret = %{public}d", ret);
    return ret;
}

int32_t AudioSuiteManager::SetEnvironmentType(uint32_t nodeId, EnvironmentType environmentType)
{
    AUDIO_INFO_LOG("EnvironmentType enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    // check
    std::string name = "EnvironmentType";
    std::string value = std::to_string(static_cast<int32_t>(environmentType));
    int32_t ret = suiteEngine_->SetOptions(nodeId, name, value);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine EnvironmentType failed, ret = %{public}d", ret);
    return ret;
}

int32_t AudioSuiteManager::SetVoiceBeautifierType(uint32_t nodeId, VoiceBeautifierType voiceBeautifierType)
{
    AUDIO_INFO_LOG("SetVoiceBeautifierType enter.");
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST, "suite engine not inited");

    // check
    std::string name = "VoiceBeautifierType";
    std::string value = std::to_string(static_cast<int32_t>(voiceBeautifierType));
    int32_t ret = suiteEngine_->SetOptions(nodeId, name, value);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine SetVoiceBeautifierType failed, ret = %{public}d", ret);
    return ret;
}

int32_t AudioSuiteManager::InstallTap(uint32_t nodeId, AudioNodePortType portType,
    std::shared_ptr<SuiteNodeReadTapDataCallback> callback)
{
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST,
        "InstallTap failed suite engine not inited");

    isFinisInstallTap_ = false;
    int32_t ret = suiteEngine_->InstallTap(nodeId, portType, callback);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine InstallTap failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinisInstallTap_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "InstallTap timeout");

    AUDIO_INFO_LOG("InstallTap leave");
    return installTapResult_;
}

int32_t AudioSuiteManager::RemoveTap(uint32_t nodeId, AudioNodePortType portType)
{
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_NODE_NOT_EXIST,
        "InstallTap failed suite engine not inited");

    isFinisRemoveTap_ = false;
    int32_t ret = suiteEngine_->RemoveTap(nodeId, portType);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine RemoveTap failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinisRemoveTap_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "RemoveTap timeout");

    AUDIO_INFO_LOG("RemoveTap leave");
    return removeTapResult_;
}

int32_t AudioSuiteManager::RenderFrame(uint32_t pipelineId,
    uint8_t *audioData, int32_t frameSize, int32_t *writeLen, bool *finishedFlag)
{
    std::lock_guard<std::mutex> lock(lock_);
    CHECK_AND_RETURN_RET_LOG(suiteEngine_ != nullptr, ERR_AUDIO_SUITE_PIPELINE_NOT_EXIST, "suite engine not inited");

    isFinisRenderFrame_ = false;
    int32_t ret = suiteEngine_->RenderFrame(pipelineId, audioData, frameSize, writeLen, finishedFlag);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "engine RenderFrame failed, ret = %{public}d", ret);

    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinisRenderFrame_;
    });
    CHECK_AND_RETURN_RET_LOG(stopWaiting, ERROR, "RenderFrame timeout");

    AUDIO_INFO_LOG("RenderFrame leave");
    return renderFrameResult_;
}

void AudioSuiteManager::OnCreatePipeline(int32_t result, uint32_t pipelineId)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnCreatePipeline enter");
    isFinishCreatePipeline_ = true;
    engineCreateResult_ = result;
    engineCreatePipelineId_ = pipelineId;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnDestroyPipeline(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnDestroyPipeline result: %{public}d", result);
    isFinishDestroyPipeline_ = true;
    destroyPipelineResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnStartPipeline(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnStartPipeline enter");
    isFinishStartPipeline_ = true;
    startPipelineResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnStopPipeline(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnStopPipeline enter");
    isFinishStopPipeline_ = true;
    stopPipelineResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnGetPipelineState(AudioSuitePipelineState state)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnGetPipelineState enter");
    isFinishGetPipelineState_ = true;
    getPipelineState_ = state;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnCreateNode(int32_t result, uint32_t nodeId)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnCreateNode enter");
    isFinishCreateNode_ = true;
    engineCreateNodeResult_ = result;
    engineCreateNodeId_ = nodeId;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnDestroyNode(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnDestroyNode enter");
    isFinishDestroyNode_ = true;
    destroyNodeResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnEnableNode(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnEnableNode enter");
    isFinishEnableNode_ = true;
    enableNodeResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnGetNodeEnable(AudioNodeEnable enable)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnGetNodeEnable enter");
    isFinishGetNodeEnable_ = true;
    getNodeEnable_ = enable;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnSetAudioFormat(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnSetAudioFormat enter");
    isFinishSetFormat_ = true;
    setFormatResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnWriteDataCallback(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnWriteDataCallback enter");
    isFinishSetWriteData_ = true;
    setWriteDataResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnConnectNodes(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnConnectNodes enter");
    isFinishConnectNodes_ = true;
    connectNodesResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnDisConnectNodes(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnDisConnectNodes enter");
    isFinishDisConnectNodes_ = true;
    disConnectNodesResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnInstallTap(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnInstallTap callback");
    isFinisInstallTap_ = true;
    installTapResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnRemoveTap(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("RemoveTap callback");
    isFinisRemoveTap_ = true;
    removeTapResult_ = result;
    callbackCV_.notify_all();
}

void AudioSuiteManager::OnRenderFrame(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    AUDIO_INFO_LOG("OnRenderFrame callback");
    isFinisRenderFrame_ = true;
    renderFrameResult_ = result;
    callbackCV_.notify_all();
}

}  // namespace AudioSuite
}  // namespace AudioStandard
}  // namespace OHOS
