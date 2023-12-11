/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "audio_policy_server_handler.h"
#include "audio_policy_service.h"

namespace OHOS {
namespace AudioStandard {
AudioPolicyServerHandler::AudioPolicyServerHandler() : AppExecFwk::EventHandler(
    AppExecFwk::EventRunner::Create("AudioPolicyAsyncRunner"))
{
    AUDIO_DEBUG_LOG("AudioPolicyServerHandler created");
}

AudioPolicyServerHandler::~AudioPolicyServerHandler()
{
    AUDIO_WARNING_LOG("AudioPolicyServerHandler destroyed");
};

void AudioPolicyServerHandler::AddAudioPolicyClientProxyMap(int32_t clientPid, const sptr<IAudioPolicyClient>& cb)
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    audioPolicyClientProxyAPSCbsMap_.emplace(clientPid, cb);
    AUDIO_INFO_LOG("AudioPolicyServerHandler::AddAudioPolicyClientProxyMap, group data num [%{public}zu]",
        audioPolicyClientProxyAPSCbsMap_.size());
}

void AudioPolicyServerHandler::ReduceAudioPolicyClientProxyMap(pid_t clientPid)
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    audioPolicyClientProxyAPSCbsMap_.erase(clientPid);
    AUDIO_INFO_LOG("AudioPolicyServerHandler::ReduceAudioPolicyClientProxyMap, group data num [%{public}zu]",
        audioPolicyClientProxyAPSCbsMap_.size());
}

void AudioPolicyServerHandler::AddInterruptCbsMap(uint32_t sessionID,
    const std::shared_ptr<AudioInterruptCallback> &callback)
{
    std::lock_guard<std::mutex> lock(interruptMapMutex_);
    interruptCbsMap_[sessionID] = callback;
    AUDIO_INFO_LOG("AudioPolicyServerHandler::AddInterruptCbsMap, group data num [%{public}zu]",
        interruptCbsMap_.size());
}

int32_t AudioPolicyServerHandler::ReduceInterruptCbsMap(uint32_t sessionID)
{
    std::lock_guard<std::mutex> lock(interruptMapMutex_);
    if (interruptCbsMap_.erase(sessionID) == 0) {
        AUDIO_ERR_LOG("ReduceInterruptCbsMap session %{public}d not present", sessionID);
        return ERR_INVALID_OPERATION;
    }
    return SUCCESS;
}

void AudioPolicyServerHandler::AddExternInterruptCbsMap(int32_t clientId,
    const std::shared_ptr<AudioInterruptCallback> &callback)
{
    std::lock_guard<std::mutex> lock(amInterruptMapMutex_);
    amInterruptCbsMap_[clientId] = callback;
    AUDIO_INFO_LOG("AudioPolicyServerHandler::AddExternInterruptCbsMap, group data num [%{public}zu]",
        amInterruptCbsMap_.size());
}

int32_t AudioPolicyServerHandler::ReduceExternInterruptCbsMap(int32_t clientId)
{
    std::lock_guard<std::mutex> lock(amInterruptMapMutex_);
    if (amInterruptCbsMap_.erase(clientId) == 0) {
        AUDIO_ERR_LOG("ReduceExternInterruptCbsMap client %{public}d not present", clientId);
        return ERR_INVALID_OPERATION;
    }
    return SUCCESS;
}

void AudioPolicyServerHandler::AddAvailableDeviceChangeMap(int32_t clientId, const AudioDeviceUsage usage,
    const sptr<IStandardAudioPolicyManagerListener> &callback)
{
    std::lock_guard<std::mutex> lock(updateAvailableDeviceChangeMapMutex_);
    availableDeviceChangeCbsMap_[{clientId, usage}] = callback;
    AUDIO_INFO_LOG("AudioPolicyServerHandler::AddAvailableDeviceChangeMap, group data num [%{public}zu]",
        availableDeviceChangeCbsMap_.size());
}

void AudioPolicyServerHandler::ReduceAvailableDeviceChangeMap(const int32_t clientId, AudioDeviceUsage usage)
{
    std::lock_guard<std::mutex> lock(updateAvailableDeviceChangeMapMutex_);
    if (availableDeviceChangeCbsMap_.erase({clientId, usage}) == 0) {
        AUDIO_INFO_LOG("client not present in %{public}s", __func__);
    }
    // for routing manager napi remove all device change callback
    if (usage == AudioDeviceUsage::D_ALL_DEVICES) {
        for (auto it = availableDeviceChangeCbsMap_.begin(); it != availableDeviceChangeCbsMap_.end();) {
            if ((*it).first.first == clientId) {
                it = availableDeviceChangeCbsMap_.erase(it);
            } else {
                it++;
            }
        }
    }
    AUDIO_INFO_LOG("AudioPolicyServerHandler::ReduceAvailableDeviceChangeMap, group data num [%{public}zu]",
        availableDeviceChangeCbsMap_.size());
}

void AudioPolicyServerHandler::AddDistributedRoutingRoleChangeCbsMap(int32_t clientId,
    const sptr<IStandardAudioRoutingManagerListener> &callback)
{
    std::lock_guard<std::mutex> lock(updateAvailableDeviceChangeMapMutex_);
    if (callback != nullptr) {
        distributedRoutingRoleChangeCbsMap_[clientId] = callback;
    }
    AUDIO_DEBUG_LOG("SetDistributedRoutingRoleCallback: distributedRoutingRoleChangeCbsMap_ size: %{public}zu",
        distributedRoutingRoleChangeCbsMap_.size());
}

int32_t AudioPolicyServerHandler::RemoveDistributedRoutingRoleChangeCbsMap(int32_t clientId)
{
    std::lock_guard<std::mutex> lock(updateAvailableDeviceChangeMapMutex_);
    if (distributedRoutingRoleChangeCbsMap_.erase(clientId) == 0) {
        AUDIO_ERR_LOG("RemoveDistributedRoutingRoleChangeCbsMap clientPid %{public}d not present", clientId);
        return ERR_INVALID_OPERATION;
    }

    AUDIO_DEBUG_LOG("UnsetDistributedRoutingRoleCallback: distributedRoutingRoleChangeCbsMap_ size: %{public}zu",
        distributedRoutingRoleChangeCbsMap_.size());
    return SUCCESS;
}

bool AudioPolicyServerHandler::SendDeviceChangedCallback(const vector<sptr<AudioDeviceDescriptor>> &desc,
    bool isConnected)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->deviceChangeAction.type = isConnected ? DeviceChangeType::CONNECT : DeviceChangeType::DISCONNECT;
    eventContextObj->deviceChangeAction.deviceDescriptors = desc;

    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::AUDIO_DEVICE_CHANGE, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendDeviceChangedCallback event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendAvailableDeviceChange(const vector<sptr<AudioDeviceDescriptor>> &desc,
    bool isConnected)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->deviceChangeAction.type = isConnected ? DeviceChangeType::CONNECT : DeviceChangeType::DISCONNECT;
    eventContextObj->deviceChangeAction.deviceDescriptors = desc;

    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::AVAILABLE_AUDIO_DEVICE_CHANGE,
        eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendAvailableDeviceChange event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendVolumeKeyEventCallback(const VolumeEvent &volumeEvent)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->volumeEvent = volumeEvent;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::VOLUME_KEY_EVENT, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendVolumeKeyEventCallback event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendAudioFocusInfoChangeCallBack(int32_t callbackCategory,
    const AudioInterrupt &audioInterrupt, const std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->audioInterrupt = audioInterrupt;
    eventContextObj->focusInfoList = focusInfoList;
    bool ret = false;

    lock_guard<mutex> runnerlock(runnerMutex_);
    if (callbackCategory == REQUEST_CALLBACK_CATEGORY) {
        ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::REQUEST_CALLBACK_CATEGORY, eventContextObj));
        CHECK_AND_RETURN_RET_LOG(ret, ret, "Send REQUEST_CALLBACK_CATEGORY event failed");
    } else if (callbackCategory == ABANDON_CALLBACK_CATEGORY) {
        ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::ABANDON_CALLBACK_CATEGORY, eventContextObj));
        CHECK_AND_RETURN_RET_LOG(ret, ret, "Send ABANDON_CALLBACK_CATEGORY event failed");
    }
    ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::FOCUS_INFOCHANGE, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send FOCUS_INFOCHANGE event failed");

    return ret;
}

bool AudioPolicyServerHandler::SendRingerModeUpdatedCallBack(const AudioRingerMode &ringMode)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->ringMode = ringMode;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::RINGER_MODEUPDATE_EVENT, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send RINGER_MODEUPDATE_EVENT event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendMicStateUpdatedCallBack(const MicStateChangeEvent &micStateChangeEvent)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->micStateChangeEvent = micStateChangeEvent;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::MIC_STATE_CHANGE_EVENT, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send MIC_STATE_CHANGE_EVENT event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendInterruptEventInternalCallBack(const InterruptEventInternal &interruptEvent)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->interruptEvent = interruptEvent;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::INTERRUPT_EVENT, eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send INTERRUPT_EVENT event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendInterruptEventWithSeesionIdCallBack(const InterruptEventInternal &interruptEvent,
    const uint32_t &sessionID)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->interruptEvent = interruptEvent;
    eventContextObj->sessionID = sessionID;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::INTERRUPT_EVENT_WITH_SESSIONID,
        eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send INTERRUPT_EVENT_WITH_SESSIONID event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendInterruptEventWithClientIdCallBack(const InterruptEventInternal &interruptEvent,
    const int32_t &clientId)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->interruptEvent = interruptEvent;
    eventContextObj->clientId = clientId;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::INTERRUPT_EVENT_WITH_CLIENTID,
        eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "Send INTERRUPT_EVENT_WITH_CLIENTID event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendPreferredOutputDeviceUpdated()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::PREFERRED_OUTPUT_DEVICE_UPDATED));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendPreferredOutputDeviceUpdated event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendPreferredInputDeviceUpdated()
{
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::PREFERRED_INPUT_DEVICE_UPDATED));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendPreferredInputDeviceUpdated event failed");
    return ret;
}

bool AudioPolicyServerHandler::SendDistributedRoutingRoleChange(const sptr<AudioDeviceDescriptor> &descriptor,
    const CastType &type)
{
    std::shared_ptr<EventContextObj> eventContextObj = std::make_shared<EventContextObj>();
    CHECK_AND_RETURN_RET_LOG(eventContextObj != nullptr, false, "EventContextObj get nullptr");
    eventContextObj->descriptor = descriptor;
    eventContextObj->type = type;
    lock_guard<mutex> runnerlock(runnerMutex_);
    bool ret = SendEvent(AppExecFwk::InnerEvent::Get(EventAudioServerCmd::DISTRIBUTED_ROUTING_ROLE_CHANGE,
        eventContextObj));
    CHECK_AND_RETURN_RET_LOG(ret, ret, "SendDistributedRoutingRoleChange event failed");
    return ret;
}

void AudioPolicyServerHandler::HandleDeviceChangedCallback(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        if (it->second && eventContextObj->deviceChangeAction.deviceDescriptors.size() > 0) {
            if (!(it->second->hasBTPermission_)) {
                AudioPolicyService::GetAudioPolicyService().
                    UpdateDescWhenNoBTPermission(eventContextObj->deviceChangeAction.deviceDescriptors);
            }
            it->second->OnDeviceChange(eventContextObj->deviceChangeAction);
        }
    }
}

void AudioPolicyServerHandler::HandleAvailableDeviceChange(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updateAvailableDeviceChangeMapMutex_);
    for (auto it = availableDeviceChangeCbsMap_.begin(); it != availableDeviceChangeCbsMap_.end(); ++it) {
        AudioDeviceUsage usage = it->first.second;
        eventContextObj->deviceChangeAction.deviceDescriptors = AudioPolicyService::GetAudioPolicyService().
            DeviceFilterByUsage(it->first.second, eventContextObj->deviceChangeAction.deviceDescriptors);
        if (it->second && eventContextObj->deviceChangeAction.deviceDescriptors.size() > 0) {
            if (!(it->second->hasBTPermission_)) {
                AudioPolicyService::GetAudioPolicyService().
                    UpdateDescWhenNoBTPermission(eventContextObj->deviceChangeAction.deviceDescriptors);
            }
            it->second->OnAvailableDeviceChange(usage, eventContextObj->deviceChangeAction);
        }
    }
}

void AudioPolicyServerHandler::HandleVolumeKeyEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        sptr<IAudioPolicyClient> volumeChangeCb = it->second;
        if (volumeChangeCb == nullptr) {
            AUDIO_ERR_LOG("volumeChangeCb: nullptr for client : %{public}d", it->first);
            continue;
        }
        AUDIO_DEBUG_LOG("SetA2dpDeviceVolume trigger volumeChangeCb clientPid : %{public}d", it->first);
        volumeChangeCb->OnVolumeKeyEvent(eventContextObj->volumeEvent);
    }
}

void AudioPolicyServerHandler::HandleRequestCateGoryEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");

    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        it->second->OnAudioFocusRequested(eventContextObj->audioInterrupt);
    }
}

void AudioPolicyServerHandler::HandleAbandonCateGoryEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        it->second->OnAudioFocusAbandoned(eventContextObj->audioInterrupt);
    }
}

void AudioPolicyServerHandler::HandleFocusInfoChangeEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    AUDIO_INFO_LOG("HandleFocusInfoChangeEvent focusInfoList :%{public}zu", eventContextObj->focusInfoList.size());
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        it->second->OnAudioFocusInfoChange(eventContextObj->focusInfoList);
    }
}

void AudioPolicyServerHandler::HandleRingerModeUpdatedEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        sptr<IAudioPolicyClient> ringerModeListenerCb = it->second;
        if (ringerModeListenerCb == nullptr) {
            AUDIO_ERR_LOG("ringerModeListenerCb nullptr for client %{public}d", it->first);
            continue;
        }

        AUDIO_DEBUG_LOG("ringerModeListenerCb client %{public}d", it->first);
        ringerModeListenerCb->OnRingerModeUpdated(eventContextObj->ringMode);
    }
}

void AudioPolicyServerHandler::HandleMicStateUpdatedEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        sptr<IAudioPolicyClient> micStateChangeListenerCb = it->second;
        if (micStateChangeListenerCb == nullptr) {
            AUDIO_ERR_LOG("callback is nullptr for client %{public}d", it->first);
            continue;
        }
        micStateChangeListenerCb->OnMicStateUpdated(eventContextObj->micStateChangeEvent);
    }
}

void AudioPolicyServerHandler::HandleInterruptEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(interruptMapMutex_);
    for (auto it : interruptCbsMap_) {
        if (it.second != nullptr) {
            it.second->OnInterrupt(eventContextObj->interruptEvent);
        }
    }
}

void AudioPolicyServerHandler::HandleInterruptEventWithSessionId(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");

    std::lock_guard<std::mutex> lock(interruptMapMutex_);
    std::shared_ptr<AudioInterruptCallback> policyListenerCb = interruptCbsMap_[eventContextObj->sessionID];
    CHECK_AND_RETURN_LOG(policyListenerCb != nullptr, "policyListenerCb get nullptr");
    policyListenerCb->OnInterrupt(eventContextObj->interruptEvent);
}

void AudioPolicyServerHandler::HandleInterruptEventWithClientId(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");

    std::lock_guard<std::mutex> lock(amInterruptMapMutex_);
    std::shared_ptr<AudioInterruptCallback> policyListenerCb = amInterruptCbsMap_[eventContextObj->clientId];
    CHECK_AND_RETURN_LOG(policyListenerCb != nullptr, "policyListenerCb get nullptr");
    policyListenerCb->OnInterrupt(eventContextObj->interruptEvent);
}

void AudioPolicyServerHandler::HandlePreferredOutputDeviceUpdated()
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        AudioRendererInfo rendererInfo;
        auto deviceDescs = AudioPolicyService::GetAudioPolicyService().
            GetPreferredOutputDeviceDescriptors(rendererInfo);
        if (!(it->second->hasBTPermission_)) {
            AudioPolicyService::GetAudioPolicyService().
                UpdateDescWhenNoBTPermission(deviceDescs);
        }
        it->second->OnPreferredOutputDeviceUpdated(deviceDescs);
    }
}

void AudioPolicyServerHandler::HandlePreferredInputDeviceUpdated()
{
    std::lock_guard<std::mutex> lock(updatePolicyPorxyMapMutex_);
    for (auto it = audioPolicyClientProxyAPSCbsMap_.begin(); it != audioPolicyClientProxyAPSCbsMap_.end(); ++it) {
        AudioCapturerInfo captureInfo;
        auto deviceDescs = AudioPolicyService::GetAudioPolicyService().GetPreferredInputDeviceDescriptors(captureInfo);
        if (!(it->second->hasBTPermission_)) {
            AudioPolicyService::GetAudioPolicyService().UpdateDescWhenNoBTPermission(deviceDescs);
        }
        it->second->OnPreferredInputDeviceUpdated(deviceDescs);
    }
}

void AudioPolicyServerHandler::HandleDistributedRoutingRoleChangeEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    std::shared_ptr<EventContextObj> eventContextObj = event->GetSharedObject<EventContextObj>();
    CHECK_AND_RETURN_LOG(eventContextObj != nullptr, "EventContextObj get nullptr");
    std::lock_guard<std::mutex> lock(configDistributedRoutingMutex_);
    for (auto it = distributedRoutingRoleChangeCbsMap_.begin(); it != distributedRoutingRoleChangeCbsMap_.end(); it++) {
        it->second->OnDistributedRoutingRoleChange(eventContextObj->descriptor, eventContextObj->type);
    }
}

void AudioPolicyServerHandler::ProcessEvent(const AppExecFwk::InnerEvent::Pointer &event)
{
    uint32_t eventId = event->GetInnerEventId();
    AUDIO_INFO_LOG("eventId:%{public}zu", eventId);

    switch (eventId) {
        case EventAudioServerCmd::AUDIO_DEVICE_CHANGE:
            HandleDeviceChangedCallback(event);
            break;
        case EventAudioServerCmd::VOLUME_KEY_EVENT:
            HandleVolumeKeyEvent(event);
            break;
        case EventAudioServerCmd::REQUEST_CALLBACK_CATEGORY:
            HandleRequestCateGoryEvent(event);
            break;
        case EventAudioServerCmd::ABANDON_CALLBACK_CATEGORY:
            HandleAbandonCateGoryEvent(event);
            break;
        case EventAudioServerCmd::FOCUS_INFOCHANGE:
            HandleFocusInfoChangeEvent(event);
            break;
        case EventAudioServerCmd::RINGER_MODEUPDATE_EVENT:
            HandleRingerModeUpdatedEvent(event);
            break;
        case EventAudioServerCmd::MIC_STATE_CHANGE_EVENT:
            HandleMicStateUpdatedEvent(event);
            break;
        case EventAudioServerCmd::INTERRUPT_EVENT:
            HandleInterruptEvent(event);
            break;
        case EventAudioServerCmd::INTERRUPT_EVENT_WITH_SESSIONID:
            HandleInterruptEventWithSessionId(event);
            break;
        case EventAudioServerCmd::INTERRUPT_EVENT_WITH_CLIENTID:
            HandleInterruptEventWithClientId(event);
            break;
        case EventAudioServerCmd::PREFERRED_OUTPUT_DEVICE_UPDATED:
            HandlePreferredOutputDeviceUpdated();
            break;
        case EventAudioServerCmd::PREFERRED_INPUT_DEVICE_UPDATED:
            HandlePreferredInputDeviceUpdated();
            break;
        case EventAudioServerCmd::AVAILABLE_AUDIO_DEVICE_CHANGE:
            HandleAvailableDeviceChange(event);
            break;
        case EventAudioServerCmd::DISTRIBUTED_ROUTING_ROLE_CHANGE:
            HandleDistributedRoutingRoleChangeEvent(event);
            break;
        default:
            break;
    }
}
} // namespace AudioStandard
} // namespace OHOS
