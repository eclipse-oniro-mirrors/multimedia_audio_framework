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
#define LOG_TAG "HpaeManager"
#endif
#include "hpae_manager.h"
#include <string>
#include <atomic>
#include <unordered_map>
#include "audio_errors.h"
#include "audio_schedule.h"
#include "audio_engine_log.h"
#include "audio_utils.h"
#include "audio_setting_provider.h"
#include "hpae_node_common.h"
#include "system_ability_definition.h"
#include "hpae_co_buffer_node.h"
namespace OHOS {
namespace AudioStandard {
namespace HPAE {
namespace {
constexpr uint32_t DEFAULT_SUSPEND_TIME_IN_MS = 3000;  // 3s to stop hdi
static inline const std::unordered_set<SourceType> INNER_SOURCE_TYPE_SET = {
    SOURCE_TYPE_PLAYBACK_CAPTURE, SOURCE_TYPE_REMOTE_CAST};
}  // namespace
static constexpr int32_t SINK_INVALID_ID = -1;
static const std::string BT_SINK_NAME = "Bt_Speaker";

HpaeManagerThread::~HpaeManagerThread()
{
    DeactivateThread();
}

void HpaeManagerThread::ActivateThread(HpaeManager *hpaeManager)
{
    m_hpaeManager = hpaeManager;
    auto threadFunc = std::bind(&HpaeManagerThread::Run, this);
    thread_ = std::thread(threadFunc);
    pthread_setname_np(thread_.native_handle(), "HpaeManager");
}

void HpaeManagerThread::Run()
{
    running_.store(true);
    ScheduleThreadInServer(getpid(), gettid());
    while (running_.load() && m_hpaeManager != nullptr) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            bool isProcessing = m_hpaeManager->IsMsgProcessing();
            bool signal = recvSignal_.load();
            Trace trace("runFunc:" + std::to_string(signal) + " isPorcessing:" + std::to_string(isProcessing));
            condition_.wait(lock, [this] { return m_hpaeManager->IsMsgProcessing() || recvSignal_.load(); });
        }
        m_hpaeManager->HandleMsg();
        recvSignal_.store(false);
    }
    UnscheduleThreadInServer(getpid(), gettid());
}

void HpaeManagerThread::Notify()
{
    std::unique_lock<std::mutex> lock(mutex_);
    recvSignal_.store(true);
    condition_.notify_all();
}

void HpaeManagerThread::DeactivateThread()
{
    Notify();
    running_.store(false);
    if (thread_.joinable()) {
        thread_.join();
    }
}

HpaeManager::HpaeManager() : hpaeNoLockQueue_(CURRENT_REQUEST_COUNT)  // todo Message queue exceeds the upper limit
{
    RegisterHandler(UPDATE_STATUS, &HpaeManager::HandleUpdateStatus);
    RegisterHandler(INIT_DEVICE_RESULT, &HpaeManager::HandleInitDeviceResult);
    RegisterHandler(DEINIT_DEVICE_RESULT, &HpaeManager::HandleDeInitDeviceResult);
    RegisterHandler(MOVE_SINK_INPUT, &HpaeManager::HandleMoveSinkInput);
    RegisterHandler(MOVE_ALL_SINK_INPUT, &HpaeManager::HandleMoveAllSinkInputs);
    RegisterHandler(MOVE_SOURCE_OUTPUT, &HpaeManager::HandleMoveSourceOutput);
    RegisterHandler(MOVE_ALL_SOURCE_OUTPUT, &HpaeManager::HandleMoveAllSourceOutputs);
    RegisterHandler(DUMP_SINK_INFO, &HpaeManager::HandleDumpSinkInfo);
    RegisterHandler(DUMP_SOURCE_INFO, &HpaeManager::HandleDumpSourceInfo);
    RegisterHandler(MOVE_SESSION_FAILED, &HpaeManager::HandleMoveSessionFailed);
    RegisterHandler(CONNECT_CO_BUFFER_NODE, &HpaeManager::HandleConnectCoBufferNode);
    RegisterHandler(DISCONNECT_CO_BUFFER_NODE, &HpaeManager::HandleDisConnectCoBufferNode);
}

HpaeManager::~HpaeManager()
{
    if (IsInit()) {
        DeInit();
    }
}

int32_t HpaeManager::Init()
{
    sinkSourceIndex_ = 0;
    hpaeManagerThread_ = std::make_unique<HpaeManagerThread>();
    hpaeManagerThread_->ActivateThread(this);
    isInit_.store(true);
    return 0;
}

int32_t HpaeManager::SuspendAudioDevice(std::string &audioPortName, bool isSuspend)
{
    AUDIO_INFO_LOG("suspend audio device: %{public}s, isSuspend: %{public}d", audioPortName.c_str(), isSuspend);
    auto request = [this, audioPortName, isSuspend]() {
        if (SafeGetMap(rendererManagerMap_, audioPortName)) {
            rendererManagerMap_[audioPortName]->SuspendStreamManager(isSuspend);
        } else if (SafeGetMap(capturerManagerMap_, audioPortName)) {
            AUDIO_WARNING_LOG("capture not support suspend");
            return;
        } else {
            AUDIO_WARNING_LOG("can not find sink: %{public}s", audioPortName.c_str());
            return;
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

bool HpaeManager::SetSinkMute(const std::string &sinkName, bool isMute, bool isSync)
{
    auto request = [this, sinkName, isMute, isSync]() {
        // todo for device change
        AUDIO_INFO_LOG("HpaeManager::SetSinkMute sinkName: %{public}s isMute: %{public}d, isSync: %{public}d",
            sinkName.c_str(),
            isMute,
            isSync);
        if (SafeGetMap(rendererManagerMap_, sinkName)) {
            rendererManagerMap_[sinkName]->SetMute(isMute);
        } else {
            AUDIO_WARNING_LOG("can not find sink: %{public}s for mute:%{public}d", sinkName.c_str(), isMute);
        }
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnSetSinkMuteCb(SUCCESS);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetSourceOutputMute(int32_t uid, bool setMute)
{
    auto request = [this, uid, setMute]() {
        AUDIO_INFO_LOG("HpaeManager::SetSourceOutputMute uid: %{public}d setMute: %{public}d", uid, setMute);
        if (capturerIdSourceNameMap_.find(uid) != capturerIdSourceNameMap_.end()) {
            capturerManagerMap_[capturerIdSourceNameMap_[uid]]->SetMute(setMute);
        } else {
            AUDIO_WARNING_LOG("can not find sink: %{public}d for mute:%{public}d", uid, setMute);
        }
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnSetSourceOutputMuteCb(SUCCESS);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAllSinks()
{
    auto request = [this]() {
        std::vector<SinkInfo> sinks;
        // todo for device change
        AUDIO_INFO_LOG("HpaeManager::GetAllSinks end");
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnGetAllSinksCb(SUCCESS, sinks);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::DeInit()
{
    if (hpaeManagerThread_ != nullptr) {
        hpaeManagerThread_->DeactivateThread();
        hpaeManagerThread_ = nullptr;
    }
    hpaeNoLockQueue_.HandleRequests();  // todo suspend
    isInit_.store(false);
    AUDIO_INFO_LOG("HpaeManager::DeInit end");
    return SUCCESS;
}

int32_t HpaeManager::RegisterSerivceCallback(const std::weak_ptr<AudioServiceHpaeCallback> &callback)
{
    auto request = [this, callback]() {
        serviceCallback_ = callback;
        AUDIO_INFO_LOG("HpaeManager::RegisterSerivceCallback end");
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::RegisterHpaeDumpCallback(const std::weak_ptr<AudioServiceHpaeDumpCallback> &callback)
{
    auto request = [this, callback]() {
        dumpCallback_ = callback;
        AUDIO_INFO_LOG("HpaeManager::RegisterHpaeDumpCallback end");
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::PrintAudioModuleInfo(const AudioModuleInfo &audioModuleInfo)
{
    AUDIO_INFO_LOG("rate: %{public}s ch: %{public}s buffersize: %{public}s ",
        audioModuleInfo.rate.c_str(),
        audioModuleInfo.channels.c_str(),
        audioModuleInfo.bufferSize.c_str());
    AUDIO_INFO_LOG("format: %{public}s name: %{public}s  lib: %{public}s ",
        audioModuleInfo.format.c_str(),
        audioModuleInfo.name.c_str(),
        audioModuleInfo.lib.c_str());
    AUDIO_INFO_LOG("deviceType: %{public}s  className: %{public}s  adapterName: %{public}s ",
        audioModuleInfo.deviceType.c_str(),
        audioModuleInfo.className.c_str(),
        audioModuleInfo.adapterName.c_str());
    AUDIO_INFO_LOG("OpenMicSpeaker: %{public}s networkId: %{public}s fileName: %{public}s ",
        audioModuleInfo.OpenMicSpeaker.c_str(),
        audioModuleInfo.networkId.c_str(),
        audioModuleInfo.fileName.c_str());
    AUDIO_INFO_LOG("fixedLatency: %{public}s sinkLatency: %{public}s renderInIdleState: %{public}s ",
        audioModuleInfo.fixedLatency.c_str(),
        audioModuleInfo.sinkLatency.c_str(),
        audioModuleInfo.renderInIdleState.c_str());
    AUDIO_INFO_LOG("sceneName: %{public}s sourceType: %{public}s offloadEnable: %{public}s ",
        audioModuleInfo.sceneName.c_str(),
        audioModuleInfo.sourceType.c_str(),
        audioModuleInfo.offloadEnable.c_str());
}

int32_t HpaeManager::ReloadRenderManager(const AudioModuleInfo &audioModuleInfo)
{
    HpaeSinkInfo sinkInfo;
    sinkInfo.sinkId = sinkNameSinkIdMap_[audioModuleInfo.name];
    sinkInfo.suspendTime = DEFAULT_SUSPEND_TIME_IN_MS;
    int32_t ret = TransModuleInfoToHpaeSinkInfo(audioModuleInfo, sinkInfo);
    if (ret != SUCCESS) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        }
        return ret;
    }
    rendererManagerMap_[audioModuleInfo.name]->ReloadRenderManager(sinkInfo);
    return SUCCESS;
}

int32_t HpaeManager::OpenOutputAudioPort(const AudioModuleInfo &audioModuleInfo, uint32_t sinkSourceIndex)
{
    if (SafeGetMap(rendererManagerMap_, audioModuleInfo.name)) {
        AUDIO_INFO_LOG("sink name: %{public}s already open", audioModuleInfo.name.c_str());
        if (!rendererManagerMap_[audioModuleInfo.name]->IsInit()) {
            if (ReloadRenderManager(audioModuleInfo) != SUCCESS) {
                return ERROR;
            }
        } else if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[audioModuleInfo.name]);
        }
        return sinkNameSinkIdMap_[audioModuleInfo.name];
    }
    sinkSourceIndex_.fetch_add(1);
    HpaeSinkInfo sinkInfo;
    sinkInfo.sinkId = sinkSourceIndex;
    sinkInfo.suspendTime = DEFAULT_SUSPEND_TIME_IN_MS;
    int32_t ret = TransModuleInfoToHpaeSinkInfo(audioModuleInfo, sinkInfo);
    if (ret != SUCCESS) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        }
        return ret;
    }
    auto rendererManager = IHpaeRendererManager::CreateRendererManager(sinkInfo);
    rendererManager->RegisterSendMsgCallback(weak_from_this());
    rendererManagerMap_[audioModuleInfo.name] = rendererManager;
    sinkNameSinkIdMap_[audioModuleInfo.name] = sinkSourceIndex;
    sinkIdSinkNameMap_[sinkSourceIndex] = audioModuleInfo.name;
    if (defaultSink_ == "" && coreSink_ == "") {
        defaultSink_ = audioModuleInfo.name;
        coreSink_ = audioModuleInfo.name;
        AUDIO_INFO_LOG("SetDefaultSink name: %{public}s", defaultSink_.c_str());
    }
    rendererManager->Init();
    AUDIO_INFO_LOG(
        "open sink name: %{public}s end sinkIndex is %{public}u", audioModuleInfo.name.c_str(), sinkSourceIndex);
    return SUCCESS;
}

int32_t HpaeManager::OpenInputAudioPort(const AudioModuleInfo &audioModuleInfo, uint32_t sinkSourceIndex)
{
    HpaeSourceInfo sourceInfo;
    int32_t ret = TransModuleInfoToHpaeSourceInfo(audioModuleInfo, sourceInfo);
    if (ret != SUCCESS) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        }
        return ret;
    }
    if (SafeGetMap(capturerManagerMap_, audioModuleInfo.name)) {
        HpaeSourceInfo oldInfo = capturerManagerMap_[audioModuleInfo.name]->GetSourceInfo();
        if (CheckSourceInfoIsDifferent(sourceInfo, oldInfo)) {
            AUDIO_INFO_LOG("source name: %{public}s need reload", audioModuleInfo.name.c_str());
            sourceInfo.sourceId = oldInfo.sourceId;
            capturerManagerMap_[audioModuleInfo.name]->ReloadCaptureManager(sourceInfo);
            return sourceNameSourceIdMap_[audioModuleInfo.name];
        }
        AUDIO_INFO_LOG("source name: %{public}s already open", audioModuleInfo.name.c_str());
        if (!capturerManagerMap_[audioModuleInfo.name]->IsInit()) {
            capturerManagerMap_[audioModuleInfo.name]->Init();
        } else if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(sourceNameSourceIdMap_[audioModuleInfo.name]);
        }
        return sourceNameSourceIdMap_[audioModuleInfo.name];
    }
    sinkSourceIndex_.fetch_add(1);
    sourceInfo.sourceId = sinkSourceIndex;
    auto capturerManager = std::make_shared<HpaeCapturerManager>(sourceInfo);
    capturerManager->RegisterSendMsgCallback(weak_from_this());
    capturerManagerMap_[audioModuleInfo.name] = capturerManager;
    sourceNameSourceIdMap_[audioModuleInfo.name] = sinkSourceIndex;
    sourceIdSourceNameMap_[sinkSourceIndex] = audioModuleInfo.name;
    capturerManagerMap_[audioModuleInfo.name]->Init();
    AUDIO_INFO_LOG(
        "open source name: %{public}s end sourceIndex is %{public}u", audioModuleInfo.name.c_str(), sinkSourceIndex);
    return SUCCESS;
}

int32_t HpaeManager::OpenVirtualAudioPort(const AudioModuleInfo &audioModuleInfo, uint32_t sinkSourceIndex)
{
    if (SafeGetMap(rendererManagerMap_, audioModuleInfo.name)) {
        AUDIO_INFO_LOG("inner capture name: %{public}s already open", audioModuleInfo.name.c_str());
        if (!rendererManagerMap_[audioModuleInfo.name]->IsInit()) {
            rendererManagerMap_[audioModuleInfo.name]->Init();
        } else if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[audioModuleInfo.name]);
        }
        return sinkNameSinkIdMap_[audioModuleInfo.name];
    }
    sinkSourceIndex_.fetch_add(1);
    HpaeSinkInfo sinkInfo;
    sinkInfo.sinkId = sinkSourceIndex;
    int32_t ret = TransModuleInfoToHpaeSinkInfo(audioModuleInfo, sinkInfo);
    sinkInfo.deviceClass = audioModuleInfo.name;
    sinkInfo.adapterName = audioModuleInfo.name;
    if (ret != SUCCESS) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        }
        return ret;
    }
    auto rendererManager = IHpaeRendererManager::CreateRendererManager(sinkInfo);
    rendererManager->RegisterSendMsgCallback(weak_from_this());
    rendererManagerMap_[audioModuleInfo.name] = rendererManager;
    sinkNameSinkIdMap_[audioModuleInfo.name] = sinkSourceIndex;
    sinkIdSinkNameMap_[sinkSourceIndex] = audioModuleInfo.name;
    rendererManagerMap_[audioModuleInfo.name]->Init();
    AUDIO_INFO_LOG("HpaeManager::OpenAudioPort name: %{public}s end sinkIndex is %{public}u",
        audioModuleInfo.name.c_str(),
        sinkSourceIndex);
    return SUCCESS;
}

int32_t HpaeManager::OpenAudioPortInner(const AudioModuleInfo &audioModuleInfo)
{
    uint32_t sinkSourceIndex = static_cast<uint32_t>(sinkSourceIndex_.load());
    if ((audioModuleInfo.lib != "libmodule-hdi-source.z.so") &&
        (audioModuleInfo.lib != "libmodule-inner-capturer-sink.z.so")) {
        OpenOutputAudioPort(audioModuleInfo, sinkSourceIndex);
    } else if (audioModuleInfo.lib == "libmodule-hdi-source.z.so") {
        OpenInputAudioPort(audioModuleInfo, sinkSourceIndex);
    } else {
        OpenVirtualAudioPort(audioModuleInfo, sinkSourceIndex);
    }
    return sinkSourceIndex;
}

uint32_t HpaeManager::OpenAudioPort(const AudioModuleInfo &audioModuleInfo)
{
    auto request = [this, audioModuleInfo]() {
        PrintAudioModuleInfo(audioModuleInfo);
        OpenAudioPortInner(audioModuleInfo);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::DumpSinkInfo(std::string deviceName)
{
    auto request = [this, deviceName]() {
        AUDIO_INFO_LOG("DumpSinkInfo %{public}s", deviceName.c_str());
        if (!SafeGetMap(rendererManagerMap_, deviceName)) {
            AUDIO_WARNING_LOG("can not find sinkName: %{public}s in rendererManagerMap_", deviceName.c_str());
            if (auto ptr = dumpCallback_.lock()) {
                std::string dumpStr;
                ptr->OnDumpSinkInfoCb(dumpStr, ERROR);
            }
            return;
        }
        rendererManagerMap_[deviceName]->DumpSinkInfo();
    };
    SendRequest(request, __func__);
}

void HpaeManager::DumpSourceInfo(std::string deviceName)
{
    auto request = [this, deviceName]() {
        AUDIO_INFO_LOG("DumpSourceInfo %{public}s", deviceName.c_str());
        if (!SafeGetMap(capturerManagerMap_, deviceName)) {
            AUDIO_WARNING_LOG("can not find sourceName: %{public}s in capturerManagerMap_", deviceName.c_str());
            if (auto ptr = dumpCallback_.lock()) {
                std::string dumpStr;
                ptr->OnDumpSourceInfoCb(dumpStr, ERROR);
            }
            return;
        }
        capturerManagerMap_[deviceName]->DumpSourceInfo();
    };
    SendRequest(request, __func__);
}

void HpaeManager::DumpAllAvailableDevice(HpaeDeviceInfo &devicesInfo)
{
    auto request = [this, &devicesInfo]() {
        AUDIO_INFO_LOG("DumpAllAvailableDevice");
        devicesInfo.sinkInfos.clear();
        for (auto rendererPair : rendererManagerMap_) {
            devicesInfo.sinkInfos.emplace_back(
                HpaeSinkSourceInfo{rendererPair.first, rendererPair.second->GetDeviceHDFDumpInfo()});
        }
        devicesInfo.sourceInfos.clear();
        for (auto capturerPair : capturerManagerMap_) {
            devicesInfo.sourceInfos.emplace_back(
                HpaeSinkSourceInfo{capturerPair.first, capturerPair.second->GetDeviceHDFDumpInfo()});
        }
        if (auto ptr = dumpCallback_.lock()) {
            ptr->OnDumpAllAvailableDeviceCb(SUCCESS);
        }
    };
    SendRequest(request, __func__);
}

void HpaeManager::AddPreferSinkForDefaultChange(bool isAdd, const std::string &sinkName)
{
    if (!isAdd) {
        return;
    }
    for (const auto& sinkinput : rendererIdSinkNameMap_) {
        if (sinkinput.second == sinkName) {
            idPreferSinkNameMap_[sinkinput.first] = sinkName;
        }
    }
}

int32_t HpaeManager::CloseOutAudioPort(std::string sinkName)
{
    if (!SafeGetMap(rendererManagerMap_, sinkName)) {
        AUDIO_WARNING_LOG("can not find sinkName: %{public}s in rendererManagerMap_", sinkName.c_str());
        return SUCCESS;
    }
    bool isChangeDefaultSink = false;
    if (sinkName == defaultSink_ && defaultSink_ != coreSink_) {
        if (GetRendererManagerByName(coreSink_) != nullptr) {
            AUDIO_INFO_LOG("reset default sink to core sink.");
            defaultSink_ = coreSink_;
            isChangeDefaultSink = true;
        } else {
            AUDIO_ERR_LOG("can not find core sink to replace default sink.");
        }
    }
    AddPreferSinkForDefaultChange(isChangeDefaultSink, sinkName);
    rendererManagerMap_[sinkName]->DeInit(sinkName != defaultSink_);
    if (sinkName != defaultSink_) {
        rendererManagerMap_.erase(sinkName);
        sinkIdSinkNameMap_.erase(sinkNameSinkIdMap_[sinkName]);
        sinkNameSinkIdMap_.erase(sinkName);
    }
    return SUCCESS;
}

int32_t HpaeManager::CloseInAudioPort(std::string sourceName)
{
    if (!SafeGetMap(capturerManagerMap_, sourceName)) {
        AUDIO_WARNING_LOG("can not find sourceName: %{public}s in capturerManagerMap_", sourceName.c_str());
        return SUCCESS;
    }
    capturerManagerMap_[sourceName]->DeInit(sourceName != defaultSource_);
    if (sourceName != defaultSource_) {
        capturerManagerMap_.erase(sourceName);
        sourceIdSourceNameMap_.erase(sourceNameSourceIdMap_[sourceName]);
        sourceNameSourceIdMap_.erase(sourceName);
    }
    return SUCCESS;
}

int32_t HpaeManager::CloseAudioPort(int32_t audioHandleIndex)
{
    auto request = [this, audioHandleIndex]() {
        int32_t ret = -1;
        if (sinkIdSinkNameMap_.find(audioHandleIndex) != sinkIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("close sink index: %{public}d name %{public}s",
                audioHandleIndex,
                sinkIdSinkNameMap_[audioHandleIndex].c_str());
            ret = CloseOutAudioPort(sinkIdSinkNameMap_[audioHandleIndex]);
        } else {
            AUDIO_INFO_LOG("close source index: %{public}d name %{public}s",
                audioHandleIndex,
                sourceIdSourceNameMap_[audioHandleIndex].c_str());
            ret = CloseInAudioPort(sourceIdSourceNameMap_[audioHandleIndex]);
        }
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnCloseAudioPortCb(ret);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetDefaultSink(std::string name)
{
    CHECK_AND_RETURN_RET_LOG(!name.empty(), ERROR_INVALID_PARAM, "invalid sink name");
    AUDIO_INFO_LOG("HpaeManager::SetDefaultSink name: %{public}s", name.c_str());
    auto request = [this, name]() {
        AUDIO_INFO_LOG("SetDefaultSink name: %{public}s", name.c_str());
        if (name == defaultSink_) {
            AUDIO_INFO_LOG("sink is same as default sink");
            return;
        }
        if (!SafeGetMap(rendererManagerMap_, name)) {
            AUDIO_WARNING_LOG("sink: %{public}s not exist, do not change default sink", name.c_str());
            return;
        }
        std::shared_ptr<IHpaeRendererManager> rendererManager = GetRendererManagerByName(defaultSink_);
        if (rendererManager == nullptr) {
            AUDIO_INFO_LOG("default sink not exist, set default sink direct");
            defaultSink_ = name;
            return;
        }
        std::vector<uint32_t> sessionIds;
        rendererManager->MoveAllStream(name, sessionIds, MOVE_ALL);
        std::string oldDefaultSink = defaultSink_;
        defaultSink_ = name;
        if (!rendererManager->IsInit()) {
            rendererManagerMap_.erase(oldDefaultSink);
            sinkIdSinkNameMap_.erase(sinkNameSinkIdMap_[oldDefaultSink]);
            sinkNameSinkIdMap_.erase(oldDefaultSink);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetDefaultSource(std::string name)
{
    CHECK_AND_RETURN_RET_LOG(!name.empty(), ERROR_INVALID_PARAM, "invalid source name");
    auto request = [this, name]() {
        if (name == defaultSource_) {
            AUDIO_INFO_LOG("source is same as default source");
            return;
        }
        if (!SafeGetMap(capturerManagerMap_, name)) {
            AUDIO_WARNING_LOG("source: %{public}s not exist, do not change default source", name.c_str());
            return;
        }
        std::shared_ptr<IHpaeCapturerManager> capturerManager = GetCapturerManagerByName(defaultSource_);
        if (capturerManager == nullptr) {
            AUDIO_INFO_LOG("default source not exist, set default source direct");
            defaultSource_ = name;
            return;
        }
        std::vector<uint32_t> sessionIds;
        capturerManager->MoveAllStream(name, sessionIds, MOVE_ALL);
        std::string oldDefaultSource_ = defaultSource_;
        defaultSource_ = name;
        if (!capturerManager->IsInit()) {
            capturerManagerMap_.erase(oldDefaultSource_);
            sourceIdSourceNameMap_.erase(sourceNameSourceIdMap_[oldDefaultSource_]);
            sourceNameSourceIdMap_.erase(oldDefaultSource_);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAllSinkInputs()
{
    AUDIO_INFO_LOG("GetAllSinkInputs");
    auto request = [this]() {
        std::vector<SinkInput> results;
        std::transform(sinkInputs_.begin(), sinkInputs_.end(), std::back_inserter(results), [](const auto &pair) {
            return pair.second;
        });
        AUDIO_INFO_LOG("sink input number:%{public}zu", results.size());
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnGetAllSinkInputsCb(SUCCESS, results);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::MoveToPreferSink(const std::string &name, std::shared_ptr<AudioServiceHpaeCallback> serviceCallback)
{
    AUDIO_INFO_LOG("enter in");
    std::vector<uint32_t> sessionIds;
    for (const auto &id : idPreferSinkNameMap_) {
        if (id.second == name && rendererIdStreamInfoMap_[id.first].state == HPAE_SESSION_RUNNING &&
            rendererIdSinkNameMap_[id.first] != id.second && rendererIdSinkNameMap_[id.first] == defaultSink_) {
            sessionIds.emplace_back(id.first);
            movingIds_.emplace(id.first, HPAE_SESSION_RUNNING);
        }
    }
    if (sessionIds.size() == 0) {
        serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[name]);
        return;
    }

    auto request = [this, name, sessionIds, serviceCallback]() {
        AUDIO_INFO_LOG("Move %{public}s To Prefer Sink: %{public}s", defaultSink_.c_str(), name.c_str());
        if (!SafeGetMap(rendererManagerMap_, defaultSink_)) {
            AUDIO_ERR_LOG("can not find default sink: %{public}s", defaultSink_.c_str());
            serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[name]);
            return;
        }
        rendererManagerMap_[defaultSink_]->MoveAllStream(name, sessionIds, MOVE_PREFER);
    };
    SendRequest(request, __func__);
}

int32_t HpaeManager::GetAllSourceOutputs()
{
    AUDIO_INFO_LOG("GetAllSourceOutputs");
    auto request = [this]() {
        std::vector<SourceOutput> results;
        std::transform(sourceOutputs_.begin(), sourceOutputs_.end(), std::back_inserter(results), [](const auto &pair) {
            return pair.second;
        });
        AUDIO_INFO_LOG("source output number:%{public}zu", results.size());
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnGetAllSourceOutputsCb(SUCCESS, results);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::MoveSourceOutputByIndexOrName(
    uint32_t sourceOutputId, uint32_t sourceIndex, std::string sourceName)
{
    auto request = [this, sourceOutputId, sourceName]() {
        bool isReturn = false;
        int32_t errorCode = ERROR_INVALID_PARAM;
        AUDIO_INFO_LOG("move session:%{public}d, source name:%{public}s", sourceOutputId, sourceName.c_str());
        if (sourceName.empty()) {
            AUDIO_ERR_LOG("move session:%{public}u failed,source name is empty.", sourceOutputId);
            isReturn = true;
        }
        std::shared_ptr<IHpaeCapturerManager> capturerManager = GetCapturerManagerByName(sourceName);
        if (capturerManager == nullptr || !capturerManager->IsInit()) {
            AUDIO_ERR_LOG("move session:%{public}u failed, can not find source:%{public}s or source is not open.",
                sourceOutputId, sourceName.c_str());
            isReturn = true;
        }
        std::shared_ptr<IHpaeCapturerManager> oldCaptureManager = GetCapturerManagerById(sourceOutputId);
        if (oldCaptureManager == nullptr) {
            AUDIO_ERR_LOG("move session:%{public}u failed,can not find source.", sourceOutputId);
            isReturn = true;
        }
        std::string name = capturerIdSourceNameMap_[sourceOutputId];
        if (sourceName == name) {
            AUDIO_INFO_LOG("move session:%{public}u,source:%{public}s is the same, no need move",
                sourceOutputId, sourceName.c_str());
            isReturn = true;
            errorCode = SUCCESS;
        }
        if (capturerIdStreamInfoMap_.find(sourceOutputId) == capturerIdStreamInfoMap_.end()) {
            AUDIO_ERR_LOG("move session:%{public}u failed,can not find session.", sourceOutputId);
            isReturn = true;
        }
        if (!capturerIdStreamInfoMap_[sourceOutputId].streamInfo.isMoveAble) {
            AUDIO_ERR_LOG("move session:%{public}u failed,session is not moveable.", sourceOutputId);
            isReturn = true;
        }
        if (isReturn) {
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->OnMoveSourceOutputByIndexOrNameCb(errorCode);
            }
            return;
        }
        AUDIO_INFO_LOG("start move session:%{public}u, [%{public}s] --> [%{public}s], state:%{public}d",
            sourceOutputId, name.c_str(), sourceName.c_str(), capturerIdStreamInfoMap_[sourceOutputId].state);
        movingIds_.emplace(sourceOutputId, capturerIdStreamInfoMap_[sourceOutputId].state);
        oldCaptureManager->MoveStream(sourceOutputId, sourceName);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::MoveSinkInputByIndexOrName(uint32_t sinkInputId, uint32_t sinkIndex, std::string sinkName)
{
    auto request = [this, sinkInputId, sinkName]() {
        bool isReturn = false;
        int32_t errorCode = ERROR_INVALID_PARAM;
        if (sinkName.empty()) {
            AUDIO_ERR_LOG("move session:%{public}u failed,sink name is empty.", sinkInputId);
            isReturn = true;
        }

        std::shared_ptr<IHpaeRendererManager> rendererManager = GetRendererManagerByName(sinkName);
        if (rendererManager == nullptr || !rendererManager->IsInit()) {
            AUDIO_ERR_LOG("move session:%{public}u failed, can not find sink:%{public}s or sink is not open.",
                sinkInputId, sinkName.c_str());
            isReturn = true;
        }

        std::shared_ptr<IHpaeRendererManager> oldRendererManager = GetRendererManagerById(sinkInputId);
        if (oldRendererManager == nullptr) {
            AUDIO_ERR_LOG("move session:%{public}u failed,can not find sink", sinkInputId);
            isReturn = true;
        }

        std::string name = rendererIdSinkNameMap_[sinkInputId];
        if (sinkName == name) {
            AUDIO_INFO_LOG("sink:%{public}s is the same, no need move session:%{public}u", sinkName.c_str(),
                sinkInputId);
            isReturn = true;
            errorCode = SUCCESS;
        }

        if (rendererIdStreamInfoMap_.find(sinkInputId) == rendererIdStreamInfoMap_.end()) {
            AUDIO_ERR_LOG("move session:%{public}u failed,can not find session", sinkInputId);
            isReturn = true;
        }

        if (!rendererIdStreamInfoMap_[sinkInputId].streamInfo.isMoveAble) {
            AUDIO_ERR_LOG("move session:%{public}u failed,session is not moveable.", sinkInputId);
            isReturn = true;
        }

        if (isReturn) {
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->OnMoveSinkInputByIndexOrNameCb(errorCode);
            }
            return;
        }

        AUDIO_INFO_LOG("start move session:%{public}u, [%{public}s] --> [%{public}s],state:%{public}d",
            sinkInputId, name.c_str(), sinkName.c_str(), rendererIdStreamInfoMap_[sinkInputId].state);
        movingIds_.emplace(sinkInputId, rendererIdStreamInfoMap_[sinkInputId].state);
        oldRendererManager->MoveStream(sinkInputId, sinkName);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::HandleMsg()
{
    hpaeNoLockQueue_.HandleRequests();
}

bool HpaeManager::IsInit()
{
    return isInit_.load();
}

bool HpaeManager::IsRunning()
{
    if (hpaeManagerThread_ == nullptr) {
        return false;
    }
    return hpaeManagerThread_->IsRunning();
}

bool HpaeManager::IsMsgProcessing()
{
    return !hpaeNoLockQueue_.IsFinishProcess();
}

int32_t HpaeManager::GetMsgCount()
{
    return receiveMsgCount_.load();
}

void HpaeManager::Invoke(HpaeMsgCode cmdID, const std::any &args)
{
    auto it = handlers_.find(cmdID);
    if (it != handlers_.end()) {
        auto request = [it, args]() { it->second(args); };
        SendRequest(request, __func__);
        return;
    };
    AUDIO_ERR_LOG("HpaeManager::Invoke cmdID %{public}d not found", (int32_t)cmdID);
}

template <typename... Args>
void HpaeManager::RegisterHandler(HpaeMsgCode cmdID, void (HpaeManager::*func)(Args...))
{
    handlers_[cmdID] = [this, cmdID, func](const std::any &packedArgs) {
        // unpack args
        auto args = std::any_cast<std::tuple<Args...>>(&packedArgs);
        // print log if args parse error
        CHECK_AND_RETURN_LOG(args != nullptr, "cmdId %{public}d type mismatched", cmdID);
        std::apply(
            [this, func](
                auto &&...unpackedArgs) { (this->*func)(std::forward<decltype(unpackedArgs)>(unpackedArgs)...); },
            *args);
    };
}

bool HpaeManager::MovingSinkStateChange(uint32_t sessionId, const std::shared_ptr<HpaeSinkInputNode>& sinkInput)
{
    if (movingIds_.find(sessionId) != movingIds_.end()) {
        if (movingIds_[sessionId] == HPAE_SESSION_RELEASED) {
            rendererIdSinkNameMap_.erase(sessionId);
            rendererIdStreamInfoMap_.erase(sessionId);
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->OnMoveSinkInputByIndexOrNameCb(SUCCESS);
            }
            movingIds_.erase(sessionId);
            return true;
        }
        if (movingIds_[sessionId] != rendererIdStreamInfoMap_[sessionId].state) {
            sinkInput->SetState(movingIds_[sessionId]);
        }
        sinkInput->SetOffloadEnabled(rendererIdStreamInfoMap_[sessionId].offloadEnable);
        movingIds_.erase(sessionId);
    }
    return false;
}

void HpaeManager::HandleMoveSinkInput(const std::shared_ptr<HpaeSinkInputNode> sinkInputNode, std::string sinkName)
{
    uint32_t sessionId = sinkInputNode->GetNodeInfo().sessionId;
    AUDIO_INFO_LOG("handle move session:%{public}u to new sink:%{public}s", sessionId, sinkName.c_str());
    if (MovingSinkStateChange(sessionId, sinkInputNode)) {
        return;
    }

    std::shared_ptr<IHpaeRendererManager> rendererManager = GetRendererManagerByName(sinkName);
    if (rendererManager == nullptr) {
        AUDIO_ERR_LOG("handle move session:%{public}u failed,can not find sink by name:%{public}s",
            sessionId, sinkName.c_str());
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnMoveSinkInputByIndexOrNameCb(ERROR_INVALID_PARAM);
        }
        return;
    }
    
    rendererManager->AddNodeToSink(sinkInputNode);
    rendererIdSinkNameMap_[sessionId] = sinkName;
    if (sinkName != defaultSink_) {
        idPreferSinkNameMap_[sessionId] = sinkName;
    }
    if (sinkInputs_.find(sessionId) != sinkInputs_.end()) {
        sinkInputs_[sessionId].deviceSinkId = sinkNameSinkIdMap_[sinkName];
        sinkInputs_[sessionId].sinkName = sinkName;
    }
    if (auto serviceCallback = serviceCallback_.lock()) {
        serviceCallback->OnMoveSinkInputByIndexOrNameCb(SUCCESS);
    }
}

void HpaeManager::HandleMoveSourceOutput(HpaeCaptureMoveInfo moveInfo, std::string sourceName)
{
    uint32_t sessionId = moveInfo.sessionId;
    AUDIO_INFO_LOG("handle move session:%{public}u to new source:%{public}s", sessionId, sourceName.c_str());
    if (movingIds_.find(sessionId) != movingIds_.end()) {
        if (movingIds_[sessionId] == HPAE_SESSION_RELEASED) {
            capturerIdSourceNameMap_.erase(sessionId);
            capturerIdStreamInfoMap_.erase(sessionId);
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->OnMoveSourceOutputByIndexOrNameCb(SUCCESS);
            }
            movingIds_.erase(sessionId);
            return;
        }
        if (movingIds_[sessionId] != capturerIdStreamInfoMap_[sessionId].state) {
            moveInfo.sessionInfo.state = movingIds_[sessionId];
        }
        movingIds_.erase(sessionId);
    }

    std::shared_ptr<IHpaeCapturerManager> catpureManager = GetCapturerManagerByName(sourceName);
    if (catpureManager == nullptr) {
        AUDIO_ERR_LOG("handle move session:%{public}u failed,can not find source by name:%{public}s",
            sessionId, sourceName.c_str());
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnMoveSourceOutputByIndexOrNameCb(ERROR_INVALID_PARAM);
        }
        return;
    }
    
    catpureManager->AddNodeToSource(moveInfo);
    capturerIdSourceNameMap_[sessionId] = sourceName;
    if (sourceOutputs_.find(sessionId) != sourceOutputs_.end()) {
        sourceOutputs_[sessionId].deviceSourceId = sourceNameSourceIdMap_[sourceName];
    }
    if (auto serviceCallback = serviceCallback_.lock()) {
        serviceCallback->OnMoveSourceOutputByIndexOrNameCb(SUCCESS);
    }
}

void HpaeManager::HandleMoveAllSinkInputs(
    std::vector<std::shared_ptr<HpaeSinkInputNode>> sinkInputs, std::string sinkName, MoveSessionType moveType)
{
    AUDIO_INFO_LOG("handle move session count:%{public}zu to name:%{public}s", sinkInputs.size(), sinkName.c_str());
    if (moveType == MOVE_PREFER) {
        for (auto it = sinkInputs.begin(); it != sinkInputs.end();) {
            CHECK_AND_CONTINUE_LOG(*it, "sinkInput is nullptr");
            uint32_t sessionId = (*it)->GetNodeInfo().sessionId;
            if (MovingSinkStateChange(sessionId, *it)) {
                sinkInputs.erase(it);
                continue;
            }
            it++;
        }
    }

    if (sinkName.empty()) {
        AUDIO_INFO_LOG("sink name is empty, move to default sink:%{public}s", defaultSink_.c_str());
        sinkName = defaultSink_;
    }

    if (!SafeGetMap(rendererManagerMap_, sinkName)) {
        AUDIO_WARNING_LOG("can not find sink: %{public}s", sinkName.c_str());
        if (moveType == MOVE_PREFER) {
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[sinkName]);
            }
        }
        return;
    }
    
    rendererManagerMap_[sinkName]->AddAllNodesToSink(sinkInputs, true);
    for (const auto &sinkInput : sinkInputs) {
        CHECK_AND_CONTINUE_LOG(sinkInput, "sinkInput is nullptr");
        uint32_t sessionId = sinkInput->GetNodeInfo().sessionId;
        rendererIdSinkNameMap_[sessionId] = sinkName;
        if (sinkInputs_.find(sessionId) != sinkInputs_.end()) {
            sinkInputs_[sessionId].deviceSinkId = sinkNameSinkIdMap_[sinkName];
            sinkInputs_[sessionId].sinkName = sinkName;
        }
    }
    if (moveType == MOVE_PREFER) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnOpenAudioPortCb(sinkNameSinkIdMap_[sinkName]);
        }
    }
}

void HpaeManager::HandleMoveAllSourceOutputs(const std::vector<HpaeCaptureMoveInfo> moveInfos, std::string sourceName)
{
    AUDIO_INFO_LOG("handle move session count:%{public}zu to name:%{public}s", moveInfos.size(), sourceName.c_str());
    if (sourceName.empty()) {
        AUDIO_INFO_LOG("source is empty, move to default source:%{public}s", defaultSource_.c_str());
        sourceName = defaultSource_;
    }
    if (!SafeGetMap(capturerManagerMap_, sourceName)) {
        AUDIO_WARNING_LOG("can not find source: %{public}s", sourceName.c_str());
        return;
    }
    capturerManagerMap_[sourceName]->AddAllNodesToSource(moveInfos, true);
    for (const auto &it : moveInfos) {
        capturerIdSourceNameMap_[it.sessionId] = sourceName;
        if (sourceOutputs_.find(it.sessionId) != sourceOutputs_.end()) {
            sourceOutputs_[it.sessionId].deviceSourceId = sourceNameSourceIdMap_[sourceName];
        }
    }
}

void HpaeManager::HandleMoveSessionFailed(HpaeStreamClassType streamClassType, uint32_t sessionId,
    MoveSessionType moveType, std::string name)
{
    AUDIO_INFO_LOG("handle move session:%{public}u failed to %{public}s", sessionId, name.c_str());
    movingIds_.erase(sessionId);
    if (moveType != MOVE_SINGLE) {
        return;
    }
    if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnMoveSinkInputByIndexOrNameCb(ERROR_INVALID_PARAM);
        }
    } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD) {
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnMoveSinkInputByIndexOrNameCb(ERROR_INVALID_PARAM);
        }
    }
}

void HpaeManager::HandleUpdateStatus(
    HpaeStreamClassType streamClassType, uint32_t sessionId, HpaeSessionState status, IOperation operation)
{
    AUDIO_INFO_LOG("HpaeManager::HandleUpdateStatus sessionid:%{public}u "
                   "status:%{public}d operation:%{public}d",
        sessionId,
        status,
        operation);
    if (operation == OPERATION_INVALID) {
        // maybe dosomething while move sink inputs
        return;
    }
    auto it = streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY ? rendererIdStreamInfoMap_.find(sessionId)
                                                             : capturerIdStreamInfoMap_.find(sessionId);
    if (it != rendererIdStreamInfoMap_.end() && it != capturerIdStreamInfoMap_.end()) {
        if (auto callback = it->second.statusCallback.lock()) {
            callback->OnStatusUpdate(operation);
        }
    }
}

void HpaeManager::HandleDumpSinkInfo(std::string deviceName, std::string dumpStr)
{
    AUDIO_INFO_LOG("HpaeManager::HandleDumpSinkInfo deviceName:%{public}s dumpStr:%{public}s",
        deviceName.c_str(),
        dumpStr.c_str());
    if (auto ptr = dumpCallback_.lock()) {
        ptr->OnDumpSinkInfoCb(dumpStr, SUCCESS);
    }
}

void HpaeManager::HandleDumpSourceInfo(std::string deviceName, std::string dumpStr)
{
    AUDIO_INFO_LOG("HpaeManager::HandleDumpSourceInfo deviceName:%{public}s dumpStr:%{public}s",
        deviceName.c_str(),
        dumpStr.c_str());
    if (auto ptr = dumpCallback_.lock()) {
        ptr->OnDumpSourceInfoCb(dumpStr, SUCCESS);
    }
}

void HpaeManager::HandleInitDeviceResult(std::string deviceName, int32_t result)
{
    AUDIO_INFO_LOG("deviceName:%{public}s result:%{public}d ", deviceName.c_str(), result);
    auto serviceCallback = serviceCallback_.lock();
    if (serviceCallback && result == SUCCESS) {
        if (sinkNameSinkIdMap_.find(deviceName) != sinkNameSinkIdMap_.end()) {
            MoveToPreferSink(deviceName, serviceCallback);
        } else if (sourceNameSourceIdMap_.find(deviceName) != sourceNameSourceIdMap_.end()) {
            serviceCallback->OnOpenAudioPortCb(sourceNameSourceIdMap_[deviceName]);
        } else {
            AUDIO_ERR_LOG("device:%{public}s is not exist.", deviceName.c_str());
            serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        }
    } else if (serviceCallback) {
        serviceCallback->OnOpenAudioPortCb(SINK_INVALID_ID);
        AUDIO_INFO_LOG("HandleInitDeviceResult deviceName:%{public}s "
                       "result:%{public}d error",
            deviceName.c_str(),
            result);
    } else {
        AUDIO_INFO_LOG("OnOpenAudioPortCb is nullptr");
    }
}

void HpaeManager::HandleDeInitDeviceResult(std::string deviceName, int32_t result)
{
    AUDIO_INFO_LOG("deviceName:%{public}s result:%{public}d ", deviceName.c_str(), result);
    auto serviceCallback = serviceCallback_.lock();
    if (serviceCallback && result == SUCCESS && sinkNameSinkIdMap_.find(deviceName) != sinkNameSinkIdMap_.end()) {
        serviceCallback->OnCloseAudioPortCb(sinkNameSinkIdMap_[deviceName]);
    } else if (serviceCallback) {
        serviceCallback->OnCloseAudioPortCb(ERROR);
        AUDIO_INFO_LOG("deviceName:%{public}s "
                       "result:%{public}d error",
            deviceName.c_str(),
            result);
    } else {
        AUDIO_INFO_LOG("HandleDeInitDeviceResult is nullptr");
    }
    if (SafeGetMap(rendererManagerMap_, deviceName)) {
        AUDIO_INFO_LOG("OnCloseAudioPortCb is sink");
        rendererManagerMap_[deviceName]->DeactivateThread();
        if (deviceName != defaultSink_) {
            rendererManagerMap_.erase(deviceName);
            sinkIdSinkNameMap_.erase(sinkNameSinkIdMap_[deviceName]);
            sinkNameSinkIdMap_.erase(deviceName);
        }
    } else if (SafeGetMap(capturerManagerMap_, deviceName)) {
        AUDIO_INFO_LOG("OnCloseAudioPortCb is source");
        capturerManagerMap_[deviceName]->DeactivateThread();
        if (deviceName != defaultSource_) {
            capturerManagerMap_.erase(deviceName);
            sourceIdSourceNameMap_.erase(sourceNameSourceIdMap_[deviceName]);
            sourceNameSourceIdMap_.erase(deviceName);
        }
    } else {
        AUDIO_INFO_LOG("deviceName:%{public}s  can not find", deviceName.c_str());
    }
}

void HpaeManager::SendRequest(Request &&request, std::string funcName)
{
    Trace trace("sendrequest::" + funcName);
    hpaeNoLockQueue_.PushRequest(std::move(request));
    CHECK_AND_RETURN_LOG(hpaeManagerThread_, "hpaeManagerThread_ is nullptr");
    hpaeManagerThread_->Notify();
}
// play and record stream interface
int32_t HpaeManager::CreateStream(const HpaeStreamInfo &streamInfo)
{
    auto request = [this, streamInfo]() {
        AUDIO_INFO_LOG("streamType is %{public}d sessionId %{public}u sourceType is %{public}d",
            streamInfo.streamType,
            streamInfo.sessionId,
            streamInfo.sourceType);
        if (INNER_SOURCE_TYPE_SET.count(streamInfo.sourceType) != 0) {
            return CreateStreamForCapInner(streamInfo);
        } else if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY) {
            std::string deviceName = streamInfo.deviceName == "" ? defaultSink_ : streamInfo.deviceName;
            AUDIO_INFO_LOG("devicename:%{public}s, sessionId:%{public}u", deviceName.c_str(), streamInfo.sessionId);
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, deviceName),
                "can not find sink[%{public}s] in rendererManagerMap_",
                deviceName.c_str());
            rendererIdSinkNameMap_[streamInfo.sessionId] = deviceName;
            rendererManagerMap_[deviceName]->CreateStream(streamInfo);
            rendererIdStreamInfoMap_[streamInfo.sessionId].streamInfo = streamInfo;
            rendererIdStreamInfoMap_[streamInfo.sessionId].state = HPAE_SESSION_NEW;
            AddStreamToCollection(streamInfo, deviceName);
        } else if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD) {
            std::string deviceName = streamInfo.deviceName == "" ? defaultSource_ : streamInfo.deviceName;
            AUDIO_INFO_LOG("source:%{public}s, sessionId:%{public}u", deviceName.c_str(), streamInfo.sessionId);
            CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, deviceName),
                "can not find source[%{public}s] in capturerManagerMap_",
                deviceName.c_str());
            capturerIdSourceNameMap_[streamInfo.sessionId] = deviceName;
            capturerManagerMap_[deviceName]->CreateStream(streamInfo);
            capturerIdStreamInfoMap_[streamInfo.sessionId].streamInfo = streamInfo;
            capturerIdStreamInfoMap_[streamInfo.sessionId].state = HPAE_SESSION_NEW;
            AddStreamToCollection(streamInfo, deviceName);
        } else {
            AUDIO_WARNING_LOG(
                "can not find default sink or source streamClassType %{public}d", streamInfo.streamClassType);
        }
    };
    SendRequest(request, __func__);
    AUDIO_INFO_LOG("defaultSink_ is %{public}s defaultSource_ is %{public}s streamClassType %{public}u",
        defaultSink_.c_str(), defaultSource_.c_str(), streamInfo.streamClassType);
    return SUCCESS;
}

void HpaeManager::AddStreamToCollection(const HpaeStreamInfo &streamInfo, const std::string &name)
{
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch());
    if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY) {
        SinkInput sinkInput;
        sinkInput.streamId = streamInfo.sessionId;
        sinkInput.paStreamId = streamInfo.sessionId;
        sinkInput.streamType = streamInfo.streamType;
        sinkInput.sinkName = name;
        sinkInput.deviceSinkId = sinkNameSinkIdMap_[name];
        sinkInput.pid = streamInfo.pid;
        sinkInput.uid = streamInfo.uid;
        sinkInput.startTime = static_cast<uint64_t>(ms.count());
        sinkInputs_[streamInfo.sessionId] = sinkInput;
    } else if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD) {
        SourceOutput sourceOutputInfo;
        sourceOutputInfo.streamId = streamInfo.sessionId;
        sourceOutputInfo.paStreamId = streamInfo.sessionId;
        sourceOutputInfo.streamType = streamInfo.streamType;
        sourceOutputInfo.deviceSourceId = sourceNameSourceIdMap_[name];
        sourceOutputInfo.pid = streamInfo.pid;
        sourceOutputInfo.uid = streamInfo.uid;
        sourceOutputInfo.startTime = static_cast<uint64_t>(ms.count());
        sourceOutputs_[streamInfo.sessionId] = sourceOutputInfo;
    }
}

void HpaeManager::DestroyCapture(uint32_t sessionId)
{
    if (capturerIdSourceNameMap_.find(sessionId) == capturerIdSourceNameMap_.end()) {
        AUDIO_WARNING_LOG("can not find capture by id:%{public}u", sessionId);
        return;
    }
    std::string captureName = capturerIdSourceNameMap_[sessionId];
    if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
        std::shared_ptr<IHpaeRendererManager> renderManager = GetRendererManagerByName(captureName);
        if (renderManager != nullptr) {
            renderManager->DestroyStream(sessionId);
        }
    } else {
        std::shared_ptr<IHpaeCapturerManager> capManager = GetCapturerManagerByName(captureName);
        if (capManager != nullptr) {
            capManager->DestroyStream(sessionId);
        }
    }
}

bool HpaeManager::SetMovingStreamState(HpaeStreamClassType streamType, uint32_t sessionId,
        HpaeSessionState status, HpaeSessionState state, IOperation operation)
{
    if (movingIds_.find(sessionId) == movingIds_.end()) {
        return false;
    }
    AUDIO_INFO_LOG("sessionId:%{public}u is moving", sessionId);
    if (operation != OPERATION_FLUSHED && operation != OPERATION_DRAINED) {
        movingIds_[sessionId] = status;
        if (streamType == HPAE_STREAM_CLASS_TYPE_PLAY) {
            rendererIdStreamInfoMap_[sessionId].state = state;
        } else {
            capturerIdStreamInfoMap_[sessionId].state = state;
        }
    }
    if (streamType == HPAE_STREAM_CLASS_TYPE_PLAY) {
        if (auto statusCallback = rendererIdStreamInfoMap_[sessionId].statusCallback.lock()) {
            statusCallback->OnStatusUpdate(operation);
        }
        if (operation == OPERATION_RELEASED) {
            sinkInputs_.erase(sessionId);
            idPreferSinkNameMap_.erase(sessionId);
        }
    } else {
        if (auto statusCallback = capturerIdStreamInfoMap_[sessionId].statusCallback.lock()) {
            statusCallback->OnStatusUpdate(operation);
        }
        if (operation == OPERATION_RELEASED) {
            sourceOutputs_.erase(sessionId);
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->HandleSourceAudioStreamRemoved(sessionId);
            }
        } else if (operation == OPERATION_STARTED) {
            if (capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType == SOURCE_TYPE_LIVE &&
                (effectLiveState_ == "NROFF" || effectLiveState_ == "NRON")) {
                const std::string combinedParam = "live_effect=" + effectLiveState_;
                HpaePolicyManager::GetInstance().SetAudioParameter("primary",
                    AudioParamKey::PARAM_KEY_STATE, "", combinedParam);
            }
        }
    }
    return true;
}

int32_t HpaeManager::DestroyStream(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        AUDIO_INFO_LOG("DestroyStream streamClassType %{public}d, sessionId %{public}u", streamClassType, sessionId);
        if (SetMovingStreamState(streamClassType, sessionId, HPAE_SESSION_RELEASED,
            HPAE_SESSION_RELEASED, OPERATION_RELEASED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY) {
            std::shared_ptr<IHpaeRendererManager> renderManager = GetRendererManagerById(sessionId);
            if (renderManager!= nullptr) {
                renderManager->DestroyStream(sessionId);
            }
            rendererIdSinkNameMap_.erase(sessionId);
            rendererIdStreamInfoMap_.erase(sessionId);
            sinkInputs_.erase(sessionId);
            idPreferSinkNameMap_.erase(sessionId);
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD) {
            DestroyCapture(sessionId);
            capturerIdSourceNameMap_.erase(sessionId);
            capturerIdStreamInfoMap_.erase(sessionId);
            sourceOutputs_.erase(sessionId);
            if (auto serviceCallback = serviceCallback_.lock()) {
                serviceCallback->HandleSourceAudioStreamRemoved(sessionId);
            }
        } else {
            AUDIO_WARNING_LOG(
                "can not find sessionId streamClassType  %{public}d, sessionId %{public}u", streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Start(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        if (rendererIdStreamInfoMap_[sessionId].state == HPAE_SESSION_RELEASED) {
            AUDIO_WARNING_LOG("Start session:%{public}u failed. session already released.", sessionId);
            return;
        }
        AUDIO_INFO_LOG(
            "HpaeManager::Start sessionId: %{public}u streamClassType:%{public}d", sessionId, streamClassType);
        if (SetMovingStreamState(streamClassType, sessionId, HPAE_SESSION_RUNNING,
            HPAE_SESSION_RUNNING, OPERATION_STARTED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer Start sessionId: %{public}u deviceName:%{public}s",
                sessionId, rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->Start(sessionId);
            rendererIdStreamInfoMap_[sessionId].state = HPAE_SESSION_RUNNING;
            rendererIdStreamInfoMap_[sessionId].statusCallback.lock()->OnStatusUpdate(OPERATION_STARTED);
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer Start sessionId: %{public}u deviceName:%{public}s",
                sessionId, capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->Start(sessionId);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->Start(sessionId);
            }
            capturerIdStreamInfoMap_[sessionId].state = HPAE_SESSION_RUNNING;
            if (capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType == SOURCE_TYPE_LIVE &&
                (effectLiveState_ == "NROFF" || effectLiveState_ == "NRON")) {
                const std::string combinedParam = "live_effect=" + effectLiveState_;
                HpaePolicyManager::GetInstance().SetAudioParameter("primary",
                    AudioParamKey::PARAM_KEY_STATE, "", combinedParam);
            }
        } else {
            AUDIO_WARNING_LOG("Start can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Pause(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        if (rendererIdStreamInfoMap_[sessionId].state == HPAE_SESSION_RELEASED) {
            AUDIO_WARNING_LOG("Pause session:%{public}u failed. session already released.", sessionId);
            return;
        }
        AUDIO_INFO_LOG(
            "HpaeManager::Pause sessionId: %{public}u streamClassType:%{public}d", sessionId, streamClassType);
        if (SetMovingStreamState(streamClassType, sessionId, HPAE_SESSION_PAUSED,
            HPAE_SESSION_PAUSING, OPERATION_PAUSED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer Pause sessionId: %{public}u deviceName:%{public}s",
                sessionId, rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->Pause(sessionId);
            rendererIdStreamInfoMap_[sessionId].state = HPAE_SESSION_PAUSING;
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer Pause sessionId: %{public}u deviceName:%{public}s",
                sessionId, capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->Pause(sessionId);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->Pause(sessionId);
            }
            capturerIdStreamInfoMap_[sessionId].state = HPAE_SESSION_PAUSING;
        } else {
            AUDIO_WARNING_LOG("Pause can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Flush(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        if (rendererIdStreamInfoMap_[sessionId].state == HPAE_SESSION_RELEASED) {
            AUDIO_WARNING_LOG("Flush session:%{public}u failed. session already released.", sessionId);
            return;
        }
        AUDIO_INFO_LOG(
            "HpaeManager::Flush sessionId: %{public}u streamClassType:%{public}d", sessionId, streamClassType);
        if (SetMovingStreamState(streamClassType, sessionId,
            HPAE_SESSION_INVALID, HPAE_SESSION_INVALID, OPERATION_FLUSHED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer Flush sessionId: %{public}u deviceName:%{public}s",
                sessionId, rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->Flush(sessionId);
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer Flush sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->Flush(sessionId);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->Flush(sessionId);
            }
        } else {
            AUDIO_WARNING_LOG("Flush can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Drain(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        if (rendererIdStreamInfoMap_[sessionId].state == HPAE_SESSION_RELEASED) {
            AUDIO_WARNING_LOG("Drain session:%{public}u failed. session already released.", sessionId);
            return;
        }
        AUDIO_INFO_LOG(
            "HpaeManager::Drain sessionId: %{public}u streamClassType:%{public}d", sessionId, streamClassType);
        if (SetMovingStreamState(streamClassType, sessionId,
            HPAE_SESSION_INVALID, HPAE_SESSION_INVALID, OPERATION_DRAINED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer Drain sessionId: %{public}u deviceName:%{public}s",
                sessionId, rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->Drain(sessionId);
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer Drain sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->Drain(sessionId);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->Drain(sessionId);
            }
        } else {
            AUDIO_WARNING_LOG("Drain can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Stop(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    auto request = [this, streamClassType, sessionId]() {
        if (rendererIdStreamInfoMap_[sessionId].state == HPAE_SESSION_RELEASED) {
            AUDIO_WARNING_LOG("Stop session:%{public}u failed. session already released.", sessionId);
            return;
        }
        AUDIO_INFO_LOG(
            "HpaeManager::Stop sessionId: %{public}u streamClassType:%{public}d", sessionId, streamClassType);
        if (SetMovingStreamState(streamClassType, sessionId, HPAE_SESSION_STOPPED,
            HPAE_SESSION_STOPPING, OPERATION_STOPPED)) {
            return;
        }
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer Stop sessionId: %{public}u deviceName:%{public}s",
                sessionId, rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->Stop(sessionId);
            rendererIdStreamInfoMap_[sessionId].state = HPAE_SESSION_STOPPING;
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer Stop sessionId: %{public}u deviceName:%{public}s",
                sessionId, capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->Stop(sessionId);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->Stop(sessionId);
            }
            capturerIdStreamInfoMap_[sessionId].state = HPAE_SESSION_STOPPING;
        } else {
            AUDIO_WARNING_LOG("Stop can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType, sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::Release(HpaeStreamClassType streamClassType, uint32_t sessionId)
{
    DestroyStream(streamClassType, sessionId);
    return SUCCESS;
}

int32_t HpaeManager::RegisterStatusCallback(
    HpaeStreamClassType streamClassType, uint32_t sessionId, const std::weak_ptr<IStatusCallback> &callback)
{
    auto request = [this, streamClassType, sessionId, callback]() {
        AUDIO_INFO_LOG(
            "RegisterStatusCallback streamClassType %{public}d, sessionId %{public}u", streamClassType, sessionId);
        if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
            rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer RegisterStatusCallback sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                rendererIdSinkNameMap_[sessionId].c_str());
            rendererIdStreamInfoMap_[sessionId].statusCallback = callback;
        } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
                   capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer RegisterStatusCallback sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                capturerIdSourceNameMap_[sessionId].c_str());
            capturerIdStreamInfoMap_[sessionId].statusCallback = callback;
        } else {
            AUDIO_WARNING_LOG(
                "RegisterStatusCallback can not find sessionId streamClassType  %{public}d, sessionId %{public}u",
                streamClassType,
                sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

// record stream interface
int32_t HpaeManager::RegisterReadCallback(uint32_t sessionId, const std::weak_ptr<ICapturerStreamCallback> &callback)
{
    auto request = [this, sessionId, callback]() {
        AUDIO_INFO_LOG("RegisterReadCallback sessionId %{public}u", sessionId);
        if (capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
            AUDIO_INFO_LOG("capturer RegisterReadCallback sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                capturerIdSourceNameMap_[sessionId].c_str());
            if (INNER_SOURCE_TYPE_SET.count(capturerIdStreamInfoMap_[sessionId].streamInfo.sourceType) != 0) {
                CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                rendererManagerMap_[capturerIdSourceNameMap_[sessionId]]->RegisterReadCallback(sessionId, callback);
            } else {
                CHECK_AND_RETURN_LOG(SafeGetMap(capturerManagerMap_, capturerIdSourceNameMap_[sessionId]),
                    "cannot find device:%{public}s", capturerIdSourceNameMap_[sessionId].c_str());
                capturerManagerMap_[capturerIdSourceNameMap_[sessionId]]->RegisterReadCallback(sessionId, callback);
            }
        } else {
            AUDIO_WARNING_LOG("RegisterReadCallback can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetSourceOutputInfo(uint32_t sessionId, HpaeStreamInfo &streamInfo)
{
    // to do
    return SUCCESS;
}

// play stream interface
int32_t HpaeManager::SetClientVolume(uint32_t sessionId, float volume)
{
    auto request = [this, sessionId, volume]() {
        AUDIO_INFO_LOG("SetClientVolume sessionId %{public}u %{public}f", sessionId, volume);
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->SetClientVolume(sessionId, volume);
        } else {
            AUDIO_WARNING_LOG("SetClientVolume can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetRate(uint32_t sessionId, int32_t rate)
{
    auto request = [this, sessionId, rate]() {
        AUDIO_INFO_LOG("SetRate sessionId %{public}u %{public}d", sessionId, rate);
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->SetRate(sessionId, rate);
        } else {
            AUDIO_WARNING_LOG("SetRate can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetAudioEffectMode(uint32_t sessionId, int32_t effectMode)
{
    auto request = [this, sessionId, effectMode]() {
        AUDIO_INFO_LOG("SetAudioEffectMode sessionId %{public}u %{public}d", sessionId, effectMode);
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->SetAudioEffectMode(sessionId, effectMode);
        } else {
            AUDIO_WARNING_LOG("SetAudioEffectMode can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAudioEffectMode(uint32_t sessionId, int32_t &effectMode)
{
    return SUCCESS;
}

int32_t HpaeManager::SetPrivacyType(uint32_t sessionId, int32_t privacyType)
{
    auto request = [this, sessionId, privacyType]() {
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->SetPrivacyType(sessionId, privacyType);
        } else {
            AUDIO_WARNING_LOG("SetPrivacyType can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetPrivacyType(uint32_t sessionId, int32_t &privacyType)
{
    return SUCCESS;
}

int32_t HpaeManager::RegisterWriteCallback(uint32_t sessionId, const std::weak_ptr<IStreamCallback> &callback)
{
    auto request = [this, sessionId, callback]() {
        AUDIO_INFO_LOG("RegisterWriteCallback sessionId %{public}u", sessionId);
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            AUDIO_INFO_LOG("renderer RegisterWriteCallback sessionId: %{public}u deviceName:%{public}s",
                sessionId,
                rendererIdSinkNameMap_[sessionId].c_str());
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->RegisterWriteCallback(sessionId, callback);
        } else {
            AUDIO_WARNING_LOG("RegisterWriteCallback can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetOffloadPolicy(uint32_t sessionId, int32_t state)
{
    auto request = [this, sessionId, state]() {
        AUDIO_INFO_LOG("SetOffloadPolicy sessionId %{public}u %{public}d", sessionId, state);
        if (rendererIdStreamInfoMap_.find(sessionId) != rendererIdStreamInfoMap_.end()) {
            rendererIdStreamInfoMap_[sessionId].offloadType = state;
            rendererIdStreamInfoMap_[sessionId].offloadEnable = state != OFFLOAD_DEFAULT;
        } else {
            AUDIO_WARNING_LOG("rendererIdStreamInfoMap_ can not find sessionId %{public}u", sessionId);
        }
        if (movingIds_.find(sessionId) != movingIds_.end()) { return ; }
        auto rendererManager = GetRendererManagerById(sessionId);
        if (rendererManager != nullptr) {
            rendererManager->SetOffloadPolicy(sessionId, state);
        } else {
            AUDIO_WARNING_LOG("SetOffloadPolicy can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

size_t HpaeManager::GetWritableSize(uint32_t sessionId)
{
    return SUCCESS;
}

int32_t HpaeManager::UpdateSpatializationState(uint32_t sessionId, bool spatializationEnabled, bool headTrackingEnabled)
{
    auto request = [this, sessionId, spatializationEnabled, headTrackingEnabled]() {
        AUDIO_INFO_LOG("UpdateSpatializationState sessionId %{public}u spatializationEnabled %{public}d "
                       "headTrackingEnabled %{public}d",
            sessionId,
            spatializationEnabled,
            headTrackingEnabled);
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->UpdateSpatializationState(
                sessionId, spatializationEnabled, headTrackingEnabled);
        } else {
            AUDIO_WARNING_LOG("UpdateSpatializationState can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::UpdateMaxLength(uint32_t sessionId, uint32_t maxLength)
{
    auto request = [this, sessionId, maxLength]() {
        if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
            CHECK_AND_RETURN_LOG(SafeGetMap(rendererManagerMap_, rendererIdSinkNameMap_[sessionId]),
                "cannot find device:%{public}s", rendererIdSinkNameMap_[sessionId].c_str());
            rendererManagerMap_[rendererIdSinkNameMap_[sessionId]]->UpdateMaxLength(sessionId, maxLength);
        } else {
            AUDIO_WARNING_LOG("UpdateMaxLength can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetOffloadRenderCallbackType(uint32_t sessionId, int32_t type)
{
    auto request = [this, sessionId, type]() {
        AUDIO_INFO_LOG("SetOffloadRenderCallbackType sessionId %{public}u %{public}d", sessionId, type);
        auto rendererManager = GetRendererManagerById(sessionId);
        if (rendererManager != nullptr) {
            rendererManager->SetOffloadRenderCallbackType(sessionId, type);
        } else {
            AUDIO_WARNING_LOG("SetOffloadRenderCallbackType can not find sessionId, sessionId %{public}u", sessionId);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

// only interface for unit test
int32_t HpaeManager::GetSessionInfo(
    HpaeStreamClassType streamClassType, uint32_t sessionId, HpaeSessionInfo &sessionInfo)
{
    if (streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY &&
        rendererIdStreamInfoMap_.find(sessionId) != rendererIdStreamInfoMap_.end()) {
        sessionInfo = rendererIdStreamInfoMap_[sessionId];
    } else if (streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD &&
               capturerIdStreamInfoMap_.find(sessionId) != capturerIdStreamInfoMap_.end()) {
        sessionInfo = capturerIdStreamInfoMap_[sessionId];
    } else {
        return ERROR;
    }
    return SUCCESS;
}

std::shared_ptr<IHpaeRendererManager> HpaeManager::GetRendererManagerByName(const std::string &sinkName)
{
    if (!SafeGetMap(rendererManagerMap_, sinkName)) {
        AUDIO_WARNING_LOG("can not find sinkName: %{public}s ", sinkName.c_str());
        return nullptr;
    }
    return rendererManagerMap_[sinkName];
}

std::shared_ptr<IHpaeCapturerManager> HpaeManager::GetCapturerManagerByName(const std::string &sourceName)
{
    if (!SafeGetMap(capturerManagerMap_, sourceName)) {
        AUDIO_WARNING_LOG("can not find sourceName: %{public}s ", sourceName.c_str());
        return nullptr;
    }
    return capturerManagerMap_[sourceName];
}

std::shared_ptr<IHpaeRendererManager> HpaeManager::GetRendererManagerById(uint32_t sessionId)
{
    if (rendererIdSinkNameMap_.find(sessionId) != rendererIdSinkNameMap_.end()) {
        return GetRendererManagerByName(rendererIdSinkNameMap_[sessionId]);
    }
    AUDIO_WARNING_LOG("can not find renderer by sessionId: %{public}u", sessionId);
    return nullptr;
}

std::shared_ptr<IHpaeCapturerManager> HpaeManager::GetCapturerManagerById(uint32_t sessionId)
{
    if (capturerIdSourceNameMap_.find(sessionId) != capturerIdSourceNameMap_.end()) {
        return GetCapturerManagerByName(capturerIdSourceNameMap_[sessionId]);
    }
    AUDIO_WARNING_LOG("can not find capture by sessionId: %{public}u", sessionId);
    return nullptr;
}

void HpaeManager::InitAudioEffectChainManager(const std::vector<EffectChain> &effectChains,
    const EffectChainManagerParam &effectChainManagerParam,
    const std::vector<std::shared_ptr<AudioEffectLibEntry>> &effectLibraryList)
{
    auto request = [effectChains, effectChainManagerParam, effectLibraryList]() {
        HpaePolicyManager::GetInstance().InitAudioEffectChainManager(effectChains,
            effectChainManagerParam, effectLibraryList);
    };
    SendRequest(request, __func__);
}

void HpaeManager::SetOutputDeviceSink(int32_t device, const std::string &sinkName)
{
    auto request = [device, sinkName]() {
        HpaePolicyManager::GetInstance().SetOutputDeviceSink(device, sinkName);
    };
    SendRequest(request, __func__);
}

int32_t HpaeManager::UpdateSpatializationState(AudioSpatializationState spatializationState)
{
    auto request = [spatializationState]() {
        HpaePolicyManager::GetInstance().UpdateSpatializationState(spatializationState);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::UpdateSpatialDeviceType(AudioSpatialDeviceType spatialDeviceType)
{
    auto request = [spatialDeviceType]() {
        HpaePolicyManager::GetInstance().UpdateSpatialDeviceType(spatialDeviceType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetSpatializationSceneType(AudioSpatializationSceneType spatializationSceneType)
{
    auto request = [spatializationSceneType]() {
        HpaePolicyManager::GetInstance().SetSpatializationSceneType(spatializationSceneType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::EffectRotationUpdate(const uint32_t rotationState)
{
    auto request = [rotationState]() {
        HpaePolicyManager::GetInstance().EffectRotationUpdate(rotationState);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetEffectSystemVolume(const int32_t systemVolumeType, const float systemVolume)
{
    auto request = [systemVolumeType, systemVolume]() {
        HpaePolicyManager::GetInstance().SetEffectSystemVolume(systemVolumeType, systemVolume);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetAudioEffectProperty(const AudioEffectPropertyArrayV3 &propertyArray)
{
    auto request = [propertyArray]() {
        HpaePolicyManager::GetInstance().SetAudioEffectProperty(propertyArray);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray)
{
    auto request = [this, &propertyArray]() {
        HpaePolicyManager::GetInstance().GetAudioEffectProperty(propertyArray);
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnGetAudioEffectPropertyCbV3(SUCCESS);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetAudioEffectProperty(const AudioEffectPropertyArray &propertyArray)
{
    auto request = [propertyArray]() {
        HpaePolicyManager::GetInstance().SetAudioEffectProperty(propertyArray);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAudioEffectProperty(AudioEffectPropertyArray &propertyArray)
{
    auto request = [this, &propertyArray]() {
        HpaePolicyManager::GetInstance().GetAudioEffectProperty(propertyArray);
        if (auto serviceCallback = serviceCallback_.lock()) {
            serviceCallback->OnGetAudioEffectPropertyCb(SUCCESS);
        }
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::InitHdiState()
{
    auto request = []() {
        HpaePolicyManager::GetInstance().InitHdiState();
    };
    SendRequest(request, __func__);
}

void HpaeManager::UpdateEffectBtOffloadSupported(const bool &isSupported)
{
    auto request = [isSupported]() {
        HpaePolicyManager::GetInstance().UpdateEffectBtOffloadSupported(isSupported);
    };
    SendRequest(request, __func__);
}

void HpaeManager::UpdateParamExtra(const std::string &mainkey, const std::string &subkey, const std::string &value)
{
    auto request = [mainkey, subkey, value]() {
        HpaePolicyManager::GetInstance().UpdateParamExtra(mainkey, subkey, value);
    };
    SendRequest(request, __func__);
}

void HpaeManager::HandleRendererManager(const std::string &sinkName, const HpaeStreamInfo &streamInfo)
{
    auto rendererManager = SafeGetMap(rendererManagerMap_, sinkName);
    CHECK_AND_RETURN_LOG(rendererManager, "can not find sink[%{public}s] in rendererManagerMap_", sinkName.c_str());
    rendererManager->CreateStream(streamInfo);
    if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_PLAY) {
        rendererIdSinkNameMap_[streamInfo.sessionId] = sinkName;
        rendererIdStreamInfoMap_[streamInfo.sessionId] = {streamInfo, HPAE_SESSION_NEW};
    } else if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_RECORD) {
        capturerIdSourceNameMap_[streamInfo.sessionId] = sinkName;
        capturerIdStreamInfoMap_[streamInfo.sessionId] = {streamInfo, HPAE_SESSION_NEW};
    }
}

void HpaeManager::CreateStreamForCapInner(const HpaeStreamInfo &streamInfo)
{
    if (streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_INVALID) {
        AUDIO_INFO_LOG("streamInfo.streamClassType == HPAE_STREAM_CLASS_TYPE_INVALID");
        return;
    }
    std::string deviceName = streamInfo.deviceName;
    HandleRendererManager(deviceName, streamInfo);
    AddStreamToCollection(streamInfo, deviceName);
    return;
}

void HpaeManager::InitAudioEnhanceChainManager(const std::vector<EffectChain> &enhanceChains,
    const EffectChainManagerParam &managerParam,
    const std::vector<std::shared_ptr<AudioEffectLibEntry>> &enhanceLibraryList)
{
    auto request = [enhanceChains, managerParam, enhanceLibraryList]() {
        HpaePolicyManager::GetInstance().InitAudioEnhanceChainManager(enhanceChains, managerParam, enhanceLibraryList);
    };
    SendRequest(request, __func__);
}

int32_t HpaeManager::SetOutputDevice(const uint32_t &renderId, const DeviceType &outputDevice)
{
    auto request = [renderId, outputDevice]() {
        HpaePolicyManager::GetInstance().SetOutputDevice(renderId, outputDevice);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetVolumeInfo(const AudioVolumeType &volumeType, const float &systemVol)
{
    auto request = [volumeType, systemVol]() {
        HpaePolicyManager::GetInstance().SetVolumeInfo(volumeType, systemVol);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetMicrophoneMuteInfo(const bool &isMute)
{
    auto request = [isMute]() {
        HpaePolicyManager::GetInstance().SetMicrophoneMuteInfo(isMute);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetStreamVolumeInfo(const uint32_t &sessionId, const float &streamVol)
{
    auto request = [sessionId, streamVol]() {
        HpaePolicyManager::GetInstance().SetStreamVolumeInfo(sessionId, streamVol);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetAudioEnhanceProperty(const AudioEffectPropertyArrayV3 &propertyArray, DeviceType deviceType)
{
    auto request = [propertyArray, deviceType]() {
        HpaePolicyManager::GetInstance().SetAudioEnhanceProperty(propertyArray, deviceType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAudioEnhanceProperty(AudioEffectPropertyArrayV3 &propertyArray, DeviceType deviceType)
{
    auto request = [&propertyArray, deviceType]() {
        HpaePolicyManager::GetInstance().GetAudioEnhanceProperty(propertyArray, deviceType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::SetAudioEnhanceProperty(const AudioEnhancePropertyArray &propertyArray, DeviceType deviceType)
{
    auto request = [propertyArray, deviceType]() {
        HpaePolicyManager::GetInstance().SetAudioEnhanceProperty(propertyArray, deviceType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

int32_t HpaeManager::GetAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray, DeviceType deviceType)
{
    auto request = [&propertyArray, deviceType]() {
        HpaePolicyManager::GetInstance().GetAudioEnhanceProperty(propertyArray, deviceType);
    };
    SendRequest(request, __func__);
    return SUCCESS;
}

void HpaeManager::UpdateExtraSceneType(
    const std::string &mainkey, const std::string &subkey, const std::string &extraSceneType)
{
    auto request = [mainkey, subkey, extraSceneType]() {
        HpaePolicyManager::GetInstance().UpdateExtraSceneType(mainkey, subkey, extraSceneType);
    };
    SendRequest(request, __func__);
    return;
}

void HpaeManager::NotifySettingsDataReady()
{
    HpaePolicyManager::GetInstance().LoadEffectProperties();
    LoadEffectLive();
}
    
void HpaeManager::NotifyAccountsChanged()
{
    HpaePolicyManager::GetInstance().LoadEffectProperties();
    LoadEffectLive();
}
    
 bool HpaeManager::IsAcousticEchoCancelerSupported(SourceType sourceType)
 {
    if (sourceType == SOURCE_TYPE_VOICE_COMMUNICATION || sourceType == SOURCE_TYPE_VOICE_TRANSCRIPTION) {
        return true;
    }
    if (sourceType != SOURCE_TYPE_LIVE) {
        return false;
    }
    std::string value = HpaePolicyManager::GetInstance().GetAudioParameter("primary", AudioParamKey::PARAM_KEY_STATE,
        "source_type_live_aec_supported");
    AUDIO_INFO_LOG("live_aec_supported: %{public}s", value.c_str());
    if (value == "true") {
        return true;
    }
    return false;
}

void HpaeManager::LoadEffectLive()
{
    AudioSettingProvider &settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    ErrCode ret = ERROR;
    if (!settingProvider.CheckOsAccountReady()) {
        AUDIO_ERR_LOG("OS account not ready");
    } else {
        std::string configValue;
        ret = settingProvider.GetStringValue("live_effect", configValue, "system");
        if (ret == SUCCESS && !configValue.empty()) {
            effectLiveState_ = configValue;
            return;
        }
    }
    std::string state = HpaePolicyManager::GetInstance().GetAudioParameter(
        "primary", AudioParamKey::PARAM_KEY_STATE, "live_effect_supported");
    if (state != "true") {
        effectLiveState_ = "NoSupport";
    } else {
        effectLiveState_ = "NROFF";
    }
    AUDIO_INFO_LOG("EffectLive %{public}s", effectLiveState_.c_str());
    if (settingProvider.CheckOsAccountReady()) {
        settingProvider.PutStringValue("live_effect", effectLiveState_, "system");
    }
}

bool HpaeManager::SetEffectLiveParameter(const std::vector<std::pair<std::string, std::string>> &params)
{
    CHECK_AND_RETURN_RET_LOG(!params.empty(), false, "params is empty");
    const auto &[paramKey, paramValue] = params[0];
    if (paramKey != "live_effect" || (paramValue != "NRON" && paramValue != "NROFF")) {
        AUDIO_ERR_LOG("Parameter Error");
        return false;
    }

    if (effectLiveState_ == "") {
        LoadEffectLive();
    }

    if (effectLiveState_ == "NoSupport") {
        AUDIO_ERR_LOG("effectLive not supported");
        return false;
    }

    const std::string combinedParam = paramKey + "=" + paramValue;
    HpaePolicyManager::GetInstance().SetAudioParameter("primary", AudioParamKey::PARAM_KEY_STATE, "", combinedParam);
    effectLiveState_ = paramValue;
    AudioSettingProvider &settingProvider = AudioSettingProvider::GetInstance(AUDIO_POLICY_SERVICE_ID);
    if (!settingProvider.CheckOsAccountReady()) {
        AUDIO_ERR_LOG("OS account not ready");
        return false;
    }

    ErrCode ret = settingProvider.PutStringValue(paramKey, paramValue, "system");
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Failed to set system value");
        return false;
    }
    return true;
}
    
bool HpaeManager::GetEffectLiveParameter(const std::vector<std::string> &subKeys,
    std::vector<std::pair<std::string, std::string>> &result)
{
    std::string targetKey = subKeys.empty() ? "live_effect_supported" : subKeys[0];
    if (targetKey != "live_effect_supported") {
        AUDIO_ERR_LOG("Parameter Error");
        return false;
    }
    if (effectLiveState_ != "") {
        result.emplace_back(targetKey, effectLiveState_);
        return true;
    }
    LoadEffectLive();
    result.emplace_back(targetKey, effectLiveState_);
    return true;
}

int32_t HpaeManager::UpdateCollaborativeState(bool isCollaborationEnabled)
{
    auto request = [this, isCollaborationEnabled]() {
        std::shared_ptr<IHpaeRendererManager> rendererManager = GetRendererManagerByName(BT_SINK_NAME);
        CHECK_AND_RETURN_LOG(rendererManager != nullptr,
            "can not find sink[%{public}s] in rendererManagerMap_", BT_SINK_NAME.c_str());
        rendererManager->UpdateCollaborativeState(isCollaborationEnabled);
    };
    SendRequest(request, __func__);
    return true;
}

void HpaeManager::HandleConnectCoBufferNode(std::shared_ptr<HpaeCoBufferNode> hpaeCoBufferNode)
{
    auto request = [this, hpaeCoBufferNode]() {
        AUDIO_INFO_LOG("HandleConnectCoBufferNode");
        std::shared_ptr<IHpaeRendererManager> defaultRendererManager = GetRendererManagerByName(coreSink_);
        CHECK_AND_RETURN_LOG(defaultRendererManager != nullptr,
            "can not find sink[%{public}s] in rendererManagerMap_", coreSink_.c_str());
        CHECK_AND_RETURN_LOG(hpaeCoBufferNode != nullptr, "hpaeCoBufferNode is nullptr");
        defaultRendererManager->ConnectCoBufferNode(hpaeCoBufferNode);
    };
    SendRequest(request, __func__);
}

void HpaeManager::HandleDisConnectCoBufferNode(std::shared_ptr<HpaeCoBufferNode> hpaeCoBufferNode)
{
    auto request = [this, hpaeCoBufferNode]() {
        AUDIO_INFO_LOG("HandleDisConnectCoBufferNode");
        std::shared_ptr<IHpaeRendererManager> defaultRendererManager = GetRendererManagerByName(coreSink_);
        CHECK_AND_RETURN_LOG(defaultRendererManager != nullptr,
            "can not find sink[%{public}s] in rendererManagerMap_", coreSink_.c_str());
        CHECK_AND_RETURN_LOG(hpaeCoBufferNode != nullptr, "hpaeCoBufferNode is nullptr");
        defaultRendererManager->DisConnectCoBufferNode(hpaeCoBufferNode);
    };
    SendRequest(request, __func__);
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS
