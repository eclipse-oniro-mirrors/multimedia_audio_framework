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
#define LOG_TAG "HpaeRendererManager"
#endif
#include "hpae_renderer_manager.h"
#include "audio_stream_info.h"
#include "audio_errors.h"
#include "audio_engine_log.h"
#include "hpae_node_common.h"
#include "audio_effect_chain_manager.h"
#include "audio_utils.h"
#include "audio_volume.h"

constexpr int32_t DEFAULT_EFFECT_RATE = 48000;
constexpr int32_t DEFAULT_EFFECT_FRAME_LEN = 960;

namespace OHOS {
namespace AudioStandard {
namespace HPAE {

HpaeRendererManager::HpaeRendererManager(HpaeSinkInfo &sinkInfo)
    : hpaeNoLockQueue_(CURRENT_REQUEST_COUNT), sinkInfo_(sinkInfo)
{}

HpaeRendererManager::~HpaeRendererManager()
{
    AUDIO_INFO_LOG("destructor renderer");
    if (isInit_.load()) {
        DeInit();
    }
}

bool HpaeRendererManager::IsMchDevice()
{
    return sinkInfo_.deviceName == "MCH_Speaker";
}

int32_t HpaeRendererManager::CreateInputSession(const HpaeStreamInfo &streamInfo)
{
    Trace trace("[" + std::to_string(streamInfo.sessionId) + "]HpaeRendererManager::CreateInputSession");
    HpaeNodeInfo nodeInfo;
    nodeInfo.channels = streamInfo.channels;
    nodeInfo.format = streamInfo.format;
    nodeInfo.frameLen = streamInfo.frameLen;
    nodeInfo.channelLayout = (AudioChannelLayout)streamInfo.channelLayout;
    nodeInfo.streamType = streamInfo.streamType;
    nodeInfo.sessionId = streamInfo.sessionId;
    nodeInfo.samplingRate = static_cast<AudioSamplingRate>(streamInfo.samplingRate);
    nodeInfo.sceneType = TransToProperSceneType(streamInfo.effectInfo.streamUsage, streamInfo.effectInfo.effectScene);
    nodeInfo.effectInfo = streamInfo.effectInfo;
    TransNodeInfoForCollaboration(nodeInfo, isCollaborationEnabled_);
    nodeInfo.fadeType = streamInfo.fadeType;
    nodeInfo.statusCallback = weak_from_this();
    nodeInfo.deviceClass = sinkInfo_.deviceClass;
    nodeInfo.deviceNetId = sinkInfo_.deviceNetId;
    nodeInfo.nodeName = "HpaeSinkInputNode";
    nodeInfo.nodeId = OnGetNodeId();
    sinkInputNodeMap_[streamInfo.sessionId] = std::make_shared<HpaeSinkInputNode>(nodeInfo);
    sinkInputNodeMap_[streamInfo.sessionId]->SetAppUid(streamInfo.uid);
    AUDIO_INFO_LOG("streamType %{public}u, sessionId = %{public}u, current sceneType is %{public}d",
        nodeInfo.streamType,
        nodeInfo.sessionId,
        nodeInfo.sceneType);
    CreateProcessCluster(nodeInfo);
    if (!sessionNodeMap_[nodeInfo.sessionId].bypass) {
        CHECK_AND_RETURN_RET_LOG(SafeGetMap(sceneClusterMap_, nodeInfo.sceneType), ERROR,
            "could not find processorType %{public}d", nodeInfo.sceneType);
        sceneTypeToProcessClusterCountMap_[nodeInfo.sceneType]++;
        int32_t ret = sceneClusterMap_[nodeInfo.sceneType]->AudioRendererCreate(nodeInfo);
        if (ret != SUCCESS) {
            AUDIO_WARNING_LOG("update audio effect when creating failed, ret = %{public}d", ret);
        }
    }
    return SUCCESS;
}

int32_t HpaeRendererManager::AddNodeToSink(const std::shared_ptr<HpaeSinkInputNode> &node)
{
    auto request = [this, node]() { AddSingleNodeToSink(node); };
    SendRequest(request);
    return SUCCESS;
}

void HpaeRendererManager::AddSingleNodeToSink(const std::shared_ptr<HpaeSinkInputNode> &node, bool isConnect)
{
    Trace trace("HpaeRendererManager::AddSingleNodeToSink");
    HpaeNodeInfo nodeInfo = node->GetNodeInfo();
    nodeInfo.deviceClass = sinkInfo_.deviceClass;
    nodeInfo.deviceNetId = sinkInfo_.deviceNetId;
    // no need history buffer in not offload sink
    nodeInfo.historyFrameCount = 0;
    nodeInfo.nodeId = OnGetNodeId();
    nodeInfo.statusCallback = weak_from_this();
    nodeInfo.sceneType = TransToProperSceneType(nodeInfo.effectInfo.streamUsage, nodeInfo.effectInfo.effectScene);
    // for collaboration
    TransNodeInfoForCollaboration(nodeInfo, isCollaborationEnabled_);
    node->SetNodeInfo(nodeInfo);
    uint32_t sessionId = nodeInfo.sessionId;
    
    sinkInputNodeMap_[sessionId] = node;
    SetSessionState(sessionId, node->GetState());
    sessionNodeMap_[sessionId].sceneType = nodeInfo.sceneType;

    AUDIO_INFO_LOG("[FinishMove] session :%{public}u to sink:%{public}s, sceneType is %{public}d",
        sessionId, sinkInfo_.deviceClass.c_str(), nodeInfo.sceneType);
    HpaeNodeInfo processNodeInfo = nodeInfo;
    processNodeInfo.samplingRate = (AudioSamplingRate)DEFAULT_EFFECT_RATE;
    processNodeInfo.frameLen = (uint32_t)DEFAULT_EFFECT_FRAME_LEN;
    processNodeInfo.channels = STEREO;
    processNodeInfo.channelLayout = CH_LAYOUT_STEREO;
    CreateProcessCluster(processNodeInfo);
    if (!sessionNodeMap_[nodeInfo.sessionId].bypass) {
        CHECK_AND_RETURN_LOG(SafeGetMap(sceneClusterMap_, nodeInfo.sceneType),
            "[FinishMove] could not find processorType %{public}d", nodeInfo.sceneType);
        sceneTypeToProcessClusterCountMap_[nodeInfo.sceneType]++;
        int32_t ret = sceneClusterMap_[nodeInfo.sceneType]->AudioRendererCreate(nodeInfo);
        if (ret != SUCCESS) {
            AUDIO_WARNING_LOG("[FinishMove] session:%{public}u update audio effect when creating failed ret %{public}d",
                sessionId, ret);
        }
    }

    CHECK_AND_RETURN_LOG(isConnect == true, "[FinishMove] not need connect session:%{public}d", sessionId);
    if (node->GetState() == HPAE_SESSION_RUNNING) {
        AUDIO_INFO_LOG("[FinishMove] session:%{public}u connect to sink:%{public}s",
            sessionId, sinkInfo_.deviceClass.c_str());
        ConnectInputSession(sessionId);
    }
}

void HpaeRendererManager::CreateDefaultProcessCluster(HpaeNodeInfo &nodeInfo)
{
    AUDIO_INFO_LOG("use default processCluster");
    if (sceneClusterMap_.find(HPAE_SCENE_DEFAULT) == sceneClusterMap_.end()) {
        AUDIO_INFO_LOG("default processCluster is null, create default processCluster");
        HpaeNodeInfo temp = nodeInfo;
        temp.sceneType = HPAE_SCENE_DEFAULT;
        auto hpaeProcessCluster = std::make_shared<HpaeProcessCluster>(temp, sinkInfo_);
        sceneClusterMap_[HPAE_SCENE_DEFAULT] = hpaeProcessCluster;
        sceneClusterMap_[nodeInfo.sceneType] = hpaeProcessCluster;
        sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT]++;
    } else {
        sceneClusterMap_[nodeInfo.sceneType] = sceneClusterMap_[HPAE_SCENE_DEFAULT];
        sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT]++;
    }
    return;
}

void HpaeRendererManager::CreateProcessCluster(HpaeNodeInfo &nodeInfo)
{
    Trace trace("HpaeRendererManager::CreateProcessCluster");
    std::string sceneType = TransProcessorTypeToSceneType(nodeInfo.sceneType);
    int32_t processClusterDecision = AudioEffectChainManager::GetInstance()->CheckProcessClusterInstances(sceneType);
    HpaeNodeInfo temp = nodeInfo;
    std::shared_ptr<HpaeProcessCluster> hpaeProcessCluster = nullptr;
    switch (processClusterDecision) {
        case NO_NEED_TO_CREATE_PROCESSCLUSTER:
            AUDIO_INFO_LOG("no need to create processCluster");
            if (!SafeGetMap(sceneClusterMap_, nodeInfo.sceneType)) {
                AUDIO_INFO_LOG("processCluster is null, create a new processCluster");
                sceneClusterMap_[nodeInfo.sceneType] = std::make_shared<HpaeProcessCluster>(nodeInfo, sinkInfo_);
            }
            break;
        case CREATE_NEW_PROCESSCLUSTER:
            AUDIO_INFO_LOG("create new processCluster");
            sceneClusterMap_[nodeInfo.sceneType] = std::make_shared<HpaeProcessCluster>(nodeInfo, sinkInfo_);
            break;
        case CREATE_DEFAULT_PROCESSCLUSTER:
            temp.sceneType = HPAE_SCENE_DEFAULT;
            AUDIO_INFO_LOG("begin control, create default processCluster");
            hpaeProcessCluster = std::make_shared<HpaeProcessCluster>(temp, sinkInfo_);
            sceneClusterMap_[HPAE_SCENE_DEFAULT] = hpaeProcessCluster;
            sceneClusterMap_[nodeInfo.sceneType] = hpaeProcessCluster;
            sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT]++;
            break;
        case USE_DEFAULT_PROCESSCLUSTER:
            CreateDefaultProcessCluster(nodeInfo);
            break;
        case USE_NONE_PROCESSCLUSTER:
            AUDIO_INFO_LOG("use none processCluster");
            break;
        case CREATE_EXTRA_PROCESSCLUSTER:
            AUDIO_INFO_LOG("out of control");
            if (!SafeGetMap(sceneClusterMap_, nodeInfo.sceneType)) {
                AUDIO_INFO_LOG("out of control, create a new processCluster");
                sceneClusterMap_[nodeInfo.sceneType] = std::make_shared<HpaeProcessCluster>(nodeInfo, sinkInfo_);
            }
            break;
        default:
            break;
    }
    if (processClusterDecision == USE_NONE_PROCESSCLUSTER) {
        sessionNodeMap_[nodeInfo.sessionId].bypass = true;
    }
    return;
}

int32_t HpaeRendererManager::AddAllNodesToSink(
    const std::vector<std::shared_ptr<HpaeSinkInputNode>> &sinkInputs, bool isConnect)
{
    auto request = [this, sinkInputs, isConnect]() {
        for (const auto &it : sinkInputs) {
            AddSingleNodeToSink(it, isConnect);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

HpaeProcessorType HpaeRendererManager::TransToProperSceneType(StreamUsage streamUsage, AudioEffectScene effectScene)
{
    if (sinkInfo_.lib == "libmodule-split-stream-sink.z.so") {
        return TransStreamUsageToSplitSceneType(streamUsage, sinkInfo_.splitMode);
    } else if (sinkInfo_.deviceClass == "remote" || IsMchDevice()) {
        return HPAE_SCENE_EFFECT_NONE;
    } else {
        return TransEffectSceneToSceneType(effectScene);
    }
}

HpaeProcessorType HpaeRendererManager::GetProcessorType(uint32_t sessionId)
{
    HpaeNodeInfo nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
    if ((sessionNodeMap_[sessionId].bypass || nodeInfo.effectInfo.effectMode == EFFECT_NONE) &&
        (!isSplitProcessorType(nodeInfo.sceneType))) {
            return HPAE_SCENE_EFFECT_NONE;
    }
    return nodeInfo.sceneType;
}

int32_t HpaeRendererManager::CreateStream(const HpaeStreamInfo &streamInfo)
{
    if (!IsInit()) {
        return ERR_INVALID_OPERATION;
    }
    auto request = [this, streamInfo]() {
        Trace trace("HpaeRendererManager::CreateStream id[" +
            std::to_string(streamInfo.sessionId) + "]");
        AUDIO_INFO_LOG("CreateStream sessionId %{public}u deviceName %{public}s",
            streamInfo.sessionId,
            sinkInfo_.deviceName.c_str());
        CreateInputSession(streamInfo);
        SetSessionState(streamInfo.sessionId, HPAE_SESSION_PREPARED);
        sessionNodeMap_[streamInfo.sessionId].isMoveAble = streamInfo.isMoveAble;
        sinkInputNodeMap_[streamInfo.sessionId]->SetState(HPAE_SESSION_PREPARED);
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::DestroyStream(uint32_t sessionId)
{
    if (!IsInit()) {
        return ERR_INVALID_OPERATION;
    }
    auto request = [this, sessionId]() {
        Trace trace("HpaeRendererManager::DestroyStream id[" +
            std::to_string(sessionId) + "]");
        AUDIO_INFO_LOG("DestroyStream sessionId %{public}u", sessionId);
        CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
            "Release not find sessionId %{public}u", sessionId);
        SetSessionState(sessionId, HPAE_SESSION_RELEASED);
        sinkInputNodeMap_[sessionId]->SetState(HPAE_SESSION_RELEASED);
        DeleteInputSession(sessionId);
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::DeleteInputSession(uint32_t sessionId)
{
    Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::DeleteInputSession");
    if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
        AUDIO_INFO_LOG("could not find session:%{public}d", sessionId);
        return SUCCESS;
    }
    HpaeNodeInfo nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
    HpaeProcessorType sceneType = GetProcessorType(sessionId);
    if (SafeGetMap(sceneClusterMap_, sceneType)) {
        DeleteProcessCluster(nodeInfo, sceneType, sessionId);
    }
    sinkInputNodeMap_.erase(sessionId);
    return SUCCESS;
}

void HpaeRendererManager::DeleteProcessCluster(
    const HpaeNodeInfo &nodeInfo, HpaeProcessorType sceneType, uint32_t sessionId)
{
    Trace trace("[" + std::to_string(sessionId) +
        "]HpaeRendererManager::DeleteProcessCluster sceneType:" + std::to_string(sessionId));
    if (!sessionNodeMap_[sessionId].bypass) {
        CHECK_AND_RETURN_LOG(SafeGetMap(sceneClusterMap_, nodeInfo.sceneType),
            "could not find processorType %{public}d", nodeInfo.sceneType);
        sceneClusterMap_[nodeInfo.sceneType]->AudioRendererRelease(sinkInputNodeMap_[sessionId]->GetNodeInfo());
    }
    sceneClusterMap_[sceneType]->DisConnect(sinkInputNodeMap_[sessionId]);
    if (sceneClusterMap_[sceneType]->GetPreOutNum() == 0) {
        // for collaboration
        if (sceneType == HPAE_SCENE_COLLABORATIVE && hpaeCoBufferNode_ != nullptr) {
            hpaeCoBufferNode_->DisConnect(sceneClusterMap_[sceneType]);
            TriggerCallback(DISCONNECT_CO_BUFFER_NODE, hpaeCoBufferNode_);
        }
        outputCluster_->DisConnect(sceneClusterMap_[sceneType]);
        sceneClusterMap_[sceneType]->SetConnectedFlag(false);
    }
    if (!sessionNodeMap_[sessionId].bypass) {
        sceneTypeToProcessClusterCountMap_[nodeInfo.sceneType]--;
        if (sceneClusterMap_[nodeInfo.sceneType] == sceneClusterMap_[HPAE_SCENE_DEFAULT]) {
            sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT]--;
        }
        AUDIO_INFO_LOG("sceneType %{public}d is deleted, current count: %{public}d, default count: %{public}d",
            nodeInfo.sceneType, sceneTypeToProcessClusterCountMap_[nodeInfo.sceneType],
            sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT]);

        if (sceneTypeToProcessClusterCountMap_[nodeInfo.sceneType] == 0) {
            sceneClusterMap_.erase(nodeInfo.sceneType);
            sceneTypeToProcessClusterCountMap_.erase(nodeInfo.sceneType);
        }
        if (sceneTypeToProcessClusterCountMap_[HPAE_SCENE_DEFAULT] == 0) {
            sceneClusterMap_.erase(HPAE_SCENE_DEFAULT);
            sceneTypeToProcessClusterCountMap_.erase(HPAE_SCENE_DEFAULT);
        }
    }
}

bool HpaeRendererManager::isSplitProcessorType(HpaeProcessorType sceneType)
{
    if (sceneType == HPAE_SCENE_SPLIT_MEDIA || sceneType == HPAE_SCENE_SPLIT_NAVIGATION ||
        sceneType == HPAE_SCENE_SPLIT_COMMUNICATION) {
        return true;
    }
    return false;
}

int32_t HpaeRendererManager::ConnectInputSession(uint32_t sessionId)
{
    Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::ConnectInputSession");
    AUDIO_INFO_LOG("connect input session:%{public}d", sessionId);
    if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
        AUDIO_INFO_LOG("could not input node by sessionid:%{public}d", sessionId);
        return ERR_INVALID_PARAM;
    }
    if (sinkInputNodeMap_[sessionId]->GetState() != HPAE_SESSION_RUNNING) {
        return SUCCESS;
    }
    HpaeProcessorType sceneType = GetProcessorType(sessionId);
    if (SafeGetMap(sceneClusterMap_, sceneType)) {
        ConnectProcessCluster(sessionId, sceneType);
    }
    if (outputCluster_->GetState() != STREAM_MANAGER_RUNNING && !isSuspend_) {
        outputCluster_->Start();
    }
    return SUCCESS;
}

void HpaeRendererManager::ConnectProcessCluster(uint32_t sessionId, HpaeProcessorType sceneType)
{
    Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::ConnectProcessCluster sceneType:"
        + std::to_string(sceneType));
    int32_t ret = sceneClusterMap_[sceneType]->AudioRendererStart(sinkInputNodeMap_[sessionId]->GetNodeInfo());
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("update audio effect when starting failed, ret = %{public}d", ret);
    }
    if (!outputCluster_->IsProcessClusterConnected(sceneType) && !sceneClusterMap_[sceneType]->GetConnectedFlag()) {
        outputCluster_->Connect(sceneClusterMap_[sceneType]);
        sceneClusterMap_[sceneType]->SetConnectedFlag(true);
    }
    sceneClusterMap_[sceneType]->Connect(sinkInputNodeMap_[sessionId]);
    if (sceneType == HPAE_SCENE_COLLABORATIVE && hpaeCoBufferNode_ != nullptr) {
        uint32_t latency = outputCluster_->GetLatency();
        hpaeCoBufferNode_->SetLatency(latency);
        hpaeCoBufferNode_->Connect(sceneClusterMap_[sceneType]);
        TriggerCallback(CONNECT_CO_BUFFER_NODE, hpaeCoBufferNode_);
    }
}

void HpaeRendererManager::MoveAllStreamToNewSink(const std::string &sinkName,
    const std::vector<uint32_t>& moveIds, MoveSessionType moveType)
{
    Trace trace("HpaeRendererManager::MoveAllStreamToNewSink[" + sinkName + "]");
    std::string name = sinkName;
    std::vector<std::shared_ptr<HpaeSinkInputNode>> sinkInputs;
    std::vector<uint32_t> sessionIds;
    std::string idStr;
    for (const auto &it : sinkInputNodeMap_) {
        if (moveType == MOVE_ALL || std::find(moveIds.begin(), moveIds.end(), it.first) != moveIds.end()) {
            sinkInputs.emplace_back(it.second);
            sessionIds.emplace_back(it.first);
            idStr.append("[");
            idStr.append(std::to_string(it.first));
            idStr.append("],");
        }
    }
    for (const auto &it : sessionIds) {
        DisConnectInputSession(it);
    }
    AUDIO_INFO_LOG("[StartMove] session:%{public}s to sink name:%{public}s, move type:%{public}d",
        idStr.c_str(), name.c_str(), moveType);
    TriggerCallback(MOVE_ALL_SINK_INPUT, sinkInputs, name, moveType);
}

int32_t HpaeRendererManager::MoveAllStream(const std::string &sinkName, const std::vector<uint32_t>& sessionIds,
    MoveSessionType moveType)
{
    if (!IsInit()) {
        AUDIO_INFO_LOG("sink is not init ,use sync mode move to:%{public}s.", sinkName.c_str());
        MoveAllStreamToNewSink(sinkName, sessionIds, moveType);
    } else {
        AUDIO_INFO_LOG("sink is init ,use async mode move to:%{public}s.", sinkName.c_str());
        auto request = [this, sinkName, sessionIds, moveType]() {
            MoveAllStreamToNewSink(sinkName, sessionIds, moveType);
        };
        SendRequest(request);
    }
    return SUCCESS;
}

void HpaeRendererManager::MoveStreamSync(uint32_t sessionId, const std::string &sinkName)
{
    if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
        AUDIO_ERR_LOG("[StartMove] session:%{public}u failed,can not find session,move %{public}s --> %{public}s",
            sessionId, sinkInfo_.deviceName.c_str(), sinkName.c_str());
        TriggerCallback(MOVE_SESSION_FAILED, HPAE_STREAM_CLASS_TYPE_PLAY, sessionId, MOVE_SINGLE, sinkName);
        return;
    }

    if (sinkName.empty()) {
        AUDIO_ERR_LOG("[StartMove] session:%{public}u failed,sinkName is empty", sessionId);
        TriggerCallback(MOVE_SESSION_FAILED, HPAE_STREAM_CLASS_TYPE_PLAY, sessionId, MOVE_SINGLE, sinkName);
        return;
    }

    AUDIO_INFO_LOG("[StartMove] session: %{public}u,sink [%{public}s] --> [%{public}s]",
        sessionId, sinkInfo_.deviceName.c_str(), sinkName.c_str());
    std::shared_ptr<HpaeSinkInputNode> inputNode = sinkInputNodeMap_[sessionId];
    HpaeSessionState inputState = inputNode->GetState();
    if (inputState == HPAE_SESSION_STOPPING || inputState == HPAE_SESSION_PAUSING) {
        HpaeSessionState state = inputState == HPAE_SESSION_PAUSING ? HPAE_SESSION_PAUSED : HPAE_SESSION_STOPPED;
        IOperation operation = inputState == HPAE_SESSION_PAUSING ? OPERATION_PAUSED : OPERATION_STOPPED;
        SetSessionState(sessionId, state);
        inputNode->SetState(state);
        TriggerCallback(UPDATE_STATUS,
            HPAE_STREAM_CLASS_TYPE_PLAY,
            sessionId,
            sessionNodeMap_[sessionId].state,
            operation);
        // todo: do fade out
    }
    DeleteInputSession(sessionId);
    std::string name = sinkName;
    TriggerCallback(MOVE_SINK_INPUT, inputNode, name);
}

int32_t HpaeRendererManager::MoveStream(uint32_t sessionId, const std::string &sinkName)
{
    if (!IsInit()) {
        MoveStreamSync(sessionId, sinkName);
    } else {
        auto request = [this, sessionId, sinkName]() { MoveStreamSync(sessionId, sinkName); };
        SendRequest(request);
    }
    return SUCCESS;
}

int32_t HpaeRendererManager::Start(uint32_t sessionId)
{
    auto request = [this, sessionId]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Start");
        AUDIO_INFO_LOG("Start sessionId %{public}u, deviceName %{public}s", sessionId, sinkInfo_.deviceName.c_str());
        if (SafeGetMap(sinkInputNodeMap_, sessionId)) {
            sinkInputNodeMap_[sessionId]->SetState(HPAE_SESSION_RUNNING);
        }
        HandlePriPaPower(sessionId);
        ConnectInputSession(sessionId);
        SetSessionState(sessionId, HPAE_SESSION_RUNNING);
        SetSessionFade(sessionId, OPERATION_STARTED);
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::DisConnectInputSession(uint32_t sessionId)
{
    if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
        AUDIO_INFO_LOG("DisConnectInputSession sessionId %{public}u", sessionId);
        return SUCCESS;
    }
    HpaeProcessorType sceneType = GetProcessorType(sessionId);
    if (SafeGetMap(sceneClusterMap_, sceneType)) {
        DisConnectProcessCluster(sessionId, sceneType);
    }
    return SUCCESS;
}

void HpaeRendererManager::DisConnectProcessCluster(uint32_t sessionId, HpaeProcessorType sceneType)
{
    sceneClusterMap_[sceneType]->DisConnect(sinkInputNodeMap_[sessionId]);
    int32_t ret = sceneClusterMap_[sceneType]->AudioRendererStop(sinkInputNodeMap_[sessionId]->GetNodeInfo());
    if (ret != SUCCESS) {
        AUDIO_WARNING_LOG("update audio effect when stopping failed, ret = %{public}d", ret);
    }
    if (sceneClusterMap_[sceneType]->GetPreOutNum() == 0) {
        outputCluster_->DisConnect(sceneClusterMap_[sceneType]);
        // for collaboration
        if (sceneType == HPAE_SCENE_COLLABORATIVE && hpaeCoBufferNode_ != nullptr) {
            hpaeCoBufferNode_->DisConnect(sceneClusterMap_[sceneType]);
            TriggerCallback(DISCONNECT_CO_BUFFER_NODE, hpaeCoBufferNode_);
        }
        sceneClusterMap_[sceneType]->SetConnectedFlag(false);
    }
}

void HpaeRendererManager::SetSessionState(uint32_t sessionId, HpaeSessionState renderState)
{
    sessionNodeMap_[sessionId].state = renderState;
}

int32_t HpaeRendererManager::Pause(uint32_t sessionId)
{
    auto request = [this, sessionId]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Pause");
        AUDIO_INFO_LOG("Pause sessionId %{public}u deviceName %{public}s", sessionId, sinkInfo_.deviceName.c_str());
        CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
            "Pause not find sessionId %{public}u", sessionId);
        if (!SetSessionFade(sessionId, OPERATION_PAUSED)) {
            DisConnectInputSession(sessionId);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::Flush(uint32_t sessionId)
{
    auto request = [this, sessionId]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Flush");
        AUDIO_INFO_LOG("Flush sessionId %{public}u deviceName %{public}s", sessionId, sinkInfo_.deviceName.c_str());
        CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
            "Flush not find sessionId %{public}u", sessionId);
        // flush history buffer
        sinkInputNodeMap_[sessionId]->Flush();
        TriggerCallback(
            UPDATE_STATUS, HPAE_STREAM_CLASS_TYPE_PLAY, sessionId, sessionNodeMap_[sessionId].state, OPERATION_FLUSHED);
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::Drain(uint32_t sessionId)
{
    auto request = [this, sessionId]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Drain");
        AUDIO_INFO_LOG("Drain sessionId %{public}u deviceName %{public}s ", sessionId, sinkInfo_.deviceName.c_str());
        CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
            "Drain not find sessionId %{public}u", sessionId);
        sinkInputNodeMap_[sessionId]->Drain();
        if (sessionNodeMap_[sessionId].state != HPAE_SESSION_RUNNING) {
            AUDIO_INFO_LOG("TriggerCallback Drain sessionId %{public}u", sessionId);
            TriggerCallback(UPDATE_STATUS,
                HPAE_STREAM_CLASS_TYPE_PLAY,
                sessionId,
                sessionNodeMap_[sessionId].state,
                OPERATION_DRAINED);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::Stop(uint32_t sessionId)
{
    auto request = [this, sessionId]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Stop");
        AUDIO_INFO_LOG("Stop sessionId %{public}u deviceName %{public}s ", sessionId, sinkInfo_.deviceName.c_str());
        CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
            "Stop not find sessionId %{public}u", sessionId);
        if (!SetSessionFade(sessionId, OPERATION_STOPPED)) {
            DisConnectInputSession(sessionId);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::Release(uint32_t sessionId)
{
    Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::Release");
    CHECK_AND_RETURN_RET_LOG(SafeGetMap(sinkInputNodeMap_, sessionId), ERROR,
        "Release not find sessionId %{public}u", sessionId);
    return DestroyStream(sessionId);
}

bool HpaeRendererManager::CheckIsStreamRunning()
{
    bool isRunning = false;
    for (const auto& it : sessionNodeMap_) {
        if (it.second.state == HPAE_SESSION_RUNNING) {
            isRunning = true;
            break;
        }
    }
    return isRunning;
}

int32_t HpaeRendererManager::SuspendStreamManager(bool isSuspend)
{
    Trace trace("HpaeRendererManager::SuspendStreamManager: " + std::to_string(isSuspend));
    auto request = [this, isSuspend]() {
        if (isSuspend_ == isSuspend) {
            return;
        }
        isSuspend_ = isSuspend;
        if (isSuspend_) {
            if (outputCluster_ != nullptr) {
                outputCluster_->Stop();
            }
        } else if (outputCluster_ != nullptr && outputCluster_->GetState() != STREAM_MANAGER_RUNNING &&
            CheckIsStreamRunning()) {
            outputCluster_->Start();
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::SetMute(bool isMute)
{
    // to do check pulseaudio
    auto request = [this, isMute]() {
        if (isMute_ != isMute) {
            isMute_ = isMute;  // todo: fadein and fadeout and mute feature
        }
    };
    SendRequest(request);
    return SUCCESS;
}

void HpaeRendererManager::HandleMsg()
{
    hpaeNoLockQueue_.HandleRequests();
}

int32_t HpaeRendererManager::ReloadRenderManager(const HpaeSinkInfo &sinkInfo)
{
    hpaeSignalProcessThread_ = std::make_unique<HpaeSignalProcessThread>();
    auto request = [this, sinkInfo]() {
        AUDIO_INFO_LOG("ReloadRenderManager deviceName %{public}s", sinkInfo.deviceName.c_str());
        sinkInfo_ = sinkInfo;
        InitManager();

        for (const auto &it : sinkInputNodeMap_) {
            if (it.second && it.second->GetState() == HPAE_SESSION_RUNNING) {
                AUDIO_INFO_LOG("connect node :%{public}u to sink:%{public}s", it.first, sinkInfo_.deviceClass.c_str());
                ConnectInputSession(it.first);
            }
        }
    };
    SendRequest(request, true);
    hpaeSignalProcessThread_->ActivateThread(shared_from_this());
    return SUCCESS;
}

void HpaeRendererManager::InitManager()
{
    AUDIO_INFO_LOG("init devicename:%{public}s", sinkInfo_.deviceName.c_str());
    HpaeNodeInfo nodeInfo;
    nodeInfo.channels = sinkInfo_.channels;
    nodeInfo.format = sinkInfo_.format;
    nodeInfo.frameLen = sinkInfo_.frameLen;
    nodeInfo.nodeId = 0;
    nodeInfo.samplingRate = sinkInfo_.samplingRate;
    nodeInfo.sceneType = HPAE_SCENE_EFFECT_OUT;
    nodeInfo.deviceNetId = sinkInfo_.deviceNetId;
    nodeInfo.deviceClass = sinkInfo_.deviceClass;
    nodeInfo.statusCallback = weak_from_this();
    if (sinkInfo_.lib == "libmodule-split-stream-sink.z.so") {
        outputCluster_ = std::make_unique<HpaeRemoteOutputCluster>(nodeInfo);
    } else {
        outputCluster_ = std::make_unique<HpaeOutputCluster>(nodeInfo);
    }
    outputCluster_->SetTimeoutStopThd(sinkInfo_.suspendTime);
    int32_t ret = outputCluster_->GetInstance(sinkInfo_.deviceClass, sinkInfo_.deviceNetId);
    IAudioSinkAttr attr;
    attr.adapterName = sinkInfo_.adapterName.c_str();
    attr.sampleRate = sinkInfo_.samplingRate;
    attr.channel = sinkInfo_.channels;
    attr.format = sinkInfo_.format;
    attr.channelLayout = sinkInfo_.channelLayout;
    attr.deviceType = sinkInfo_.deviceType;
    attr.volume = sinkInfo_.volume;
    attr.openMicSpeaker = sinkInfo_.openMicSpeaker;
    attr.deviceNetworkId = sinkInfo_.deviceNetId.c_str();
    attr.filePath = sinkInfo_.filePath.c_str();
    attr.aux = sinkInfo_.splitMode.c_str();
    if (!sceneClusterMap_.count(HPAE_SCENE_EFFECT_NONE)) {
        HpaeNodeInfo defaultNodeInfo;
        defaultNodeInfo.frameLen = (uint32_t)DEFAULT_EFFECT_FRAME_LEN;
        defaultNodeInfo.samplingRate = (AudioSamplingRate)DEFAULT_EFFECT_RATE;
        defaultNodeInfo.format = AudioSampleFormat::INVALID_WIDTH;
        defaultNodeInfo.channels = STEREO;
        defaultNodeInfo.channelLayout = AudioChannelLayout::CH_LAYOUT_STEREO;
        defaultNodeInfo.streamType = STREAM_DEFAULT;
        defaultNodeInfo.sceneType = HPAE_SCENE_EFFECT_NONE;
        defaultNodeInfo.deviceNetId = sinkInfo_.deviceNetId;
        defaultNodeInfo.deviceClass = sinkInfo_.deviceClass;
        defaultNodeInfo.statusCallback = weak_from_this();
        sceneClusterMap_[HPAE_SCENE_EFFECT_NONE] = std::make_shared<HpaeProcessCluster>(defaultNodeInfo, sinkInfo_);
        sceneTypeToProcessClusterCountMap_[HPAE_SCENE_EFFECT_NONE] = 1;
    }

    ret = outputCluster_->Init(attr);
    isInit_.store(ret == SUCCESS);
    TriggerCallback(INIT_DEVICE_RESULT, sinkInfo_.deviceName, ret);
}

void HpaeRendererManager::CreateOutputClusterNodeInfo(HpaeNodeInfo &nodeInfo)
{
    nodeInfo.channels = sinkInfo_.channels;
    nodeInfo.format = sinkInfo_.format;
    nodeInfo.frameLen = sinkInfo_.frameLen;
    nodeInfo.nodeId = 0;
    nodeInfo.samplingRate = sinkInfo_.samplingRate;
    nodeInfo.sceneType = HPAE_SCENE_EFFECT_OUT;
    nodeInfo.deviceNetId = sinkInfo_.deviceNetId;
    nodeInfo.deviceClass = sinkInfo_.deviceClass;
    nodeInfo.statusCallback = weak_from_this();
    return;
}

int32_t HpaeRendererManager::Init()
{
    hpaeSignalProcessThread_ = std::make_unique<HpaeSignalProcessThread>();
    auto request = [this] {
        Trace trace("HpaeRendererManager::Init");
        InitManager();
    };
    SendRequest(request, true);
    hpaeSignalProcessThread_->ActivateThread(shared_from_this());
    return SUCCESS;
}

bool HpaeRendererManager::DeactivateThread()
{
    if (hpaeSignalProcessThread_ != nullptr) {
        hpaeSignalProcessThread_->DeactivateThread();
        hpaeSignalProcessThread_ = nullptr;
    }
    hpaeNoLockQueue_.HandleRequests();
    return true;
}

int32_t HpaeRendererManager::DeInit(bool isMoveDefault)
{
    Trace trace("HpaeRendererManager::DeInit");
    if (hpaeSignalProcessThread_ != nullptr) {
        hpaeSignalProcessThread_->DeactivateThread();
        hpaeSignalProcessThread_ = nullptr;
    }
    hpaeNoLockQueue_.HandleRequests();
    if (isMoveDefault) {
        std::string sinkName = "";
        std::vector<uint32_t> ids;
        AUDIO_INFO_LOG("move all sink to default sink");
        MoveAllStreamToNewSink(sinkName, ids, MOVE_ALL);
    }
    outputCluster_->Stop();
    int32_t ret = outputCluster_->DeInit();
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "RenderSinkDeInit error, ret %{public}d.", ret);
    for (const auto &item : sceneClusterMap_) {
        item.second->SetConnectedFlag(false);
    }
    outputCluster_->ResetAll();
    isInit_.store(false);
    return SUCCESS;
}

int32_t HpaeRendererManager::StartRenderSink()
{
    return SUCCESS;
}

int32_t HpaeRendererManager::SetClientVolume(uint32_t sessionId, float volume)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::SetRate(uint32_t sessionId, int32_t rate)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::SetAudioEffectMode(uint32_t sessionId, int32_t effectMode)
{
    if (effectMode < EFFECT_NONE || effectMode > EFFECT_DEFAULT) {
        return ERR_INVALID_OPERATION;
    }
    auto request = [this, sessionId, effectMode]() {
        if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
            AUDIO_WARNING_LOG("miss corresponding sinkInputNode for sessionId %{public}d", sessionId);
            return ;
        }
        HpaeNodeInfo &nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
        if (nodeInfo.effectInfo.effectMode != static_cast<AudioEffectMode>(effectMode)) {
            nodeInfo.effectInfo.effectMode = static_cast<AudioEffectMode>(effectMode);
            size_t sinkInputNodeConnectNum = sinkInputNodeMap_[sessionId]->GetOutputPort()->GetInputNum();
            if (sinkInputNodeConnectNum != 0) {
                AUDIO_INFO_LOG("UpdateProcessClusterConnection because effectMode to be %{public}d", effectMode);
                UpdateProcessClusterConnection(sessionId, effectMode);
            } else {
                AUDIO_INFO_LOG("no need to ProcessClusterConnection, sinkInputNodeConnectNum is %{public}zu",
                    sinkInputNodeConnectNum);
            }
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::GetAudioEffectMode(uint32_t sessionId, int32_t &effectMode)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::SetPrivacyType(uint32_t sessionId, int32_t privacyType)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::GetPrivacyType(uint32_t sessionId, int32_t &privacyType)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::RegisterWriteCallback(uint32_t sessionId, const std::weak_ptr<IStreamCallback> &callback)
{
    auto request = [this, sessionId, callback]() {
        AUDIO_INFO_LOG("RegisterWriteCallback sessionId %{public}u", sessionId);
        if (SafeGetMap(sinkInputNodeMap_, sessionId)) {
            sinkInputNodeMap_[sessionId]->RegisterWriteCallback(callback);
        }
    };
    hpaeNoLockQueue_.PushRequest(request);
    return SUCCESS;
}

void HpaeRendererManager::Process()
{
    Trace trace("HpaeRendererManager::Process");
    if (outputCluster_ != nullptr && IsRunning()) {
        UpdateAppsUid();
        outputCluster_->DoProcess();
    }
}

void HpaeRendererManager::UpdateAppsUid()
{
    appsUid_.clear();
    for (const auto &sinkInputNodePair : sinkInputNodeMap_) {
        if (sinkInputNodePair.second->GetState() == HPAE_SESSION_RUNNING) {
            appsUid_.emplace_back(sinkInputNodePair.second->GetAppUid());
        }
    }
    outputCluster_->UpdateAppsUid(appsUid_);
}

size_t HpaeRendererManager::GetWritableSize(uint32_t sessionId)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::UpdateSpatializationState(
    uint32_t sessionId, bool spatializationEnabled, bool headTrackingEnabled)
{
    return SUCCESS;
}

int32_t HpaeRendererManager::UpdateMaxLength(uint32_t sessionId, uint32_t maxLength)
{
    return SUCCESS;
}

std::vector<SinkInput> HpaeRendererManager::GetAllSinkInputsInfo()
{
    return {};
}

int32_t HpaeRendererManager::GetSinkInputInfo(uint32_t sessionId, HpaeSinkInputInfo &sinkInputInfo)
{
    if (!SafeGetMap(sinkInputNodeMap_, sessionId)) {
        return ERR_INVALID_OPERATION;
    }
    sinkInputInfo.nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
    sinkInputInfo.rendererSessionInfo = sessionNodeMap_[sessionId];
    return SUCCESS;
}

HpaeSinkInfo HpaeRendererManager::GetSinkInfo()
{
    return sinkInfo_;
}

bool HpaeRendererManager::IsInit()
{
    return isInit_.load();
}

bool HpaeRendererManager::IsMsgProcessing()
{
    return !hpaeNoLockQueue_.IsFinishProcess();
}

bool HpaeRendererManager::IsRunning(void)
{
    if (outputCluster_ != nullptr && hpaeSignalProcessThread_ != nullptr) {
        return outputCluster_->GetState() == STREAM_MANAGER_RUNNING && hpaeSignalProcessThread_->IsRunning();
    } else {
        return false;
    }
}

void HpaeRendererManager::SendRequest(Request &&request, bool isInit)
{
    AUDIO_DEBUG_LOG("HpaeRendererManager::isInit is %{public}s", isInit ? "true" : "false");
    CHECK_AND_RETURN_LOG(isInit || IsInit(), "HpaeRendererManager not init");
    hpaeNoLockQueue_.PushRequest(std::move(request));
    CHECK_AND_RETURN_LOG(hpaeSignalProcessThread_, "hpaeSignalProcessThread_  renderer is nullptr");
    hpaeSignalProcessThread_->Notify();
}

void HpaeRendererManager::OnNodeStatusUpdate(uint32_t sessionId, IOperation operation)
{
    TriggerCallback(UPDATE_STATUS, HPAE_STREAM_CLASS_TYPE_PLAY, sessionId, sessionNodeMap_[sessionId].state, operation);
}

void HpaeRendererManager::OnFadeDone(uint32_t sessionId, IOperation operation)
{
    auto request = [this, sessionId, operation]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::OnFadeDone: " + std::to_string(operation));
        AUDIO_INFO_LOG("Fade done, call back at RendererManager");
        DisConnectInputSession(sessionId);
        HpaeSessionState state = operation == OPERATION_STOPPED ? HPAE_SESSION_STOPPED : HPAE_SESSION_PAUSED;
        SetSessionState(sessionId, state);
        if (SafeGetMap(sinkInputNodeMap_, sessionId)) {
            sinkInputNodeMap_[sessionId]->SetState(state);
        }
        TriggerCallback(
            UPDATE_STATUS, HPAE_STREAM_CLASS_TYPE_PLAY, sessionId, sessionNodeMap_[sessionId].state, operation);
    };
    SendRequest(request);
}

int32_t HpaeRendererManager::RegisterReadCallback(uint32_t sessionId,
    const std::weak_ptr<ICapturerStreamCallback> &callback)
{
    return SUCCESS;
}

void HpaeRendererManager::OnRequestLatency(uint32_t sessionId, uint64_t &latency)
{
    // todo: add processLatency
    return;
}

void HpaeRendererManager::OnNotifyQueue()
{
    hpaeSignalProcessThread_->Notify();
}

void HpaeRendererManager::UpdateProcessClusterConnection(uint32_t sessionId, int32_t effectMode)
{
    Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::UpdateProcessClusterConnection" +
        "effectMode[" + std::to_string(effectMode) + "]");
    HpaeProcessorType sceneType = sinkInputNodeMap_[sessionId]->GetSceneType();
    if (!SafeGetMap(sceneClusterMap_, sceneType)) {
        AUDIO_WARNING_LOG("miss corresponding process cluster for scene type %{public}d", sceneType);
        return;
    }
    if (isSplitProcessorType(sceneType) || sessionNodeMap_[sessionId].bypass) {
        AUDIO_INFO_LOG("no need to update the sceneType %{public}d", sceneType);
        return;
    }

    if (effectMode == EFFECT_NONE) {
        DisConnectProcessCluster(sessionId, sceneType);
        ConnectProcessCluster(sessionId, HPAE_SCENE_EFFECT_NONE);
    } else {
        DisConnectProcessCluster(sessionId, HPAE_SCENE_EFFECT_NONE);
        ConnectProcessCluster(sessionId, sceneType);
    }
}

std::string HpaeRendererManager::GetThreadName()
{
    return sinkInfo_.deviceName;
}

bool HpaeRendererManager::SetSessionFade(uint32_t sessionId, IOperation operation)
{
    CHECK_AND_RETURN_RET_LOG(SafeGetMap(sinkInputNodeMap_, sessionId), false,
        "can not get input node of session %{public}u", sessionId);
    HpaeProcessorType sceneType = GetProcessorType(sessionId);
    AUDIO_INFO_LOG("sessionId is %{public}d, sceneType is %{public}d", sessionId, sceneType);
    std::shared_ptr<HpaeGainNode> sessionGainNode = nullptr;
    if (SafeGetMap(sceneClusterMap_, sceneType)) {
        sessionGainNode = sceneClusterMap_[sceneType]->GetGainNodeById(sessionId);
    }
    if (sessionGainNode == nullptr) {
        AUDIO_WARNING_LOG("session %{public}d do not have gain node!", sessionId);
        if (operation != OPERATION_STARTED) {
            HpaeSessionState state = operation == OPERATION_STOPPED ? HPAE_SESSION_STOPPED : HPAE_SESSION_PAUSED;
            SetSessionState(sessionId, state);
            sinkInputNodeMap_[sessionId]->SetState(state);
            TriggerCallback(UPDATE_STATUS,
                HPAE_STREAM_CLASS_TYPE_PLAY,
                sessionId,
                sessionNodeMap_[sessionId].state,
                operation);
        }
        return false;
    }
    AUDIO_INFO_LOG("get gain node of session %{public}d.", sessionId);
    if (operation != OPERATION_STARTED) {
        HpaeSessionState state = operation == OPERATION_STOPPED ? HPAE_SESSION_STOPPING : HPAE_SESSION_PAUSING;
        SetSessionState(sessionId, state);
        sinkInputNodeMap_[sessionId]->SetState(state);
    }
    sessionGainNode->SetFadeState(operation);
    return true;
}

void HpaeRendererManager::DumpSinkInfo()
{
    auto request = [this]() {
        AUDIO_INFO_LOG("DumpSinkInfo deviceName %{public}s", sinkInfo_.deviceName.c_str());
        UploadDumpSinkInfo(sinkInfo_.deviceName);
    };
    SendRequest(request);
}

int32_t HpaeRendererManager::SetOffloadPolicy(uint32_t sessionId, int32_t state)
{
    auto request = [this, sessionId, state]() {
        Trace trace("[" + std::to_string(sessionId) + "]HpaeRendererManager::SetOffloadPolicy");
        AUDIO_INFO_LOG("SetOffloadPolicy sessionId %{public}u, deviceName %{public}s, state %{public}d", sessionId,
            sinkInfo_.deviceName.c_str(), state);
        if (SafeGetMap(sinkInputNodeMap_, sessionId)) {
            sinkInputNodeMap_[sessionId]->SetOffloadEnabled(state != OFFLOAD_DEFAULT);
        } else {
            AUDIO_ERR_LOG("not find sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::UpdateCollaborativeState(bool isCollaborationEnabled)
{
    auto request = [this, isCollaborationEnabled]() {
        if (isCollaborationEnabled_ == isCollaborationEnabled) {
            AUDIO_INFO_LOG("collaboration state not changed, isCollaborationEnabled_ %{public}d",
                isCollaborationEnabled_);
            return;
        }
        AUDIO_INFO_LOG("collaborativeState change from %{public}d to %{public}d",
            isCollaborationEnabled_, isCollaborationEnabled);
        isCollaborationEnabled_ = isCollaborationEnabled;
        if (isCollaborationEnabled_) {
            // for collaboration enabled
            EnableCollaboration();
        } else {
            // for collaboration disabled
            DisableCollaboration();
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::HandlePriPaPower(uint32_t sessionId)
{
    if (!SafeGetMap(sinkInputNodeMap_, sessionId) || sinkInfo_.deviceClass != "primary") {
        return ERR_INVALID_OPERATION;
    }
    auto &nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
    struct VolumeValues volumes;
    auto audioVolume = AudioVolume::GetInstance();
    float curVolume = audioVolume->GetVolume(sessionId, nodeInfo.streamType, sinkInfo_.deviceClass, &volumes);
    auto isZeroVolume = audioVolume->IsSameVolume(0.0f, curVolume);
    AUDIO_INFO_LOG("session %{public}u, stream %{public}d, is zero volume %{public}d",
        sessionId, nodeInfo.streamType, isZeroVolume);
    if (!isZeroVolume) {
        return outputCluster_->SetPriPaPower();
    }
    return SUCCESS;
}

std::string HpaeRendererManager::GetDeviceHDFDumpInfo()
{
    std::string config;
    TransDeviceInfoToString(sinkInfo_, config);
    return config;
}

int32_t HpaeRendererManager::ConnectCoBufferNode(const std::shared_ptr<HpaeCoBufferNode> &coBufferNode)
{
    auto request = [this, coBufferNode]() {
        CHECK_AND_RETURN_LOG((outputCluster_ != nullptr) && (coBufferNode != nullptr),
            "outputCluster or coBufferNode is nullptr");
        if (!coBufferNode->IsOutputClusterConnected()) {
            outputCluster_->Connect(coBufferNode);
            coBufferNode->SetOutputClusterConnected(true);
        }
        if (outputCluster_->GetState() != STREAM_MANAGER_RUNNING && !isSuspend_) {
            outputCluster_->Start();
        }
    };
    SendRequest(request);
    return SUCCESS;
}

int32_t HpaeRendererManager::DisConnectCoBufferNode(const std::shared_ptr<HpaeCoBufferNode> &coBufferNode)
{
    auto request = [this, coBufferNode]() {
        CHECK_AND_RETURN_LOG((outputCluster_ != nullptr) && (coBufferNode != nullptr),
            "outputCluster or coBufferNode is nullptr");
        if (coBufferNode->IsOutputClusterConnected()) {
            outputCluster_->DisConnect(coBufferNode);
            coBufferNode->SetOutputClusterConnected(false);
        }
    };
    SendRequest(request);
    return SUCCESS;
}

void HpaeRendererManager::ReConnectNodeForCollaboration(uint32_t sessionId)
{
    // todo fade out
    CHECK_AND_RETURN_LOG(SafeGetMap(sinkInputNodeMap_, sessionId),
        "sinkInputNodeMap_ not find sessionId %{public}u", sessionId);
    HpaeNodeInfo nodeInfo = sinkInputNodeMap_[sessionId]->GetNodeInfo();
    HpaeProcessorType sceneType = GetProcessorType(sessionId);
    if (SafeGetMap(sceneClusterMap_, sceneType)) {
        DeleteProcessCluster(nodeInfo, sceneType, sessionId);
    }
    AUDIO_INFO_LOG("AddSingleNodeToSink sessionId %{public}u", sessionId);
    AddSingleNodeToSink(sinkInputNodeMap_[sessionId]);
}

void HpaeRendererManager::EnableCollaboration()
{
    if (hpaeCoBufferNode_ == nullptr) {
        hpaeCoBufferNode_ = std::make_shared<HpaeCoBufferNode>();
    }

    for (auto& [key, node] : sinkInputNodeMap_) {
        HpaeNodeInfo nodeInfo = node->GetNodeInfo();
        if (nodeInfo.effectInfo.effectScene == SCENE_MUSIC || nodeInfo.effectInfo.effectScene == SCENE_MOVIE) {
            ReConnectNodeForCollaboration(key);
        }
    }
}

void HpaeRendererManager::DisableCollaboration()
{
    for (auto& [key, node] : sinkInputNodeMap_) {
        HpaeNodeInfo nodeInfo = node->GetNodeInfo();
        if (nodeInfo.effectInfo.effectScene == SCENE_COLLABORATIVE) {
            ReConnectNodeForCollaboration(key);
        }
    }
    hpaeCoBufferNode_.reset();
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS