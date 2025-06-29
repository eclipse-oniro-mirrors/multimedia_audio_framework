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
#define LOG_TAG "AudioPipeSelector"
#endif

#include "audio_pipe_selector.h"
#include "audio_stream_collector.h"
#include "audio_stream_info.h"
#include "audio_definition_adapter_info.h"
#include <algorithm>

namespace OHOS {
namespace AudioStandard {

static std::map<int, AudioPipeType> flagPipeTypeMap_ = {
    {AUDIO_OUTPUT_FLAG_NORMAL, PIPE_TYPE_NORMAL_OUT},
    {AUDIO_INPUT_FLAG_NORMAL, PIPE_TYPE_NORMAL_IN},
    {AUDIO_OUTPUT_FLAG_FAST, PIPE_TYPE_NORMAL_OUT},
    {AUDIO_INPUT_FLAG_FAST, PIPE_TYPE_NORMAL_IN},
    {AUDIO_OUTPUT_FLAG_LOWPOWER, PIPE_TYPE_OFFLOAD},
    {AUDIO_OUTPUT_FLAG_MULTICHANNEL, PIPE_TYPE_MULTICHANNEL},
    {AUDIO_OUTPUT_FLAG_DIRECT, PIPE_TYPE_DIRECT_OUT},
};

AudioPipeSelector::AudioPipeSelector() : configManager_(AudioPolicyConfigManager::GetInstance())
{
}

std::shared_ptr<AudioPipeSelector> AudioPipeSelector::GetPipeSelector()
{
    static std::shared_ptr<AudioPipeSelector> instance = std::make_shared<AudioPipeSelector>();
    return instance;
}

std::vector<std::shared_ptr<AudioPipeInfo>> AudioPipeSelector::FetchPipeAndExecute(
    std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfoList = AudioPipeManager::GetPipeManager()->GetPipeList();

    if (streamDesc->routeFlag_ == AUDIO_FLAG_NONE) {
        AUDIO_INFO_LOG("Need update route flag");
        ScanPipeListForStreamDesc(pipeInfoList, streamDesc);
    }
    AUDIO_INFO_LOG("Original Pipelist size: %{public}zu, stream routeFlag: 0x%{public}x to fetch",
        pipeInfoList.size(), streamDesc->routeFlag_);

    std::vector<std::shared_ptr<AudioPipeInfo>> selectedPipeInfoList {};
    for (auto &curPipeInfo : pipeInfoList) {
        if (curPipeInfo->pipeRole_ == static_cast<AudioPipeRole>(streamDesc->audioMode_)) {
            selectedPipeInfoList.push_back(curPipeInfo);
        }
    }

    streamDesc->streamAction_ = AUDIO_STREAM_ACTION_NEW;
    std::shared_ptr<PipeStreamPropInfo> streamPropInfo = std::make_shared<PipeStreamPropInfo>();
    configManager_.GetStreamPropInfo(streamDesc, streamPropInfo);
    std::shared_ptr<AdapterPipeInfo> pipeInfoPtr = streamPropInfo->pipeInfo_.lock();
    if (pipeInfoPtr == nullptr) {
        AUDIO_ERR_LOG("Pipe info is null");
        return selectedPipeInfoList;
    }
    for (auto &pipeInfo : selectedPipeInfoList) {
        std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = pipeInfoPtr->adapterInfo_.lock();
        if (adapterInfoPtr == nullptr) {
            AUDIO_ERR_LOG("Adapter info is null");
            continue;
        }
        AUDIO_INFO_LOG("[Cur][XML]: {adapterName}[%{public}s][%{public}s], {routeFlag}[%{public}x][%{public}x]",
            pipeInfo->adapterName_.c_str(), adapterInfoPtr->adapterName.c_str(),
            pipeInfo->routeFlag_, streamDesc->routeFlag_);

        if (pipeInfo->adapterName_ == adapterInfoPtr->adapterName &&
            pipeInfo->routeFlag_ == streamDesc->routeFlag_) {
            pipeInfo->streamDescriptors_.push_back(streamDesc);
            pipeInfo->streamDescMap_[streamDesc->sessionId_] = streamDesc;
            pipeInfo->pipeAction_ = PIPE_ACTION_UPDATE;
            AUDIO_INFO_LOG("[PipeFetchInfo] use existing Pipe %{public}s for stream %{public}u",
                pipeInfo->ToString().c_str(), streamDesc->sessionId_);
            return selectedPipeInfoList;
        }
    }
    AudioPipeInfo info = {};
    ConvertStreamDescToPipeInfo(streamDesc, streamPropInfo, info);
    info.pipeAction_ = PIPE_ACTION_NEW;
    selectedPipeInfoList.push_back(std::make_shared<AudioPipeInfo>(info));
    AUDIO_INFO_LOG("[PipeFetchInfo] use new Pipe %{public}s for stream %{public}u",
        info.ToString().c_str(), streamDesc->sessionId_);
    return selectedPipeInfoList;
}

std::vector<std::shared_ptr<AudioPipeInfo>> AudioPipeSelector::FetchPipesAndExecute(
    std::vector<std::shared_ptr<AudioStreamDescriptor>> &streamDescs)
{
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfoList = AudioPipeManager::GetPipeManager()->GetPipeList();

    std::vector<std::shared_ptr<AudioPipeInfo>> selectedPipeInfoList{};
    if (streamDescs.size() == 0) {
        return selectedPipeInfoList;
    }
    for (auto &curPipeInfo : pipeInfoList) {
        if (curPipeInfo->pipeRole_ == static_cast<AudioPipeRole>(streamDescs[0]->audioMode_)) {
            selectedPipeInfoList.push_back(curPipeInfo);
        }
    }

    AUDIO_INFO_LOG("Original Pipelist size: %{public}zu for role %{public}d to fetch",
        selectedPipeInfoList.size(), streamDescs[0]->audioMode_);

    // Record current pipe--stream info for later use (Judge stream action)
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> streamDescToPipeInfo;
    for (auto &pipeInfo : selectedPipeInfoList) {
        pipeInfo->pipeAction_ = PIPE_ACTION_DEFAULT;
        for (auto &streamDesc : pipeInfo->streamDescriptors_) {
            streamDescToPipeInfo[streamDesc->sessionId_] = pipeInfo;
        }
    }

    // deep copy to new pipeList
    std::vector<std::shared_ptr<AudioPipeInfo>> newPipeInfoList;
    for (auto &pipeInfo : selectedPipeInfoList) {
        std::shared_ptr<AudioPipeInfo> temp = std::make_shared<AudioPipeInfo>(*pipeInfo);
        temp->streamDescriptors_.clear();
        temp->streamDescMap_.clear();
        newPipeInfoList.push_back(temp);
    }

    SortStreamDescsByStartTime(streamDescs);
    // traversal each streamDesc
    for (auto &streamDesc : streamDescs) {
        std::string adapterName = GetAdapterNameByStreamDesc(streamDesc);
        ScanPipeListForStreamDesc(newPipeInfoList, streamDesc); // Get route flag and apply concurrency
        bool isFindPipeInfo = IsPipeExist(newPipeInfoList, adapterName, streamDesc, streamDescToPipeInfo);
        if (!isFindPipeInfo) {
            HandlePipeNotExist(newPipeInfoList, streamDesc, streamDescToPipeInfo);
        }
    }
    // Check is pipe update
    for (auto &pipeInfo : selectedPipeInfoList) {
        if (pipeInfo->streamDescriptors_.size() == 0) {
            pipeInfo->pipeAction_ = PIPE_ACTION_DEFAULT;
        }
    }
    return newPipeInfoList;
}

void AudioPipeSelector::HandlePipeNotExist(std::vector<std::shared_ptr<AudioPipeInfo>> &newPipeInfoList,
    std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> &streamDescToPipeInfo)
{
    AudioPipeInfo pipeInfo = {};
    std::shared_ptr<PipeStreamPropInfo> streamPropInfo = std::make_shared<PipeStreamPropInfo>();
    configManager_.GetStreamPropInfo(streamDesc, streamPropInfo);
    ConvertStreamDescToPipeInfo(streamDesc, streamPropInfo, pipeInfo);
    pipeInfo.pipeAction_ = PIPE_ACTION_NEW;
    std::shared_ptr<AudioPipeInfo> tempPipeInfo = std::make_shared<AudioPipeInfo>(pipeInfo);
    newPipeInfoList.push_back(tempPipeInfo);
    streamDesc->streamAction_ = JudgeStreamAction(tempPipeInfo, streamDescToPipeInfo[streamDesc->sessionId_]);
    AUDIO_INFO_LOG("[PipeFetchInfo] use new Pipe %{public}s for stream %{public}u with action %{public}d",
        tempPipeInfo->ToString().c_str(), streamDesc->sessionId_, streamDesc->streamAction_);
}

bool AudioPipeSelector::IsPipeExist(std::vector<std::shared_ptr<AudioPipeInfo>> &newPipeInfoList,
    std::string &adapterName, std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    std::map<uint32_t, std::shared_ptr<AudioPipeInfo>> &streamDescToPipeInfo)
{
    bool isFindPipeInfo = false;
    for (auto &newPipeInfo : newPipeInfoList) {
        if (newPipeInfo->adapterName_ != adapterName || newPipeInfo->routeFlag_ != streamDesc->routeFlag_) {
            continue;
        }
        newPipeInfo->streamDescriptors_.push_back(streamDesc);
        if (streamDescToPipeInfo.find(streamDesc->sessionId_) == streamDescToPipeInfo.end()) {
            AUDIO_INFO_LOG("Cannot find session: %{public}u", streamDesc->sessionId_);
        } else {
            streamDesc->streamAction_ =
                JudgeStreamAction(newPipeInfo, streamDescToPipeInfo[streamDesc->sessionId_]);
        }
        newPipeInfo->streamDescMap_[streamDesc->sessionId_] = streamDesc;
        if (newPipeInfo->pipeAction_ != PIPE_ACTION_NEW) {
            newPipeInfo->pipeAction_ = PIPE_ACTION_UPDATE;
        }
        AUDIO_INFO_LOG("[PipeFetchInfo] use existing Pipe %{public}s for stream %{public}u with action %{public}d",
            newPipeInfo->ToString().c_str(), streamDesc->sessionId_, streamDesc->streamAction_);
        isFindPipeInfo = true;
        break;
    }
    return isFindPipeInfo;
}

void AudioPipeSelector::ScanPipeListForStreamDesc(std::vector<std::shared_ptr<AudioPipeInfo>> &pipeInfoList,
    std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    CHECK_AND_RETURN_LOG(streamDesc != nullptr, "streamDesc is nullptr");
    streamDesc->routeFlag_ = GetRouteFlagByStreamDesc(streamDesc);
    AUDIO_INFO_LOG("Route flag: %{public}u", streamDesc->routeFlag_);

    for (auto &pipeInfo : pipeInfoList) {
        bool isUpdate = false;
        for (auto &streamDescInPipe : pipeInfo->streamDescriptors_) {
            isUpdate = ProcessConcurrency(streamDescInPipe, streamDesc);
            AUDIO_INFO_LOG("isUpdate: %{public}d, action: %{public}d", isUpdate, streamDescInPipe->streamAction_);
        }
        pipeInfo->pipeAction_ = isUpdate ? PIPE_ACTION_UPDATE : pipeInfo->pipeAction_;
    }
    AUDIO_INFO_LOG("Route flag after concurrency: %{public}u", streamDesc->routeFlag_);
}

AudioPipeType AudioPipeSelector::GetPipeType(uint32_t flag, AudioMode audioMode)
{
    AUDIO_INFO_LOG("Route flag: %{public}u", flag);
    if (audioMode == AUDIO_MODE_PLAYBACK) {
        if (flag & AUDIO_OUTPUT_FLAG_FAST) {
            if (flag & AUDIO_OUTPUT_FLAG_VOIP) {
                return PIPE_TYPE_CALL_OUT;
            } else {
                return PIPE_TYPE_LOWLATENCY_OUT;
            }
        } else if (flag & AUDIO_OUTPUT_FLAG_DIRECT) {
            if (flag & AUDIO_OUTPUT_FLAG_VOIP) {
                return PIPE_TYPE_CALL_OUT;
            } else {
                return PIPE_TYPE_DIRECT_OUT;
            }
        } else if (flag & AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD) {
            return PIPE_TYPE_OFFLOAD;
        } else if (flag & AUDIO_OUTPUT_FLAG_MULTICHANNEL) {
            return PIPE_TYPE_MULTICHANNEL;
        } else {
            return PIPE_TYPE_NORMAL_OUT;
        }
    } else {
        if (flag & AUDIO_INPUT_FLAG_FAST) {
            if (flag & AUDIO_INPUT_FLAG_VOIP) {
                return PIPE_TYPE_CALL_IN;
            } else {
                return PIPE_TYPE_LOWLATENCY_IN;
            }
        } else {
            return PIPE_TYPE_NORMAL_IN;
        }
    }
}

void AudioPipeSelector::IncomingConcurrency(std::shared_ptr<AudioStreamDescriptor> stream,
    std::shared_ptr<AudioStreamDescriptor> cmpStream)
{
    // normal, mmap or voipmmap can't concurrency, if concede existing must concede incoming
    if (cmpStream->audioMode_ == AUDIO_MODE_RECORD && stream->audioMode_ == AUDIO_MODE_RECORD) {
        cmpStream->routeFlag_ = AUDIO_INPUT_FLAG_NORMAL;
        AUDIO_INFO_LOG("capture in: %{public}u  old: %{public}u", cmpStream->sessionId_, stream->sessionId_);
    }
}

bool AudioPipeSelector::ProcessConcurrency(std::shared_ptr<AudioStreamDescriptor> stream,
    std::shared_ptr<AudioStreamDescriptor> cmpStream)
{
    bool isUpdate = false;
    std::map<std::pair<AudioPipeType, AudioPipeType>, ConcurrencyAction> ruleMap =
        AudioStreamCollector::GetAudioStreamCollector().GetConcurrencyMap();
    ConcurrencyAction action = ruleMap[std::make_pair(GetPipeType(stream->routeFlag_, stream->audioMode_),
        GetPipeType(cmpStream->routeFlag_, cmpStream->audioMode_))];
    uint32_t newFlag;
    AUDIO_INFO_LOG("Action: %{public}u", action);
    switch (action) {
        case PLAY_BOTH:
            stream->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
            break;
        case CONCEDE_INCOMING:
            stream->streamAction_ = AUDIO_STREAM_ACTION_DEFAULT;
            cmpStream->routeFlag_ = cmpStream->audioMode_ == AUDIO_MODE_PLAYBACK ?
                AUDIO_OUTPUT_FLAG_NORMAL : AUDIO_INPUT_FLAG_NORMAL;
            break;
        case CONCEDE_EXISTING:
            // if concede existing, maybe need concede incomming
            IncomingConcurrency(stream, cmpStream);
            isUpdate = true;
            newFlag = stream->audioMode_ == AUDIO_MODE_PLAYBACK ?
                AUDIO_OUTPUT_FLAG_NORMAL : AUDIO_INPUT_FLAG_NORMAL;
            stream->streamAction_ = AUDIO_STREAM_ACTION_RECREATE;
            stream->routeFlag_ = newFlag;
            break;
        default:
            break;
    }
    return isUpdate;
}

uint32_t AudioPipeSelector::GetRouteFlagByStreamDesc(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    uint32_t flag = AUDIO_FLAG_NONE;
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr, flag, "streamDesc is nullptr");
    flag = configManager_.GetRouteFlag(streamDesc);
    return flag;
}

std::string AudioPipeSelector::GetAdapterNameByStreamDesc(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    std::string name = "";
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr, name, "streamDesc is nullptr");
    std::shared_ptr<PipeStreamPropInfo> streamPropInfo = std::make_shared<PipeStreamPropInfo>();
    configManager_.GetStreamPropInfo(streamDesc, streamPropInfo);

    std::shared_ptr<AdapterPipeInfo> pipeInfoPtr = streamPropInfo->pipeInfo_.lock();
    if (pipeInfoPtr == nullptr) {
        AUDIO_ERR_LOG("Adapter info is null");
        return "";
    }

    std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = pipeInfoPtr->adapterInfo_.lock();
    if (adapterInfoPtr == nullptr) {
        AUDIO_ERR_LOG("Pipe info is null");
        return "";
    }
    name = adapterInfoPtr->adapterName;
    return name;
}

void AudioPipeSelector::ConvertStreamDescToPipeInfo(std::shared_ptr<AudioStreamDescriptor> streamDesc,
    std::shared_ptr<PipeStreamPropInfo> streamPropInfo, AudioPipeInfo &info)
{
    std::shared_ptr<AdapterPipeInfo> pipeInfoPtr = streamPropInfo->pipeInfo_.lock();
    if (pipeInfoPtr == nullptr) {
        AUDIO_ERR_LOG("Adapter info is null");
        return ;
    }

    std::shared_ptr<PolicyAdapterInfo> adapterInfoPtr = pipeInfoPtr->adapterInfo_.lock();
    if (adapterInfoPtr == nullptr) {
        AUDIO_ERR_LOG("Pipe info is null");
        return ;
    }

    info.moduleInfo_.format = AudioDefinitionPolicyUtils::enumToFormatStr[streamPropInfo->format_];
    info.moduleInfo_.rate = std::to_string(streamPropInfo->sampleRate_);
    info.moduleInfo_.channels = std::to_string(AudioDefinitionPolicyUtils::ConvertLayoutToAudioChannel(
        streamPropInfo->channelLayout_));
    info.moduleInfo_.bufferSize = std::to_string(streamPropInfo->bufferSize_);

    info.moduleInfo_.lib = pipeInfoPtr->paProp_.lib_;
    info.moduleInfo_.role = pipeInfoPtr->paProp_.role_;
    info.moduleInfo_.name = pipeInfoPtr->paProp_.moduleName_;
    info.moduleInfo_.adapterName = adapterInfoPtr->adapterName;
    info.moduleInfo_.className = adapterInfoPtr->adapterName;
    info.moduleInfo_.OpenMicSpeaker = configManager_.GetUpdateRouteSupport() ? "1" : "0";

    AUDIO_INFO_LOG("Pipe name: %{public}s", pipeInfoPtr->name_.c_str());
    if (pipeInfoPtr->name_ == "multichannel_output") {
        info.moduleInfo_.className = "multichannel";
        info.moduleInfo_.fileName = "mch_dump_file";
        info.moduleInfo_.fixedLatency = "1"; // for fix max request
        info.moduleInfo_.bufferSize =
            std::to_string(((streamPropInfo->bufferSize_ / std::stoul(info.moduleInfo_.channels)) * STEREO));
        AUDIO_INFO_LOG("Buffer size: %{public}s", info.moduleInfo_.bufferSize.c_str());
    } else if (pipeInfoPtr->name_ == "offload_output") {
        info.moduleInfo_.className = "offload";
        info.moduleInfo_.offloadEnable = "1";
        info.moduleInfo_.fixedLatency = "1";
        info.moduleInfo_.fileName = "offload_dump_file";
    }

    info.moduleInfo_.deviceType = std::to_string(streamDesc->newDeviceDescs_[0]->deviceType_);
    info.moduleInfo_.networkId = streamDesc->newDeviceDescs_[0]->networkId_;
    info.moduleInfo_.sourceType = std::to_string(streamDesc->capturerInfo_.sourceType);

    info.streamDescriptors_.push_back(streamDesc);
    info.streamDescMap_[streamDesc->sessionId_] = streamDesc;
    info.routeFlag_ = streamDesc->routeFlag_;
    info.adapterName_ = adapterInfoPtr->adapterName;
    info.pipeRole_ = pipeInfoPtr->role_;
    info.name_ = pipeInfoPtr->name_;
}

AudioStreamAction AudioPipeSelector::JudgeStreamAction(
    std::shared_ptr<AudioPipeInfo> newPipe, std::shared_ptr<AudioPipeInfo> oldPipe)
{
    if (newPipe->adapterName_ == oldPipe->adapterName_ && newPipe->routeFlag_ == oldPipe->routeFlag_) {
        return AUDIO_STREAM_ACTION_DEFAULT;
    }
    if ((oldPipe->routeFlag_ & AUDIO_OUTPUT_FLAG_FAST) || (newPipe->routeFlag_ & AUDIO_OUTPUT_FLAG_FAST) ||
        (oldPipe->routeFlag_ & AUDIO_OUTPUT_FLAG_DIRECT) || (newPipe->routeFlag_ & AUDIO_OUTPUT_FLAG_DIRECT)) {
        return AUDIO_STREAM_ACTION_RECREATE;
    } else {
        return AUDIO_STREAM_ACTION_MOVE;
    }
}

void AudioPipeSelector::SortStreamDescsByStartTime(std::vector<std::shared_ptr<AudioStreamDescriptor>> &streamDescs)
{
    sort(streamDescs.begin(), streamDescs.end(), [](const std::shared_ptr<AudioStreamDescriptor> &streamDesc1,
        const std::shared_ptr<AudioStreamDescriptor> &streamDesc2) {
            return streamDesc1->startTimeStamp_ < streamDesc2->startTimeStamp_;
        });
}
} // namespace AudioStandard
} // namespace OHOS