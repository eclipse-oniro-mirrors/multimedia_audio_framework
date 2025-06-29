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
#define LOG_TAG "AudioCoreServicePrivate"
#endif

#include "audio_core_service.h"

#include <variant>

#include "system_ability.h"
#include "audio_server_proxy.h"
#include "audio_policy_utils.h"
#include "iservice_registry.h"
#include "hdi_adapter_info.h"
#include "audio_usb_manager.h"
#include "audio_spatialization_service.h"
#include "audio_collaborative_service.h"
#include "ipc_skeleton.h"

namespace OHOS {
namespace AudioStandard {
namespace {
static const int32_t MEDIA_SERVICE_UID = 1013;
const int32_t DATA_LINK_CONNECTED = 11;
const uint32_t FIRST_SESSIONID = 100000;
const uid_t MCU_UID = 7500;
constexpr uint32_t MAX_VALID_SESSIONID = UINT32_MAX - FIRST_SESSIONID;
static const int VOLUME_LEVEL_DEFAULT_SIZE = 3;
static const int32_t BLUETOOTH_FETCH_RESULT_DEFAULT = 0;
static const int32_t BLUETOOTH_FETCH_RESULT_CONTINUE = 1;
static const int32_t BLUETOOTH_FETCH_RESULT_ERROR = 2;
static const int64_t WAIT_MODEM_CALL_SET_VOLUME_TIME_US = 120000; // 120ms
static const int64_t RING_DUAL_END_DELAY_US = 100000; // 100ms
static const int64_t MEDIA_PAUSE_TO_DOUBLE_RING_DELAY_US = 100000; // 100ms
static const int64_t OLD_DEVICE_UNAVALIABLE_MUTE_MS = 1000000; // 1s
static const int64_t NEW_DEVICE_AVALIABLE_MUTE_MS = 400000; // 400ms
static const int64_t NEW_DEVICE_AVALIABLE_OFFLOAD_MUTE_MS = 1000000; // 1s
static const int64_t NEW_DEVICE_REMOTE_CAST_AVALIABLE_MUTE_MS = 300000; // 300ms
static const int64_t SELECT_DEVICE_MUTE_MS = 200000; // 200ms
static const int64_t SELECT_OFFLOAD_DEVICE_MUTE_MS = 400000; // 400ms
static const int64_t OLD_DEVICE_UNAVALIABLE_MUTE_SLEEP_MS = 150000; // 150ms
static const int64_t OLD_DEVICE_UNAVALIABLE_EXT_MUTE_MS = 300000; // 300ms
static const int64_t DISTRIBUTED_DEVICE_UNAVALIABLE_MUTE_MS = 1500000;  // 1.5s
static const int64_t DISTRIBUTED_DEVICE_UNAVALIABLE_SLEEP_US = 350000; // 350ms
static const int32_t DISTRIBUTED_DEVICE = 1003;
static const uint32_t BT_BUFFER_ADJUSTMENT_FACTOR = 50;
static const int32_t WAIT_OFFLOAD_CLOSE_TIME_SEC = 10;
static const char* CHECK_FAST_BLOCK_PREFIX = "Is_Fast_Blocked_For_AppName#";
static const std::unordered_set<SourceType> specialSourceTypeSet_ = {
    SOURCE_TYPE_PLAYBACK_CAPTURE,
    SOURCE_TYPE_WAKEUP,
    SOURCE_TYPE_VIRTUAL_CAPTURE,
    SOURCE_TYPE_REMOTE_CAST
};
}

static const std::vector<std::string> SourceNames = {
    std::string(PRIMARY_MIC),
    std::string(BLUETOOTH_MIC),
    std::string(USB_MIC),
    std::string(PRIMARY_WAKEUP),
    std::string(FILE_SOURCE),
    std::string(ACCESSORY_SOURCE)
};

std::string AudioCoreService::GetEncryptAddr(const std::string &addr)
{
    const int32_t START_POS = 6;
    const int32_t END_POS = 13;
    const int32_t ADDRESS_STR_LEN = 17;
    if (addr.empty() || addr.length() != ADDRESS_STR_LEN) {
        return std::string("");
    }
    std::string tmp = "**:**:**:**:**:**";
    std::string out = addr;
    for (int i = START_POS; i <= END_POS; i++) {
        out[i] = tmp[i];
    }
    return out;
}

void AudioCoreService::UpdateActiveDeviceAndVolumeBeforeMoveSession(
    std::vector<std::shared_ptr<AudioStreamDescriptor>> &streamDescs, const AudioStreamDeviceChangeReasonExt reason)
{
    bool needUpdateActiveDevice = true;
    bool isUpdateActiveDevice = false;
    uint32_t sessionId = 0;
    for (std::shared_ptr<AudioStreamDescriptor> streamDesc : streamDescs) {
        //  if streamDesc select bluetooth or headset, active it
        if (!HandleOutputStreamInRunning(streamDesc, reason)) {
            continue;
        }
        int32_t outputRet = ActivateOutputDevice(streamDesc, reason);
        CHECK_AND_CONTINUE_LOG(outputRet == SUCCESS, "Activate output device failed");

        // update current output device
        if (needUpdateActiveDevice) {
            isUpdateActiveDevice = UpdateOutputDevice(streamDesc->newDeviceDescs_.front(), GetRealUid(streamDesc),
                reason);
            needUpdateActiveDevice = !isUpdateActiveDevice;
            sessionId = streamDesc->sessionId_;
        }

        // started stream need to mute when switch device
        if (streamDesc->streamStatus_ == STREAM_STATUS_STARTED) {
            MuteSinkPortForSwitchDevice(streamDesc, reason);
        }
    }
    
    if (isUpdateActiveDevice) {
        AUDIO_INFO_LOG("active device updated, update volume for %{public}d", sessionId);
        AudioDeviceDescriptor audioDeviceDescriptor = audioActiveDevice_.GetCurrentOutputDevice();
        audioVolumeManager_.SetVolumeForSwitchDevice(audioDeviceDescriptor, "");
        OnPreferredOutputDeviceUpdated(audioDeviceDescriptor);
    }
}

int32_t AudioCoreService::FetchRendererPipesAndExecute(
    std::vector<std::shared_ptr<AudioStreamDescriptor>> &streamDescs, const AudioStreamDeviceChangeReasonExt reason)
{
    AUDIO_INFO_LOG("[PipeFetchStart] all %{public}zu output streams", streamDescs.size());
    UpdateActiveDeviceAndVolumeBeforeMoveSession(streamDescs, reason);
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = audioPipeSelector_->FetchPipesAndExecute(streamDescs);
    AUDIO_INFO_LOG("[PipeExecStart] for all Pipes");
    uint32_t audioFlag;
    for (auto &pipeInfo : pipeInfos) {
        CHECK_AND_CONTINUE_LOG(pipeInfo != nullptr, "pipeInfo is nullptr");
        AUDIO_INFO_LOG("[PipeExecInfo] Scan Pipe adapter: %{public}s, name: %{public}s, action: %{public}d",
            pipeInfo->moduleInfo_.adapterName.c_str(), pipeInfo->name_.c_str(), pipeInfo->pipeAction_);
        if (pipeInfo->moduleInfo_.name == OFFLOAD_PRIMARY_SPEAKER &&
            pipeInfo->streamDescriptors_.size() > 0) {
            isOffloadOpened_.store(true);
            offloadCloseCondition_.notify_all();
        }
        if (pipeInfo->pipeAction_ == PIPE_ACTION_UPDATE) {
            ProcessOutputPipeUpdate(pipeInfo, audioFlag, reason);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_NEW) {
            ProcessOutputPipeNew(pipeInfo, audioFlag, reason);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_DEFAULT) {
            // Do nothing
        }
    }
    pipeManager_->UpdateRendererPipeInfos(pipeInfos);
    RemoveUnusedPipe();
    return SUCCESS;
}

int32_t AudioCoreService::FetchCapturerPipesAndExecute(
    std::vector<std::shared_ptr<AudioStreamDescriptor>> &streamDescs)
{
    AUDIO_INFO_LOG("[PipeFetchStart] all %{public}zu input streams", streamDescs.size());
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = audioPipeSelector_->FetchPipesAndExecute(streamDescs);

    AUDIO_INFO_LOG("[PipeExecStart] for all Pipes");
    uint32_t audioFlag;
    for (auto &pipeInfo : pipeInfos) {
        CHECK_AND_CONTINUE_LOG(pipeInfo != nullptr, "pipeInfo is nullptr");
        AUDIO_INFO_LOG("[PipeExecInfo] Scan Pipe adapter: %{public}s, name: %{public}s, action: %{public}d",
            pipeInfo->moduleInfo_.adapterName.c_str(), pipeInfo->name_.c_str(), pipeInfo->pipeAction_);
        if (pipeInfo->pipeAction_ == PIPE_ACTION_UPDATE) {
            ProcessInputPipeUpdate(pipeInfo, audioFlag);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_NEW) {
            ProcessInputPipeNew(pipeInfo, audioFlag);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_DEFAULT) {
            // Do nothing
        }
    }
    pipeManager_->UpdateCapturerPipeInfos(pipeInfos);
    RemoveUnusedPipe();
    return SUCCESS;
}

int32_t AudioCoreService::ScoInputDeviceFetchedForRecongnition(bool handleFlag, const std::string &address,
    ConnectState connectState)
{
    AUDIO_INFO_LOG("handleflag %{public}d, address %{public}s, connectState %{public}d",
        handleFlag, GetEncryptAddr(address).c_str(), connectState);
    if (handleFlag && connectState != DEACTIVE_CONNECTED) {
        return SUCCESS;
    }
    return Bluetooth::AudioHfpManager::HandleScoWithRecongnition(handleFlag);
}

void AudioCoreService::BluetoothScoFetch(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    Trace trace("AudioCoreService::BluetoothScoFetch");
    CHECK_AND_RETURN_LOG(streamDesc != nullptr && streamDesc->newDeviceDescs_.size() > 0 &&
        streamDesc->newDeviceDescs_[0] != nullptr, "invalid streamDesc");
    shared_ptr<AudioDeviceDescriptor> desc = streamDesc->newDeviceDescs_[0];
    int32_t ret = Bluetooth::AudioHfpManager::SetActiveHfpDevice(desc->macAddress_);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Active hfp device failed, retrigger fetch input device");
        desc->exceptionFlag_ = true;
        audioDeviceManager_.UpdateDevicesListInfo(
            std::make_shared<AudioDeviceDescriptor>(*desc), EXCEPTION_FLAG_UPDATE);
        FetchInputDeviceAndRoute();
        return;
    }

    if (Util::IsScoSupportSource(streamDesc->capturerInfo_.sourceType)) {
        ret = ScoInputDeviceFetchedForRecongnition(true, desc->macAddress_, desc->connectState_);
    } else {
        ret = Bluetooth::AudioHfpManager::UpdateAudioScene(audioSceneManager_.GetAudioScene(true), true);
    }
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("sco [%{public}s] is not connected yet",
            GetEncryptAddr(desc->macAddress_).c_str());
    }
}

void AudioCoreService::CheckModemScene(const AudioStreamDeviceChangeReasonExt reason)
{
    if (!pipeManager_->IsModemCommunicationIdExist()) {
        return;
    }
    if (audioSceneManager_.GetAudioScene() == AUDIO_SCENE_PHONE_CALL) {
        pipeManager_->UpdateModemStreamStatus(STREAM_STATUS_STARTED);
    } else {
        pipeManager_->UpdateModemStreamStatus(STREAM_STATUS_STOPPED);
    }
    vector<std::shared_ptr<AudioDeviceDescriptor>> descs =
        audioRouterCenter_.FetchOutputDevices(STREAM_USAGE_VOICE_MODEM_COMMUNICATION, -1);
    CHECK_AND_RETURN_LOG(descs.size() != 0, "Fetch output device for voice modem communication failed");
    pipeManager_->UpdateModemStreamDevice(descs);
    AUDIO_INFO_LOG("Update route %{public}d", descs.front()->deviceType_);

    if (descs.front()->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO) {
        auto modemCommunicationMap = pipeManager_->GetModemCommunicationMap();
        auto modemMap = modemCommunicationMap.begin();
        if (modemMap != modemCommunicationMap.end()) {
            int32_t ret = HandleScoOutputDeviceFetched(modemCommunicationMap.begin()->second, reason);
            AUDIO_INFO_LOG("HandleScoOutputDeviceFetched %{public}d", ret);
        }
    }

    auto ret = ActivateNearlinkDevice(pipeManager_->GetModemCommunicationMap().begin()->second);
    audioActiveDevice_.UpdateActiveDeviceRoute(descs.front()->deviceType_, DeviceFlag::OUTPUT_DEVICES_FLAG);

    AudioDeviceDescriptor desc = AudioDeviceDescriptor(descs.front());
    std::unordered_map<uint32_t, std::shared_ptr<AudioStreamDescriptor>> modemSessionMap =
        pipeManager_->GetModemCommunicationMap();
    for (auto it = modemSessionMap.begin(); it != modemSessionMap.end(); ++it) {
        streamCollector_.UpdateRendererDeviceInfo(GetRealUid(it->second), it->first, desc);
        sleAudioDeviceManager_.UpdateSleStreamTypeCount(it->second);
    }
}

void AudioCoreService::HandleAudioCaptureState(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo)
{
    if (mode == AUDIO_MODE_RECORD &&
        (streamChangeInfo.audioCapturerChangeInfo.capturerState == CAPTURER_RELEASED ||
         streamChangeInfo.audioCapturerChangeInfo.capturerState == CAPTURER_STOPPED)) {
        auto sourceType = streamChangeInfo.audioCapturerChangeInfo.capturerInfo.sourceType;
        auto sessionId = streamChangeInfo.audioCapturerChangeInfo.sessionId;
        sleAudioDeviceManager_.UpdateSleStreamTypeCount(pipeManager_->GetStreamDescById(sessionId));
        if (Util::IsScoSupportSource(sourceType)) {
            Bluetooth::AudioHfpManager::HandleScoWithRecongnition(false);
        } else {
            AUDIO_INFO_LOG("close capture app, try to disconnect sco");
            bool isRecord = streamCollector_.HasRunningNormalCapturerStream();
            Bluetooth::AudioHfpManager::UpdateAudioScene(audioSceneManager_.GetAudioScene(true), isRecord);
        }
        audioMicrophoneDescriptor_.RemoveAudioCapturerMicrophoneDescriptorBySessionID(sessionId);
    }
}

void AudioCoreService::UpdateDefaultOutputDeviceWhenStopping(int32_t uid)
{
    std::vector<uint32_t> sessionIDSet = streamCollector_.GetAllRendererSessionIDForUID(uid);
    for (const auto &sessionID : sessionIDSet) {
        audioDeviceManager_.UpdateDefaultOutputDeviceWhenStopping(sessionID);
        audioDeviceManager_.RemoveSelectedDefaultOutputDevice(sessionID);
	if (isRingDualToneOnPrimarySpeaker_ && (streamCollector_.GetStreamType(sessionID) == STREAM_RING ||
            streamCollector_.GetStreamType(sessionID) == STREAM_ALARM)) {
            AUDIO_INFO_LOG("disable primary speaker dual tone when ringer renderer died");
            isRingDualToneOnPrimarySpeaker_ = false;
            for (std::pair<AudioStreamType, StreamUsage> stream : streamsWhenRingDualOnPrimarySpeaker_) {
                audioPolicyManager_.SetInnerStreamMute(stream.first, false, stream.second);
            }
            streamsWhenRingDualOnPrimarySpeaker_.clear();
            audioPolicyManager_.SetInnerStreamMute(STREAM_MUSIC, false, STREAM_USAGE_MUSIC);
        }
    }
}

void AudioCoreService::UpdateInputDeviceWhenStopping(int32_t uid)
{
    std::vector<uint32_t> sessionIDSet = streamCollector_.GetAllCapturerSessionIDForUID(uid);
    for (const auto &sessionID : sessionIDSet) {
        audioDeviceManager_.RemoveSelectedInputDevice(sessionID);
    }
    FetchInputDeviceAndRoute();
}

int32_t AudioCoreService::BluetoothDeviceFetchOutputHandle(shared_ptr<AudioStreamDescriptor> &streamDesc,
    const AudioStreamDeviceChangeReasonExt reason, std::string encryptMacAddr)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr, BLUETOOTH_FETCH_RESULT_ERROR, "Stream desc is nullptr");
    std::shared_ptr<AudioDeviceDescriptor> desc = streamDesc->newDeviceDescs_.front();
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, BLUETOOTH_FETCH_RESULT_CONTINUE, "Device desc is nullptr");

    if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP) {
        AUDIO_INFO_LOG("A2dp device");
        int32_t ret = ActivateA2dpDeviceWhenDescEnabled(desc, reason);
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("Activate a2dp [%{public}s] failed", encryptMacAddr.c_str());
            return BLUETOOTH_FETCH_RESULT_ERROR;
        }
    } else if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO) {
        int32_t ret = HandleScoOutputDeviceFetched(streamDesc, reason);
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("sco [%{public}s] is not connected yet", encryptMacAddr.c_str());
            return BLUETOOTH_FETCH_RESULT_ERROR;
        }
    }
    return BLUETOOTH_FETCH_RESULT_DEFAULT;
}

int32_t AudioCoreService::ActivateA2dpDeviceWhenDescEnabled(shared_ptr<AudioDeviceDescriptor> desc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, ERR_NULL_POINTER, "invalid deviceDesc");
    AUDIO_INFO_LOG("Desc isEnabled %{public}d", desc->isEnable_);
    if (desc->isEnable_) {
        return ActivateA2dpDevice(desc, reason);
    }
    return SUCCESS;
}


int32_t AudioCoreService::ActivateA2dpDevice(std::shared_ptr<AudioDeviceDescriptor> desc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    Trace trace("AudioCoreService::ActiveA2dpDevice");
    int32_t ret = SwitchActiveA2dpDevice(desc);
    AUDIO_INFO_LOG("ActivateA2dpDevice ret : %{public}d", ret);
    // In plan: re-try when failed
    return ret;
}

int32_t AudioCoreService::SwitchActiveA2dpDevice(std::shared_ptr<AudioDeviceDescriptor> deviceDescriptor)
{
    CHECK_AND_RETURN_RET_LOG(deviceDescriptor != nullptr &&
        audioA2dpDevice_.CheckA2dpDeviceExist(deviceDescriptor->macAddress_),
        ERR_INVALID_PARAM, "Target A2DP device doesn't exist.");
    int32_t result = ERROR;
#ifdef BLUETOOTH_ENABLE
    std::string lastActiveA2dpDevice = audioActiveDevice_.GetActiveBtDeviceMac();
    audioActiveDevice_.SetActiveBtDeviceMac(deviceDescriptor->macAddress_);
    AudioDeviceDescriptor lastDevice = audioPolicyManager_.GetActiveDeviceDescriptor();
    deviceDescriptor->deviceType_ = DEVICE_TYPE_BLUETOOTH_A2DP;

    if (Bluetooth::AudioA2dpManager::GetActiveA2dpDevice() == deviceDescriptor->macAddress_ &&
        audioIOHandleMap_.CheckIOHandleExist(BLUETOOTH_SPEAKER)) {
        AUDIO_WARNING_LOG("A2dp device [%{public}s] [%{public}s] is already active",
            GetEncryptAddr(deviceDescriptor->macAddress_).c_str(), deviceDescriptor->deviceName_.c_str());
        return SUCCESS;
    }

    result = Bluetooth::AudioA2dpManager::SetActiveA2dpDevice(deviceDescriptor->macAddress_);
    if (result != SUCCESS) {
        audioActiveDevice_.SetActiveBtDeviceMac(lastActiveA2dpDevice);
        AUDIO_ERR_LOG("Active [%{public}s] failed, using original [%{public}s] device",
            GetEncryptAddr(audioActiveDevice_.GetActiveBtDeviceMac()).c_str(),
            GetEncryptAddr(lastActiveA2dpDevice).c_str());
        return result;
    }

    AudioStreamInfo audioStreamInfo = {};
    audioActiveDevice_.GetActiveA2dpDeviceStreamInfo(DEVICE_TYPE_BLUETOOTH_A2DP, audioStreamInfo);
    std::string networkId = audioActiveDevice_.GetCurrentOutputDeviceNetworkId();
    std::string sinkName = AudioPolicyUtils::GetInstance().GetSinkPortName(
        audioActiveDevice_.GetCurrentOutputDeviceType());
    result = LoadA2dpModule(DEVICE_TYPE_BLUETOOTH_A2DP, audioStreamInfo, networkId, sinkName, SOURCE_TYPE_INVALID);
    CHECK_AND_RETURN_RET_LOG(result == SUCCESS, ERR_OPERATION_FAILED, "LoadA2dpModule failed %{public}d", result);
#endif
    return result;
}

int32_t AudioCoreService::LoadA2dpModule(DeviceType deviceType, const AudioStreamInfo &audioStreamInfo,
    std::string networkId, std::string sinkName, SourceType sourceType)
{
    std::list<AudioModuleInfo> moduleInfoList;
    bool ret = policyConfigMananger_.GetModuleListByType(ClassType::TYPE_A2DP, moduleInfoList);
    CHECK_AND_RETURN_RET_LOG(ret, ERR_OPERATION_FAILED, "A2dp module is not exist in the configuration file");

    // not load bt_a2dp_fast and bt_hdap, maybe need fix
    int32_t loadRet = AudioServerProxy::GetInstance().LoadHdiAdapterProxy(HDI_DEVICE_MANAGER_TYPE_BLUETOOTH, "bt_a2dp");
    if (loadRet) {
        AUDIO_ERR_LOG("load adapter failed");
    }
    for (auto &moduleInfo : moduleInfoList) {
        DeviceRole configRole = moduleInfo.role == "source" ? INPUT_DEVICE : OUTPUT_DEVICE;
        DeviceRole deviceRole = deviceType == DEVICE_TYPE_BLUETOOTH_A2DP ? OUTPUT_DEVICE : INPUT_DEVICE;
        AUDIO_INFO_LOG("Load a2dp module [%{public}s], role[%{public}d], config role[%{public}d]",
            moduleInfo.name.c_str(), deviceRole, configRole);
        if (configRole != deviceRole) {continue;}
        if (audioIOHandleMap_.CheckIOHandleExist(moduleInfo.name) == false) {
            AUDIO_INFO_LOG("A2dp device connects for the first time");
            // a2dp device connects for the first time
            GetA2dpModuleInfo(moduleInfo, audioStreamInfo, sourceType);
            uint32_t paIndex = 0;
            AudioIOHandle ioHandle = audioPolicyManager_.OpenAudioPort(moduleInfo, paIndex);
            CHECK_AND_RETURN_RET_LOG(ioHandle != HDI_INVALID_ID,
                ERR_INVALID_HANDLE, "OpenAudioPort failed ioHandle[%{public}u]", ioHandle);
            CHECK_AND_RETURN_RET_LOG(paIndex != OPEN_PORT_FAILURE,
                ERR_OPERATION_FAILED, "OpenAudioPort failed paId[%{public}u]", paIndex);
            audioIOHandleMap_.AddIOHandleInfo(moduleInfo.name, ioHandle);

            std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
            pipeInfo->id_ = ioHandle;
            pipeInfo->paIndex_ = paIndex;
            if (moduleInfo.role == "sink") {
                pipeInfo->name_ = "a2dp_output";
                pipeInfo->pipeRole_ = PIPE_ROLE_OUTPUT;
                pipeInfo->routeFlag_ = AUDIO_OUTPUT_FLAG_NORMAL;
            } else {
                pipeInfo->name_ = "a2dp_input";
                pipeInfo->pipeRole_ = PIPE_ROLE_INPUT;
                pipeInfo->routeFlag_ = AUDIO_INPUT_FLAG_NORMAL;
            }
            pipeInfo->adapterName_ = "a2dp";
            pipeInfo->moduleInfo_ = moduleInfo;
            pipeInfo->pipeAction_ = PIPE_ACTION_DEFAULT;
            pipeManager_->AddAudioPipeInfo(pipeInfo);
            AUDIO_INFO_LOG("Add PipeInfo %{public}u in load a2dp.", pipeInfo->id_);
        } else {
            // At least one a2dp device is already connected. A new a2dp device is connecting.
            // Need to reload a2dp module when switching to a2dp device.
            int32_t result = ReloadA2dpAudioPort(moduleInfo, deviceType, audioStreamInfo, networkId, sinkName,
                sourceType);
            CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "ReloadA2dpAudioPort failed %{public}d", result);
        }
    }

    return SUCCESS;
}

int32_t AudioCoreService::ReloadA2dpAudioPort(AudioModuleInfo &moduleInfo, DeviceType deviceType,
    const AudioStreamInfo &audioStreamInfo, std::string networkId, std::string sinkName,
    SourceType sourceType)
{
    AUDIO_INFO_LOG("Switch device from a2dp to another a2dp, reload a2dp module");
    if (deviceType == DEVICE_TYPE_BLUETOOTH_A2DP) {
        audioIOHandleMap_.MuteDefaultSinkPort(networkId, sinkName);
    }

    // Firstly, unload the existing a2dp sink or source.
    std::string portName = BLUETOOTH_SPEAKER;
    if (deviceType == DEVICE_TYPE_BLUETOOTH_A2DP_IN) {
        portName = BLUETOOTH_MIC;
    }
    AudioIOHandle activateDeviceIOHandle;
    audioIOHandleMap_.GetModuleIdByKey(portName, activateDeviceIOHandle);
    uint32_t curPaIndex = pipeManager_->GetPaIndexByIoHandle(activateDeviceIOHandle);
    std::vector<std::shared_ptr<AudioStreamDescriptor>> streamDescs =
        pipeManager_->GetStreamDescsByIoHandle(activateDeviceIOHandle);
    AUDIO_INFO_LOG("IoHandleId: %{public}u, paIndex: %{public}u, stream num: %{public}zu",
        activateDeviceIOHandle, curPaIndex, streamDescs.size());
    int32_t result = audioPolicyManager_.CloseAudioPort(activateDeviceIOHandle, curPaIndex);
    CHECK_AND_RETURN_RET_LOG(result == SUCCESS, result, "CloseAudioPort failed %{public}d", result);
    pipeManager_->RemoveAudioPipeInfo(activateDeviceIOHandle);

    // Load a2dp sink or source module again with the configuration of active a2dp device.
    GetA2dpModuleInfo(moduleInfo, audioStreamInfo, sourceType);
    uint32_t paIndex = 0;
    AudioIOHandle ioHandle = audioPolicyManager_.OpenAudioPort(moduleInfo, paIndex);
    CHECK_AND_RETURN_RET_LOG(ioHandle != HDI_INVALID_ID, ERR_INVALID_HANDLE,
        "OpenAudioPort failed ioHandle[%{public}u]", ioHandle);
    CHECK_AND_RETURN_RET_LOG(paIndex != OPEN_PORT_FAILURE, ERR_OPERATION_FAILED,
        "OpenAudioPort failed paId[%{public}u]", paIndex);
    audioIOHandleMap_.AddIOHandleInfo(moduleInfo.name, ioHandle);

    std::shared_ptr<AudioPipeInfo> pipeInfo = std::make_shared<AudioPipeInfo>();
    pipeInfo->id_ = ioHandle;
    pipeInfo->paIndex_ = paIndex;
    if (moduleInfo.role == "sink") {
        pipeInfo->name_ = "a2dp_output";
        pipeInfo->pipeRole_ = PIPE_ROLE_OUTPUT;
        pipeInfo->routeFlag_ = AUDIO_OUTPUT_FLAG_NORMAL;
    } else {
        pipeInfo->name_ = "a2dp_input";
        pipeInfo->pipeRole_ = PIPE_ROLE_INPUT;
        pipeInfo->routeFlag_ = AUDIO_INPUT_FLAG_NORMAL;
    }
    pipeInfo->adapterName_ = "a2dp";
    pipeInfo->moduleInfo_ = moduleInfo;
    pipeInfo->pipeAction_ = PIPE_ACTION_DEFAULT;
    pipeInfo->streamDescriptors_.insert(pipeInfo->streamDescriptors_.end(), streamDescs.begin(), streamDescs.end());
    pipeManager_->AddAudioPipeInfo(pipeInfo);
    AUDIO_INFO_LOG("Close paIndex: %{public}u, open paIndex: %{public}u", curPaIndex, paIndex);
    return SUCCESS;
}

void AudioCoreService::GetA2dpModuleInfo(AudioModuleInfo &moduleInfo, const AudioStreamInfo& audioStreamInfo,
    SourceType sourceType)
{
    uint32_t bufferSize = audioStreamInfo.samplingRate *
        AudioPolicyUtils::GetInstance().PcmFormatToBytes(audioStreamInfo.format) *
        audioStreamInfo.channels / BT_BUFFER_ADJUSTMENT_FACTOR;
    AUDIO_INFO_LOG("a2dp rate: %{public}d, format: %{public}d, channel: %{public}d",
        audioStreamInfo.samplingRate, audioStreamInfo.format, audioStreamInfo.channels);
    moduleInfo.channels = to_string(audioStreamInfo.channels);
    moduleInfo.rate = to_string(audioStreamInfo.samplingRate);
    moduleInfo.format = AudioPolicyUtils::GetInstance().ConvertToHDIAudioFormat(audioStreamInfo.format);
    moduleInfo.bufferSize = to_string(bufferSize);
    if (moduleInfo.role != "source") {
        moduleInfo.renderInIdleState = "1";
        moduleInfo.sinkLatency = "0";
    }
}

bool AudioCoreService::IsSameDevice(shared_ptr<AudioDeviceDescriptor> &desc, const AudioDeviceDescriptor &deviceInfo)
{
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, ERR_NULL_POINTER, "invalid deviceDesc");
    if (desc->networkId_ == deviceInfo.networkId_ && desc->deviceType_ == deviceInfo.deviceType_ &&
        desc->macAddress_ == deviceInfo.macAddress_ && desc->connectState_ == deviceInfo.connectState_) {
        AUDIO_INFO_LOG("Enter");
        if (deviceInfo.IsAudioDeviceDescriptor()) {
            return true;
        }
        BluetoothOffloadState state = audioA2dpOffloadFlag_.GetA2dpOffloadFlag();
        if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP &&
            // switch to A2dp
            ((deviceInfo.a2dpOffloadFlag_ == A2DP_OFFLOAD && state != A2DP_OFFLOAD) ||
            // switch to A2dp offload
            (deviceInfo.a2dpOffloadFlag_ != A2DP_OFFLOAD && state == A2DP_OFFLOAD))) {
            return false;
        }
        if (IsUsb(desc->deviceType_)) {
            return desc->deviceRole_ == deviceInfo.deviceRole_;
        }
        return true;
    } else {
        return false;
    }
}

int32_t AudioCoreService::FetchDeviceAndRoute(const AudioStreamDeviceChangeReasonExt reason)
{
    int32_t ret = FetchOutputDeviceAndRoute(reason);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Fetch output device failed");
    return FetchInputDeviceAndRoute();
}

int32_t AudioCoreService::FetchRendererPipeAndExecute(std::shared_ptr<AudioStreamDescriptor> streamDesc,
    uint32_t &sessionId, uint32_t &audioFlag, const AudioStreamDeviceChangeReasonExt reason)
{
    AUDIO_INFO_LOG("[PipeFetchStart] for stream %{public}d", sessionId);
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = audioPipeSelector_->FetchPipeAndExecute(streamDesc);

    AUDIO_INFO_LOG("[PipeExecStart] for all Pipes");
    uint32_t sinkId = HDI_INVALID_ID;
    for (auto &pipeInfo : pipeInfos) {
        CHECK_AND_CONTINUE_LOG(pipeInfo != nullptr, "pipeInfo is nullptr");
        AUDIO_INFO_LOG("[PipeExecInfo] Scan Pipe adapter: %{public}s, name: %{public}s, action: %{public}d",
            pipeInfo->moduleInfo_.adapterName.c_str(), pipeInfo->name_.c_str(), pipeInfo->pipeAction_);
        if (pipeInfo->pipeAction_ == PIPE_ACTION_UPDATE) {
            ProcessOutputPipeUpdate(pipeInfo, audioFlag, reason);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_NEW) { // new
            ProcessOutputPipeNew(pipeInfo, audioFlag, reason);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_DEFAULT) { // DEFAULT
            // Do nothing
        }
    }
    RemoveUnusedPipe();
    return SUCCESS;
}

void AudioCoreService::ProcessOutputPipeNew(std::shared_ptr<AudioPipeInfo> pipeInfo, uint32_t &flag,
    const AudioStreamDeviceChangeReasonExt reason)
{
    uint32_t paIndex = 0;
    uint32_t id = OpenNewAudioPortAndRoute(pipeInfo, paIndex);
    CHECK_AND_RETURN_LOG(id != HDI_INVALID_ID, "Invalid id: %{public}u", id);
    CHECK_AND_RETURN_LOG(paIndex != OPEN_PORT_FAILURE, "Invalid paIndex: %{public}u", paIndex);
    pipeInfo->id_ = id;
    pipeInfo->paIndex_ = paIndex;

    for (auto &desc : pipeInfo->streamDescriptors_) {
        CHECK_AND_CONTINUE_LOG(desc != nullptr, "desc is nullptr");
        AUDIO_INFO_LOG("[StreamExecInfo] Stream: %{public}u, action: %{public}d, belong to %{public}s",
            desc->sessionId_, desc->streamAction_, pipeInfo->name_.c_str());
        switch (desc->streamAction_) {
            case AUDIO_STREAM_ACTION_NEW:
                flag = desc->routeFlag_;
                break;
            case AUDIO_STREAM_ACTION_MOVE:
                if (desc->streamStatus_ != STREAM_STATUS_STARTED) {
                    MoveStreamSink(desc, pipeInfo, reason);
                } else {
                    MoveToNewOutputDevice(desc, pipeInfo, reason);
                }
                break;
            case AUDIO_STREAM_ACTION_RECREATE:
                TriggerRecreateRendererStreamCallback(desc->appInfo_.appPid,
                    desc->sessionId_, desc->routeFlag_);
                break;
            default:
                break;
        }
    }
    pipeManager_->AddAudioPipeInfo(pipeInfo);
}

void AudioCoreService::ProcessOutputPipeUpdate(std::shared_ptr<AudioPipeInfo> pipeInfo, uint32_t &flag,
    const AudioStreamDeviceChangeReasonExt reason)
{
    for (auto &desc : pipeInfo->streamDescriptors_) {
        CHECK_AND_CONTINUE_LOG(desc != nullptr, "desc is nullptr");
        AUDIO_INFO_LOG("[StreamExecInfo] Stream: %{public}u, action: %{public}d, belong to %{public}s",
            desc->sessionId_, desc->streamAction_, pipeInfo->name_.c_str());
        switch (desc->streamAction_) {
            case AUDIO_STREAM_ACTION_NEW:
                flag = desc->routeFlag_;
                break;
            case AUDIO_STREAM_ACTION_DEFAULT:
            case AUDIO_STREAM_ACTION_MOVE:
                if (desc->streamStatus_ != STREAM_STATUS_STARTED) {
                    MoveStreamSink(desc, pipeInfo, reason);
                } else {
                    MoveToNewOutputDevice(desc, pipeInfo, reason);
                }
                break;
            case AUDIO_STREAM_ACTION_RECREATE:
                TriggerRecreateRendererStreamCallback(desc->appInfo_.appPid,
                    desc->sessionId_, desc->routeFlag_);
                break;
            default:
                break;
        }
    }
    pipeManager_->UpdateAudioPipeInfo(pipeInfo);
}

int32_t AudioCoreService::FetchCapturerPipeAndExecute(std::shared_ptr<AudioStreamDescriptor> streamDesc,
    uint32_t &audioFlag, uint32_t &sessionId)
{
    if (sessionId == 0) {
        streamDesc->sessionId_ = GenerateSessionId();
        sessionId = streamDesc->sessionId_;
        AUDIO_INFO_LOG("Generate sessionId: %{public}u for stream", sessionId);
    }

    if (streamDesc->capturerInfo_.sourceType == SOURCE_TYPE_PLAYBACK_CAPTURE) {
        AUDIO_INFO_LOG("[PipeFetchInfo] playbackcapture, no need fetch pipe");
        audioFlag = AUDIO_INPUT_FLAG_NORMAL;
        return SUCCESS;
    }

    AUDIO_INFO_LOG("[PipeFetchStart] for stream %{public}d", sessionId);
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = audioPipeSelector_->FetchPipeAndExecute(streamDesc);

    AUDIO_INFO_LOG("[PipeExecStart] for all Pipes");
    for (auto &pipeInfo : pipeInfos) {
        AUDIO_INFO_LOG("[PipeExecInfo] Scan Pipe adapter: %{public}s, name: %{public}s, action: %{public}d",
            pipeInfo->moduleInfo_.adapterName.c_str(), pipeInfo->name_.c_str(), pipeInfo->pipeAction_);
        if (pipeInfo->pipeAction_ == PIPE_ACTION_UPDATE) {
            ProcessInputPipeUpdate(pipeInfo, audioFlag);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_NEW) {
            ProcessInputPipeNew(pipeInfo, audioFlag);
        } else if (pipeInfo->pipeAction_ == PIPE_ACTION_DEFAULT) {
            // Do nothing
        }
    }
    RemoveUnusedPipe();
    return SUCCESS;
}

void AudioCoreService::ProcessInputPipeNew(std::shared_ptr<AudioPipeInfo> pipeInfo, uint32_t &flag)
{
    uint32_t paIndex = 0;
    uint32_t sourceId = OpenNewAudioPortAndRoute(pipeInfo, paIndex);
    CHECK_AND_RETURN_LOG(sourceId != HDI_INVALID_ID, "Invalid sourceId: %{public}u", sourceId);
    CHECK_AND_RETURN_LOG(paIndex != OPEN_PORT_FAILURE, "Invalid paIndex: %{public}u", paIndex);
    pipeInfo->id_ = sourceId;
    pipeInfo->paIndex_ = paIndex;

    for (auto &desc : pipeInfo->streamDescriptors_) {
        AUDIO_INFO_LOG("[StreamExecInfo] Stream: %{public}u, action: %{public}d, belong to %{public}s",
            desc->sessionId_, desc->streamAction_, pipeInfo->name_.c_str());
        switch (desc->streamAction_) {
            case AUDIO_STREAM_ACTION_NEW:
                flag = desc->routeFlag_;
                break;
            case AUDIO_STREAM_ACTION_DEFAULT:
            case AUDIO_STREAM_ACTION_MOVE:
                if (desc->streamStatus_ != STREAM_STATUS_STARTED) {
                    MoveStreamSource(desc);
                } else {
                    MoveToNewInputDevice(desc);
                }
                break;
            case AUDIO_STREAM_ACTION_RECREATE:
                TriggerRecreateCapturerStreamCallback(desc);
                break;
            default:
                break;
        }
    }
    pipeManager_->AddAudioPipeInfo(pipeInfo);
}

void AudioCoreService::ProcessInputPipeUpdate(std::shared_ptr<AudioPipeInfo> pipeInfo, uint32_t &flag)
{
    for (auto desc : pipeInfo->streamDescriptors_) {
        AUDIO_INFO_LOG("[StreamExecInfo] Stream: %{public}u, action: %{public}d, belong to %{public}s",
            desc->sessionId_, desc->streamAction_, pipeInfo->name_.c_str());
        switch (desc->streamAction_) {
            case AUDIO_STREAM_ACTION_NEW:
                flag = desc->routeFlag_;
                break;
            case AUDIO_STREAM_ACTION_DEFAULT:
            case AUDIO_STREAM_ACTION_MOVE:
                if (desc->streamStatus_ != STREAM_STATUS_STARTED) {
                    MoveStreamSource(desc);
                } else {
                    MoveToNewInputDevice(desc);
                }
                break;
            case AUDIO_STREAM_ACTION_RECREATE:
                TriggerRecreateCapturerStreamCallback(desc);
                break;
            default:
                break;
        }
    }
    pipeManager_->UpdateAudioPipeInfo(pipeInfo);
}

void AudioCoreService::RemoveUnusedPipe()
{
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = pipeManager_->GetUnusedPipe();
    for (auto pipeInfo : pipeInfos) {
        CHECK_AND_CONTINUE_LOG(pipeInfo != nullptr, "pipeInfo is nullptr");
        AUDIO_INFO_LOG("[PipeExecInfo] Remove and close Pipe %{public}s", pipeInfo->ToString().c_str());
        if (pipeInfo->routeFlag_ & AUDIO_OUTPUT_FLAG_LOWPOWER) {
            DelayReleaseOffloadPipe(pipeInfo->id_, pipeInfo->paIndex_);
            continue;
        }
        audioPolicyManager_.CloseAudioPort(pipeInfo->id_, pipeInfo->paIndex_);
        pipeManager_->RemoveAudioPipeInfo(pipeInfo);
    }
}

void AudioCoreService::RemoveUnusedRecordPipe()
{
    std::vector<std::shared_ptr<AudioPipeInfo>> pipeInfos = pipeManager_->GetUnusedRecordPipe();
    for (auto pipeInfo : pipeInfos) {
        CHECK_AND_CONTINUE_LOG(pipeInfo != nullptr, "pipeInfo is nullptr");
        AUDIO_INFO_LOG("[PipeExecInfo] Remove and close Pipe %{public}s", pipeInfo->ToString().c_str());
        audioPolicyManager_.CloseAudioPort(pipeInfo->id_, pipeInfo->paIndex_);
        pipeManager_->RemoveAudioPipeInfo(pipeInfo);
    }
}

std::string AudioCoreService::GetAdapterNameBySessionId(uint32_t sessionId)
{
    AUDIO_INFO_LOG("SessionId %{public}u", sessionId);
    std::string adapterName = pipeManager_->GetAdapterNameBySessionId(sessionId);
    return adapterName;
}

int32_t AudioCoreService::GetProcessDeviceInfoBySessionId(uint32_t sessionId, AudioDeviceDescriptor &deviceInfo)
{
    AUDIO_INFO_LOG("SessionId %{public}u", sessionId);
    deviceInfo = AudioDeviceDescriptor(pipeManager_->GetProcessDeviceInfoBySessionId(sessionId));
    return SUCCESS;
}

std::atomic<uint32_t> g_sessionId = {FIRST_SESSIONID}; // begin at 100000

uint32_t AudioCoreService::GenerateSessionId()
{
    uint32_t sessionId = g_sessionId++;
    AUDIO_INFO_LOG("sessionId:%{public}d", sessionId);
    if (g_sessionId > MAX_VALID_SESSIONID) {
        AUDIO_WARNING_LOG("sessionId is too large, reset it!");
        g_sessionId = FIRST_SESSIONID;
    }
    return sessionId;
}

void AudioCoreService::AddSessionId(const uint32_t sessionId)
{
    uid_t callingUid = static_cast<uid_t>(IPCSkeleton::GetCallingUid());
    AUDIO_INFO_LOG("AddSessionId: %{public}u, callingUid: %{public}u", sessionId, callingUid);
    if (callingUid == MCU_UID) {
        // There is no audio stream for the session id of MCU. So no need to save it.
        return;
    }
    std::lock_guard<std::mutex> lock(sessionIdMutex_);
    sessionIdMap_[sessionId] = callingUid;
}

void AudioCoreService::DeleteSessionId(const uint32_t sessionId)
{
    AUDIO_INFO_LOG("DeleteSessionId: %{public}u", sessionId);
    std::lock_guard<std::mutex> lock(sessionIdMutex_);
    if (sessionIdMap_.count(sessionId) == 0) {
        AUDIO_INFO_LOG("The sessionId has been deleted from sessionIdMap_!");
    } else {
        sessionIdMap_.erase(sessionId);
    }
}

bool AudioCoreService::IsStreamBelongToUid(const uid_t uid, const uint32_t sessionId)
{
    std::lock_guard<std::mutex> lock(sessionIdMutex_);
    if (sessionIdMap_.count(sessionId) == 0) {
        AUDIO_INFO_LOG("The sessionId %{public}u is invalid!", sessionId);
        return false;
    }

    if (sessionIdMap_[sessionId] != uid) {
        AUDIO_INFO_LOG("The sessionId %{public}u does not belong to uid %{public}u!", sessionId, uid);
        return false;
    }

    AUDIO_DEBUG_LOG("The sessionId %{public}u belongs to uid %{public}u!", sessionId, uid);
    return true;
}

void AudioCoreService::OnDeviceStatusUpdated(DeviceType devType, bool isConnected, const std::string& macAddress,
    const std::string& deviceName, const AudioStreamInfo& streamInfo, DeviceRole role, bool hasPair)
{
    // Pnp device status update
    audioDeviceStatus_.OnDeviceStatusUpdated(devType, isConnected, macAddress, deviceName, streamInfo, role, hasPair);
}

void AudioCoreService::OnDeviceStatusUpdated(AudioDeviceDescriptor &updatedDesc, bool isConnected)
{
    // Bluetooth device status updated
    DeviceType devType = updatedDesc.deviceType_;
    string macAddress = updatedDesc.macAddress_;
    string deviceName = updatedDesc.deviceName_;
    bool isActualConnection = (updatedDesc.connectState_ != VIRTUAL_CONNECTED);
    AUDIO_INFO_LOG("Device connection is actual connection: %{public}d", isActualConnection);

    AudioStreamInfo streamInfo = updatedDesc.audioStreamInfo_.CheckParams() ?
        AudioStreamInfo(*updatedDesc.audioStreamInfo_.samplingRate.rbegin(), updatedDesc.audioStreamInfo_.encoding,
        updatedDesc.audioStreamInfo_.format, *updatedDesc.audioStreamInfo_.channels.rbegin()) : AudioStreamInfo();
#ifdef BLUETOOTH_ENABLE
    if (devType == DEVICE_TYPE_BLUETOOTH_A2DP && isActualConnection && isConnected) {
        int32_t ret = Bluetooth::AudioA2dpManager::GetA2dpDeviceStreamInfo(macAddress, streamInfo);
        CHECK_AND_RETURN_LOG(ret == SUCCESS, "Get a2dp device stream info failed!");
    }
    if (devType == DEVICE_TYPE_BLUETOOTH_A2DP_IN && isActualConnection && isConnected) {
        int32_t ret = Bluetooth::AudioA2dpManager::GetA2dpInDeviceStreamInfo(macAddress, streamInfo);
        CHECK_AND_RETURN_LOG(ret == SUCCESS, "Get a2dp input device stream info failed!");
    }
    if (isConnected && isActualConnection
        && devType == DEVICE_TYPE_BLUETOOTH_SCO
        && updatedDesc.deviceCategory_ != BT_UNWEAR_HEADPHONE
        && !audioDeviceManager_.GetScoState()) {
        Bluetooth::AudioHfpManager::SetActiveHfpDevice(macAddress);
    }
#endif
    audioDeviceStatus_.OnDeviceStatusUpdated(updatedDesc, devType,
        macAddress, deviceName, isActualConnection, streamInfo, isConnected);
}

void AudioCoreService::OnDeviceStatusUpdated(DStatusInfo statusInfo, bool isStop)
{
    // Distributed devices status update
    audioDeviceStatus_.OnDeviceStatusUpdated(statusInfo, isStop);
}

void AudioCoreService::MoveStreamSink(std::shared_ptr<AudioStreamDescriptor> streamDesc,
    std::shared_ptr<AudioPipeInfo> pipeInfo, const AudioStreamDeviceChangeReasonExt reason)
{
    Trace trace("AudioCoreService::MoveStreamSink");

    DeviceType oldDeviceType = DEVICE_TYPE_NONE;
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = streamDesc->newDeviceDescs_.front();
    AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u to [%{public}d][%{public}s], reason %{public}d",
        streamDesc->sessionId_, newDeviceDesc->deviceType_, GetEncryptAddr(newDeviceDesc->macAddress_).c_str(),
        static_cast<int32_t>(reason));

    std::vector<SinkInput> sinkInputs;
    audioPolicyManager_.GetAllSinkInputs(sinkInputs);
    std::vector<SinkInput> targetSinkInputs = audioOffloadStream_.FilterSinkInputs(streamDesc->sessionId_, sinkInputs);

    auto ret = (newDeviceDesc->networkId_ == LOCAL_NETWORK_ID)
        ? MoveToLocalOutputDevice(targetSinkInputs, pipeInfo, newDeviceDesc)
        : MoveToRemoteOutputDevice(targetSinkInputs, newDeviceDesc);
    audioIOHandleMap_.NotifyUnmutePort();
    CHECK_AND_RETURN_LOG(ret == SUCCESS, "Move sink input %{public}d to device %{public}d failed!",
        streamDesc->sessionId_, newDeviceDesc->deviceType_);
    streamCollector_.UpdateRendererDeviceInfo(newDeviceDesc);
}

bool AudioCoreService::IsNewDevicePlaybackSupported(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr && !streamDesc->newDeviceDescs_.empty(), false,
        "invalid streamDesc");
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = streamDesc->newDeviceDescs_.front();
    CHECK_AND_RETURN_RET_LOG(newDeviceDesc != nullptr, false, "invalid newDeviceDesc");
    if (streamDesc->streamInfo_.encoding == ENCODING_EAC3 && newDeviceDesc->deviceType_ != DEVICE_TYPE_HDMI &&
        newDeviceDesc->deviceType_ != DEVICE_TYPE_LINE_DIGITAL && audioPolicyServerHandler_) {
        audioPolicyServerHandler_->SendFormatUnsupportedErrorEvent(ERROR_UNSUPPORTED_FORMAT);
        return false;
    }
    return true;
}

void AudioCoreService::MoveToNewOutputDevice(std::shared_ptr<AudioStreamDescriptor> streamDesc,
    std::shared_ptr<AudioPipeInfo> pipeInfo, const AudioStreamDeviceChangeReasonExt reason)
{
    Trace trace("AudioCoreService::MoveToNewOutputDevice");

    DeviceType oldDeviceType = DEVICE_TYPE_NONE;
    bool isNeedTriggerCallback = true;
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = streamDesc->newDeviceDescs_.front();
    std::string oldSinkName = "";
    if (streamDesc->oldDeviceDescs_.size() == 0) {
        AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u to [%{public}d][%{public}s], reason %{public}d",
            streamDesc->sessionId_, newDeviceDesc->deviceType_,
            GetEncryptAddr(newDeviceDesc->macAddress_).c_str(), static_cast<int32_t>(reason));
    } else {
        PrepareMoveAttrs(streamDesc, oldDeviceType, isNeedTriggerCallback, oldSinkName, reason);
    }

    std::vector<SinkInput> sinkInputs;
    audioPolicyManager_.GetAllSinkInputs(sinkInputs);
    std::vector<SinkInput> targetSinkInputs = audioOffloadStream_.FilterSinkInputs(streamDesc->sessionId_, sinkInputs);

    if (isNeedTriggerCallback && audioPolicyServerHandler_) {
        std::shared_ptr<AudioDeviceDescriptor> callbackDesc = std::make_shared<AudioDeviceDescriptor>(*newDeviceDesc);
        callbackDesc->descriptorType_ = AudioDeviceDescriptor::DEVICE_INFO;
        audioPolicyServerHandler_->SendRendererDeviceChangeEvent(streamDesc->callerPid_,
            streamDesc->sessionId_, callbackDesc, reason);
    }

    AudioPolicyUtils::GetInstance().UpdateEffectDefaultSink(newDeviceDesc->deviceType_);

    CHECK_AND_RETURN_LOG(IsNewDevicePlaybackSupported(streamDesc), "new device not support playback");

    auto ret = (newDeviceDesc->networkId_ == LOCAL_NETWORK_ID)
        ? MoveToLocalOutputDevice(targetSinkInputs, pipeInfo, newDeviceDesc)
        : MoveToRemoteOutputDevice(targetSinkInputs, newDeviceDesc);
    if (ret != SUCCESS) {
        AudioPolicyUtils::GetInstance().UpdateEffectDefaultSink(oldDeviceType);
        AUDIO_ERR_LOG("Move sink input %{public}d to device %{public}d failed!",
            streamDesc->sessionId_, newDeviceDesc->deviceType_);
        audioIOHandleMap_.NotifyUnmutePort();
        return;
    }

    sleAudioDeviceManager_.UpdateSleStreamTypeCount(streamDesc);
    if (policyConfigMananger_.GetUpdateRouteSupport() && !reason.isSetAudioScene()) {
        UpdateOutputRoute(streamDesc);
    }

    streamCollector_.UpdateRendererDeviceInfo(newDeviceDesc);
    ReConfigOffloadStatus(streamDesc->sessionId_, pipeInfo, oldSinkName);
    audioIOHandleMap_.NotifyUnmutePort();
}

void AudioCoreService::OnMicrophoneBlockedUpdate(DeviceType devType, DeviceBlockStatus status)
{
    CHECK_AND_RETURN_LOG(devType != DEVICE_TYPE_NONE, "devType is none type");
    audioDeviceStatus_.OnMicrophoneBlockedUpdate(devType, status);
}

void AudioCoreService::OnPnpDeviceStatusUpdated(AudioDeviceDescriptor &desc, bool isConnected)
{
    audioDeviceStatus_.OnPnpDeviceStatusUpdated(desc, isConnected);
}

void AudioCoreService::OnDeviceConfigurationChanged(DeviceType deviceType, const std::string &macAddress,
    const std::string &deviceName, const AudioStreamInfo &streamInfo)
{
    audioDeviceStatus_.OnDeviceConfigurationChanged(deviceType, macAddress, deviceName, streamInfo);
}

int32_t AudioCoreService::OnServiceConnected(AudioServiceIndex serviceIndex)
{
    return audioDeviceStatus_.OnServiceConnected(serviceIndex);
}

void AudioCoreService::OnForcedDeviceSelected(DeviceType devType, const std::string &macAddress)
{
    audioDeviceStatus_.OnForcedDeviceSelected(devType, macAddress);
}

int32_t AudioCoreService::MoveToRemoteOutputDevice(std::vector<SinkInput> sinkInputIds,
    std::shared_ptr<AudioDeviceDescriptor> remoteDeviceDescriptor)
{
    AUDIO_INFO_LOG("Start for [%{public}zu] sink-inputs", sinkInputIds.size());

    std::string networkId = remoteDeviceDescriptor->networkId_;
    DeviceRole deviceRole = remoteDeviceDescriptor->deviceRole_;
    DeviceType deviceType = remoteDeviceDescriptor->deviceType_;

    // check: networkid
    CHECK_AND_RETURN_RET_LOG(networkId != LOCAL_NETWORK_ID, ERR_INVALID_OPERATION,
        "failed: not a remote device.");

    uint32_t sinkId = -1; // invalid sink id, use sink name instead.
    std::string moduleName = AudioPolicyUtils::GetInstance().GetRemoteModuleName(networkId, deviceRole);
    AUDIO_ERR_LOG("moduleName %{public}s", moduleName.c_str());

    AudioIOHandle moduleId;
    if (audioIOHandleMap_.GetModuleIdByKey(moduleName, moduleId)) {
        (void)moduleId; // mIOHandle is module id, not equal to sink id.
    } else {
        AUDIO_ERR_LOG("no such device.");
        if (!isOpenRemoteDevice) {
            AUDIO_INFO_LOG("directly return");
            return ERR_INVALID_PARAM;
        } else {
            return OpenRemoteAudioDevice(networkId, deviceRole, deviceType, remoteDeviceDescriptor);
        }
    }
    int32_t res = AudioServerProxy::GetInstance().CheckRemoteDeviceStateProxy(networkId, deviceRole, true);
    CHECK_AND_RETURN_RET_LOG(res == SUCCESS, ERR_OPERATION_FAILED, "remote device state is invalid!");

    // start move.
    for (size_t i = 0; i < sinkInputIds.size(); i++) {
        int32_t ret = audioPolicyManager_.MoveSinkInputByIndexOrName(sinkInputIds[i].paStreamId, sinkId, moduleName);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR, "move [%{public}d] failed", sinkInputIds[i].streamId);
        audioRouteMap_.AddRouteMapInfo(sinkInputIds[i].uid, moduleName, sinkInputIds[i].pid);
    }

    if (deviceType != DeviceType::DEVICE_TYPE_DEFAULT) {
        AUDIO_WARNING_LOG("Not defult type[%{public}d] on device:[%{public}s]",
            deviceType, GetEncryptStr(networkId).c_str());
    }
    isCurrentRemoteRenderer_ = true;
    return SUCCESS;
}

void AudioCoreService::MoveStreamSource(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    Trace trace("AudioCoreService::MoveStreamSource");
    std::vector<SourceOutput> targetSourceOutputs = FilterSourceOutputs(streamDesc->sessionId_);

    AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u to [%{public}d][%{public}s]",
        streamDesc->sessionId_, streamDesc->newDeviceDescs_.front()->deviceType_,
        GetEncryptAddr(streamDesc->newDeviceDescs_.front()->macAddress_).c_str());

    // MoveSourceOuputByIndexName
    auto ret = (streamDesc->newDeviceDescs_.front()->networkId_ == LOCAL_NETWORK_ID)
        ? MoveToLocalInputDevice(targetSourceOutputs, streamDesc->newDeviceDescs_.front())
        : MoveToRemoteInputDevice(targetSourceOutputs, streamDesc->newDeviceDescs_.front());
    CHECK_AND_RETURN_LOG((ret == SUCCESS), "Move source output %{public}d to device %{public}d failed!",
        streamDesc->sessionId_, streamDesc->newDeviceDescs_.front()->deviceType_);
    streamCollector_.UpdateCapturerDeviceInfo(streamDesc->newDeviceDescs_.front());
}

void AudioCoreService::MoveToNewInputDevice(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    Trace trace("AudioCoreService::MoveToNewInputDevice");
    std::vector<SourceOutput> targetSourceOutputs = FilterSourceOutputs(streamDesc->sessionId_);

    if (streamDesc->oldDeviceDescs_.size() == 0) {
        AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u to [%{public}d][%{public}s]",
            streamDesc->sessionId_, streamDesc->newDeviceDescs_.front()->deviceType_,
            GetEncryptAddr(streamDesc->newDeviceDescs_.front()->macAddress_).c_str());
    } else {
        AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u [%{public}d][%{public}s] to [%{public}d][%{public}s]",
            streamDesc->sessionId_, streamDesc->oldDeviceDescs_.front()->deviceType_,
            GetEncryptAddr(streamDesc->oldDeviceDescs_.front()->macAddress_).c_str(),
            streamDesc->newDeviceDescs_.front()->deviceType_,
            GetEncryptAddr(streamDesc->newDeviceDescs_.front()->macAddress_).c_str());
    }

    // MoveSourceOuputByIndexName
    auto ret = (streamDesc->newDeviceDescs_.front()->networkId_ == LOCAL_NETWORK_ID)
        ? MoveToLocalInputDevice(targetSourceOutputs, streamDesc->newDeviceDescs_.front())
        : MoveToRemoteInputDevice(targetSourceOutputs, streamDesc->newDeviceDescs_.front());
    CHECK_AND_RETURN_LOG((ret == SUCCESS), "Move source output %{public}d to device %{public}d failed!",
        streamDesc->sessionId_, streamDesc->newDeviceDescs_.front()->deviceType_);

    sleAudioDeviceManager_.UpdateSleStreamTypeCount(streamDesc);

    if (policyConfigMananger_.GetUpdateRouteSupport() &&
        streamDesc->newDeviceDescs_.front()->networkId_ == LOCAL_NETWORK_ID) {
        audioActiveDevice_.UpdateActiveDeviceRoute(streamDesc->newDeviceDescs_.front()->deviceType_,
            DeviceFlag::INPUT_DEVICES_FLAG, streamDesc->newDeviceDescs_.front()->deviceName_);
    }
    streamCollector_.UpdateCapturerDeviceInfo(streamDesc->newDeviceDescs_.front());
}

int32_t AudioCoreService::MoveToLocalInputDevice(std::vector<SourceOutput> sourceOutputs,
    std::shared_ptr<AudioDeviceDescriptor> localDeviceDescriptor)
{
    CHECK_AND_RETURN_RET_LOG(LOCAL_NETWORK_ID == localDeviceDescriptor->networkId_, ERR_INVALID_OPERATION,
        "failed: not a local device.");

    uint32_t sourceId = -1; // invalid source id, use source name instead.
    std::string sourceName = AudioPolicyUtils::GetInstance().GetSourcePortName(localDeviceDescriptor->deviceType_);
    for (size_t i = 0; i < sourceOutputs.size(); i++) {
        int32_t ret = audioPolicyManager_.MoveSourceOutputByIndexOrName(sourceOutputs[i].paStreamId,
            sourceId, sourceName);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR,
            "move [%{public}d] to local failed", sourceOutputs[i].paStreamId);
    }

    return SUCCESS;
}

int32_t AudioCoreService::MoveToRemoteInputDevice(std::vector<SourceOutput> sourceOutputs,
    std::shared_ptr<AudioDeviceDescriptor> remoteDeviceDescriptor)
{
    AUDIO_INFO_LOG("Start");

    std::string networkId = remoteDeviceDescriptor->networkId_;
    DeviceRole deviceRole = remoteDeviceDescriptor->deviceRole_;
    DeviceType deviceType = remoteDeviceDescriptor->deviceType_;

    // check: networkid
    CHECK_AND_RETURN_RET_LOG(networkId != LOCAL_NETWORK_ID, ERR_INVALID_OPERATION,
        "failed: not a remote device.");

    uint32_t sourceId = -1; // invalid sink id, use sink name instead.
    std::string moduleName = AudioPolicyUtils::GetInstance().GetRemoteModuleName(networkId, deviceRole);

    AudioIOHandle moduleId;
    if (audioIOHandleMap_.GetModuleIdByKey(moduleName, moduleId)) {
        (void)moduleId; // mIOHandle is module id, not equal to sink id.
    } else {
        AUDIO_ERR_LOG("no such device.");
        if (!isOpenRemoteDevice) {
            return ERR_INVALID_PARAM;
        } else {
            return OpenRemoteAudioDevice(networkId, deviceRole, deviceType, remoteDeviceDescriptor);
        }
    }
    int32_t res = AudioServerProxy::GetInstance().CheckRemoteDeviceStateProxy(networkId, deviceRole, true);
    CHECK_AND_RETURN_RET_LOG(res == SUCCESS, ERR_OPERATION_FAILED, "remote device state is invalid!");

    // start move.
    for (size_t i = 0; i < sourceOutputs.size(); i++) {
        int32_t ret = audioPolicyManager_.MoveSourceOutputByIndexOrName(sourceOutputs[i].paStreamId,
            sourceId, moduleName);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR,
            "move [%{public}d] failed", sourceOutputs[i].paStreamId);
    }

    if (deviceType != DeviceType::DEVICE_TYPE_DEFAULT) {
        AUDIO_DEBUG_LOG("Not defult type[%{public}d] on device:[%{public}s]",
            deviceType, GetEncryptStr(networkId).c_str());
    }
    return SUCCESS;
}

int32_t AudioCoreService::OpenRemoteAudioDevice(std::string networkId, DeviceRole deviceRole, DeviceType deviceType,
    std::shared_ptr<AudioDeviceDescriptor> remoteDeviceDescriptor)
{
    AUDIO_INFO_LOG("[PipeExecInfo] open remote pipe device %{public}d", deviceType);
    // open the test device. We should open it when device is online.
    std::string moduleName = AudioPolicyUtils::GetInstance().GetRemoteModuleName(networkId, deviceRole);
    AudioModuleInfo remoteDeviceInfo = AudioPolicyUtils::GetInstance().ConstructRemoteAudioModuleInfo(networkId,
        deviceRole, deviceType);

    auto ret = AudioServerProxy::GetInstance().LoadHdiAdapterProxy(HDI_DEVICE_MANAGER_TYPE_REMOTE, networkId);
    if (ret) {
        AUDIO_ERR_LOG("load adapter fail");
    }
    audioIOHandleMap_.OpenPortAndInsertIOHandle(moduleName, remoteDeviceInfo);

    // If device already in list, remove it else do not modify the list.
    audioConnectedDevice_.DelConnectedDevice(networkId, deviceType);
    AudioPolicyUtils::GetInstance().UpdateDisplayName(remoteDeviceDescriptor);
    audioConnectedDevice_.AddConnectedDevice(remoteDeviceDescriptor);
    audioMicrophoneDescriptor_.AddMicrophoneDescriptor(remoteDeviceDescriptor);
    return SUCCESS;
}

inline std::string PrintSourceOutput(SourceOutput sourceOutput)
{
    std::stringstream value;
    value << "streamId:[" << sourceOutput.streamId << "] ";
    value << "streamType:[" << sourceOutput.streamType << "] ";
    value << "uid:[" << sourceOutput.uid << "] ";
    value << "pid:[" << sourceOutput.pid << "] ";
    value << "statusMark:[" << sourceOutput.statusMark << "] ";
    value << "deviceSourceId:[" << sourceOutput.deviceSourceId << "] ";
    value << "startTime:[" << sourceOutput.startTime << "]";
    return value.str();
}

std::vector<SourceOutput> AudioCoreService::FilterSourceOutputs(int32_t sessionId)
{
    std::vector<SourceOutput> targetSourceOutputs = {};
    std::vector<SourceOutput> sourceOutputs = GetSourceOutputs();

    for (size_t i = 0; i < sourceOutputs.size(); i++) {
        AUDIO_DEBUG_LOG("sourceOutput[%{public}zu]:%{public}s", i, PrintSourceOutput(sourceOutputs[i]).c_str());
        if (sessionId == sourceOutputs[i].streamId) {
            targetSourceOutputs.push_back(sourceOutputs[i]);
        }
    }
    return targetSourceOutputs;
}

std::vector<SourceOutput> AudioCoreService::GetSourceOutputs()
{
    std::vector<SourceOutput> sourceOutputs;
    {
        std::unordered_map<std::string, AudioIOHandle> mapCopy = AudioIOHandleMap::GetInstance().GetCopy();
        if (std::any_of(mapCopy.cbegin(), mapCopy.cend(), [](const auto &pair) {
                return std::find(SourceNames.cbegin(), SourceNames.cend(), pair.first) != SourceNames.cend();
            })) {
            sourceOutputs = audioPolicyManager_.GetAllSourceOutputs();
        }
    }
    return sourceOutputs;
}

void AudioCoreService::UpdateOutputRoute(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    CHECK_AND_RETURN_LOG(streamDesc != nullptr, "streamDesc is nullptr");
    StreamUsage streamUsage = streamDesc->rendererInfo_.streamUsage;
    InternalDeviceType deviceType = streamDesc->newDeviceDescs_.front()->deviceType_;
    AUDIO_INFO_LOG("[PipeExecInfo] Update route streamUsage:%{public}d, devicetype:[%{public}s]",
        streamUsage, streamDesc->GetNewDevicesTypeString().c_str());
    if (Util::IsRingerOrAlarmerStreamUsage(streamUsage) && IsRingerOrAlarmerDualDevicesRange(deviceType) &&
        !VolumeUtils::IsPCVolumeEnable()) {
        if (!SelectRingerOrAlarmDevices(streamDesc)) {
            audioActiveDevice_.UpdateActiveDeviceRoute(deviceType, DeviceFlag::OUTPUT_DEVICES_FLAG);
        }

        AudioRingerMode ringerMode = audioPolicyManager_.GetRingerMode();
        if (ringerMode != RINGER_MODE_NORMAL &&
            IsRingerOrAlarmerDualDevicesRange(streamDesc->newDeviceDescs_.front()->getType()) &&
            streamDesc->newDeviceDescs_.front()->getType() != DEVICE_TYPE_SPEAKER) {
            audioPolicyManager_.SetInnerStreamMute(STREAM_RING, false, streamUsage);
            audioVolumeManager_.SetRingerModeMute(false);
            if (audioPolicyManager_.GetSystemVolumeLevel(STREAM_RING) <
                audioPolicyManager_.GetMaxVolumeLevel(STREAM_RING) / VOLUME_LEVEL_DEFAULT_SIZE) {
                audioPolicyManager_.SetDoubleRingVolumeDb(STREAM_RING,
                    audioPolicyManager_.GetMaxVolumeLevel(STREAM_RING) / VOLUME_LEVEL_DEFAULT_SIZE);
            }
        } else {
            audioVolumeManager_.SetRingerModeMute(true);
        }
        shouldUpdateDeviceDueToDualTone_ = true;
    } else {
        audioVolumeManager_.SetRingerModeMute(true);
        if (isRingDualToneOnPrimarySpeaker_ && streamUsage != STREAM_USAGE_VOICE_MODEM_COMMUNICATION) {
            std::vector<std::pair<InternalDeviceType, DeviceFlag>> activeDevices;
            activeDevices.push_back(make_pair(deviceType, DeviceFlag::OUTPUT_DEVICES_FLAG));
            activeDevices.push_back(make_pair(DEVICE_TYPE_SPEAKER, DeviceFlag::OUTPUT_DEVICES_FLAG));
            audioActiveDevice_.UpdateActiveDevicesRoute(activeDevices);
            AUDIO_INFO_LOG("Update desc [%{public}d] with speaker on session [%{public}d]",
                deviceType, streamDesc->sessionId_);
            AudioStreamType streamType = streamCollector_.GetStreamType(streamDesc->sessionId_);
            if (!AudioCoreServiceUtils::IsDualStreamWhenRingDual(streamType)) {
                streamsWhenRingDualOnPrimarySpeaker_.push_back(make_pair(streamType, streamUsage));
                audioPolicyManager_.SetInnerStreamMute(streamType, true, streamUsage);
            }
            shouldUpdateDeviceDueToDualTone_ = true;
        } else {
            audioActiveDevice_.UpdateActiveDeviceRoute(deviceType, DeviceFlag::OUTPUT_DEVICES_FLAG);
            shouldUpdateDeviceDueToDualTone_ = false;
        }
    }
}

void AudioCoreService::OnPreferredOutputDeviceUpdated(const AudioDeviceDescriptor& deviceDescriptor)
{
    AUDIO_INFO_LOG("In");
    Trace trace("AudioCoreService::OnPreferredOutputDeviceUpdated:" + std::to_string(deviceDescriptor.deviceType_));

    if (audioPolicyServerHandler_ != nullptr) {
        audioPolicyServerHandler_->SendPreferredOutputDeviceUpdated();
    }
    if (deviceDescriptor.deviceType_ != DEVICE_TYPE_BLUETOOTH_SCO) {
        spatialDeviceMap_.insert(make_pair(deviceDescriptor.macAddress_, deviceDescriptor.deviceType_));
    }

    if (deviceDescriptor.macAddress_ !=
        AudioSpatializationService::GetAudioSpatializationService().GetCurrentDeviceAddress()) {
        AudioServerProxy::GetInstance().UpdateEffectBtOffloadSupportedProxy(false);
    }
    AudioPolicyUtils::GetInstance().UpdateEffectDefaultSink(deviceDescriptor.deviceType_);
    AudioSpatializationService::GetAudioSpatializationService().UpdateCurrentDevice(deviceDescriptor.macAddress_);
    AudioCollaborativeService::GetAudioCollaborativeService().UpdateCurrentDevice(deviceDescriptor);
}

void AudioCoreService::OnPreferredInputDeviceUpdated(DeviceType deviceType, std::string networkId)
{
    AUDIO_INFO_LOG("OnPreferredInputDeviceUpdated Start");

    if (audioPolicyServerHandler_ != nullptr) {
        audioPolicyServerHandler_->SendPreferredInputDeviceUpdated();
    }
}


bool AudioCoreService::IsRingerOrAlarmerDualDevicesRange(const InternalDeviceType &deviceType)
{
    switch (deviceType) {
        case DEVICE_TYPE_SPEAKER:
        case DEVICE_TYPE_WIRED_HEADSET:
        case DEVICE_TYPE_WIRED_HEADPHONES:
        case DEVICE_TYPE_BLUETOOTH_SCO:
        case DEVICE_TYPE_BLUETOOTH_A2DP:
        case DEVICE_TYPE_USB_HEADSET:
        case DEVICE_TYPE_USB_ARM_HEADSET:
        case DEVICE_TYPE_NEARLINK:
            return true;
        default:
            return false;
    }
}

bool AudioCoreService::SelectRingerOrAlarmDevices(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc->newDeviceDescs_.size() > 0 &&
        streamDesc->newDeviceDescs_.size() <= AUDIO_CONCURRENT_ACTIVE_DEVICES_LIMIT, false,
        "audio devices not in range for ringer or alarmer.");
    const int32_t sessionId = static_cast<int32_t>(streamDesc->sessionId_);
    const StreamUsage streamUsage = streamDesc->rendererInfo_.streamUsage;
    bool allDevicesInDualDevicesRange = true;
    std::vector<std::pair<InternalDeviceType, DeviceFlag>> activeDevices;
    for (size_t i = 0; i < streamDesc->newDeviceDescs_.size(); i++) {
        if (IsRingerOrAlarmerDualDevicesRange(streamDesc->newDeviceDescs_[i]->deviceType_)) {
            activeDevices.push_back(make_pair(streamDesc->newDeviceDescs_[i]->deviceType_,
            DeviceFlag::OUTPUT_DEVICES_FLAG));
            AUDIO_INFO_LOG("select ringer/alarm devices devicetype[%{public}zu]:%{public}d",
                i, streamDesc->newDeviceDescs_[i]->deviceType_);
        } else {
            allDevicesInDualDevicesRange = false;
            break;
        }
    }

    AUDIO_INFO_LOG("select ringer/alarm sessionId:%{public}d, streamUsage:%{public}d", sessionId, streamUsage);
    if (!streamDesc->newDeviceDescs_.empty() && allDevicesInDualDevicesRange) {
        if (streamDesc->newDeviceDescs_.size() == AUDIO_CONCURRENT_ACTIVE_DEVICES_LIMIT &&
            AudioPolicyUtils::GetInstance().GetSinkName(*streamDesc->newDeviceDescs_.front(), sessionId) !=
            AudioPolicyUtils::GetInstance().GetSinkName(*streamDesc->newDeviceDescs_.back(), sessionId)) {
            AUDIO_INFO_LOG("set dual hal tone, reset primary sink to default before.");
            audioActiveDevice_.UpdateActiveDeviceRoute(DEVICE_TYPE_SPEAKER, DeviceFlag::OUTPUT_DEVICES_FLAG);
            if (enableDualHalToneState_ && enableDualHalToneSessionId_ != sessionId) {
                AUDIO_INFO_LOG("sesion changed, disable old dual hal tone.");
                UpdateDualToneState(false, enableDualHalToneSessionId_);
            }

            if ((audioPolicyManager_.GetRingerMode() != RINGER_MODE_NORMAL && streamUsage != STREAM_USAGE_ALARM) ||
                (VolumeUtils::IsPCVolumeEnable() && audioVolumeManager_.GetStreamMute(STREAM_MUSIC))) {
                AUDIO_INFO_LOG("no normal ringer mode and no alarm, dont dual hal tone.");
                return false;
            }
            UpdateDualToneState(true, sessionId);
        } else {
            if (enableDualHalToneState_ && enableDualHalToneSessionId_ == sessionId) {
                AUDIO_INFO_LOG("device unavailable, disable dual hal tone.");
                UpdateDualToneState(false, enableDualHalToneSessionId_);
            }
            isRingDualToneOnPrimarySpeaker_ = AudioCoreServiceUtils::IsRingDualToneOnPrimarySpeaker(
                streamDesc->newDeviceDescs_, sessionId);
            audioActiveDevice_.UpdateActiveDevicesRoute(activeDevices);
        }
        return true;
    }
    return false;
}

void AudioCoreService::UpdateDualToneState(const bool &enable, const int32_t &sessionId)
{
    AUDIO_INFO_LOG("Update dual tone state, enable:%{public}d, sessionId:%{public}d", enable, sessionId);
    enableDualHalToneState_ = enable;
    if (enableDualHalToneState_) {
        enableDualHalToneSessionId_ = sessionId;
    }
    Trace trace("AudioDeviceCommon::UpdateDualToneState sessionId:" + std::to_string(sessionId));
    auto ret = AudioServerProxy::GetInstance().UpdateDualToneStateProxy(enable, sessionId);
    CHECK_AND_RETURN_LOG(ret == SUCCESS, "Failed to update the dual tone state for sessionId:%{public}d", sessionId);
}

int32_t AudioCoreService::MoveToLocalOutputDevice(std::vector<SinkInput> sinkInputIds,
    std::shared_ptr<AudioPipeInfo> pipeInfo, std::shared_ptr<AudioDeviceDescriptor> localDeviceDescriptor)
{
    AUDIO_INFO_LOG("Start for [%{public}zu] sink-inputs", sinkInputIds.size());
    // check
    CHECK_AND_RETURN_RET_LOG(LOCAL_NETWORK_ID == localDeviceDescriptor->networkId_,
        ERR_INVALID_OPERATION, "failed: not a local device.");

    // start move.
    uint32_t sinkId = -1; // invalid sink id, use sink name instead.
    for (size_t i = 0; i < sinkInputIds.size(); i++) {
        AudioPipeType pipeType = PIPE_TYPE_UNKNOWN;
        std::string sinkName = localDeviceDescriptor->deviceType_ == DEVICE_TYPE_REMOTE_CAST ?
            "RemoteCastInnerCapturer" : pipeInfo->moduleInfo_.name;
        AUDIO_INFO_LOG("Session %{public}d, sinkName %{public}s", sinkInputIds[i].streamId, sinkName.c_str());
        if (sinkName == BLUETOOTH_SPEAKER) {
            std::string activePort = BLUETOOTH_SPEAKER;
            audioPolicyManager_.SuspendAudioDevice(activePort, false);
        }
        AUDIO_INFO_LOG("move for sinkInput [%{public}d], portName %{public}s pipeType %{public}d",
            sinkInputIds[i].streamId, sinkName.c_str(), pipeType);
        int32_t ret = audioPolicyManager_.MoveSinkInputByIndexOrName(sinkInputIds[i].paStreamId, sinkId, sinkName);
        CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERROR,
            "move [%{public}d] to local failed", sinkInputIds[i].streamId);
        audioRouteMap_.AddRouteMapInfo(sinkInputIds[i].uid, LOCAL_NETWORK_ID, sinkInputIds[i].pid);
    }

    isCurrentRemoteRenderer_ = false;
    return SUCCESS;
}

bool AudioCoreService::HasLowLatencyCapability(DeviceType deviceType, bool isRemote)
{
    // Distributed devices are low latency devices
    if (isRemote) {
        return true;
    }

    switch (deviceType) {
        case DeviceType::DEVICE_TYPE_EARPIECE:
        case DeviceType::DEVICE_TYPE_SPEAKER:
        case DeviceType::DEVICE_TYPE_WIRED_HEADSET:
        case DeviceType::DEVICE_TYPE_WIRED_HEADPHONES:
        case DeviceType::DEVICE_TYPE_USB_HEADSET:
        case DeviceType::DEVICE_TYPE_DP:
            return true;

        case DeviceType::DEVICE_TYPE_BLUETOOTH_SCO:
        case DeviceType::DEVICE_TYPE_BLUETOOTH_A2DP:
            return false;
        default:
            return false;
    }
}

void AudioCoreService::TriggerRecreateRendererStreamCallback(int32_t callerPid, int32_t sessionId,
    uint32_t routeFlag, const AudioStreamDeviceChangeReasonExt::ExtEnum reason)
{
    Trace trace("AudioDeviceCommon::TriggerRecreateRendererStreamCallback");
    AUDIO_INFO_LOG("Trigger recreate renderer stream %{public}d, pid: %{public}d, routeflag: 0x%{public}x",
        sessionId, callerPid, routeFlag);
    if (audioPolicyServerHandler_ != nullptr) {
        audioPolicyServerHandler_->SendRecreateRendererStreamEvent(callerPid, sessionId, routeFlag, reason);
    } else {
        AUDIO_WARNING_LOG("No audio policy server handler");
    }
}

CapturerState AudioCoreService::HandleStreamStatusToCapturerState(AudioStreamStatus status)
{
    switch (status) {
        case STREAM_STATUS_NEW:
            return CAPTURER_PREPARED;
        case STREAM_STATUS_STARTED:
            return CAPTURER_RUNNING;
        case STREAM_STATUS_PAUSED:
            return CAPTURER_PAUSED;
        case STREAM_STATUS_STOPPED:
            return CAPTURER_STOPPED;
        case STREAM_STATUS_RELEASED:
            return CAPTURER_RELEASED;
        default:
            return CAPTURER_INVALID;
    }
}

void AudioCoreService::TriggerRecreateCapturerStreamCallback(shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    Trace trace("AudioCoreService::TriggerRecreateCapturerStreamCallback");
    AUDIO_INFO_LOG("Trigger recreate capturer stream %{public}d, pid: %{public}d, routeflag: 0x%{public}x",
        streamDesc->sessionId_, streamDesc->callerPid_, streamDesc->routeFlag_);

    SwitchStreamInfo info = {
        streamDesc->sessionId_,
        streamDesc->callerUid_,
        streamDesc->appInfo_.appUid,
        streamDesc->appInfo_.appPid,
        streamDesc->appInfo_.appTokenId,
        HandleStreamStatusToCapturerState(streamDesc->streamStatus_),
    };
    if (audioPolicyServerHandler_ != nullptr) {
        SwitchStreamUtil::UpdateSwitchStreamRecord(info, SWITCH_STATE_WAITING);
        audioPolicyServerHandler_->SendRecreateCapturerStreamEvent(streamDesc->appInfo_.appPid,
            streamDesc->sessionId_, streamDesc->routeFlag_, AudioStreamDeviceChangeReasonExt::ExtEnum::UNKNOWN);
    } else {
        AUDIO_WARNING_LOG("No audio policy server handler");
    }
}

uint32_t AudioCoreService::OpenNewAudioPortAndRoute(std::shared_ptr<AudioPipeInfo> pipeInfo, uint32_t &paIndex)
{
    uint32_t id = OPEN_PORT_FAILURE;
    CHECK_AND_RETURN_RET_LOG(pipeInfo != nullptr && pipeInfo->streamDescriptors_.size() > 0 &&
        pipeInfo->streamDescriptors_.front() != nullptr, OPEN_PORT_FAILURE, "pipeInfo is invalid");
    if (pipeInfo->streamDescriptors_.front()->newDeviceDescs_.front()->deviceType_ == DEVICE_TYPE_REMOTE_CAST) {
        AUDIO_INFO_LOG("[PipeExecInfo] remote cast device do not need open pipe");
        id = pipeInfo->streamDescriptors_.front()->sessionId_;
    } else {
        HandleCommonSourceOpened(pipeInfo);
        id = audioPolicyManager_.OpenAudioPort(pipeInfo, paIndex);
        if (audioActiveDevice_.GetCurrentInputDeviceType() == DEVICE_TYPE_MIC ||
            audioActiveDevice_.GetCurrentInputDeviceType() == DEVICE_TYPE_ACCESSORY) {
            audioPolicyManager_.SetDeviceActive(audioActiveDevice_.GetCurrentInputDeviceType(),
                pipeInfo->moduleInfo_.name, true, INPUT_DEVICES_FLAG);
        }
    }
    CHECK_AND_RETURN_RET_LOG(id != HDI_INVALID_ID, ERR_INVALID_HANDLE,
        "OpenAudioPort failed ioHandle[%{public}u]", id);
    CHECK_AND_RETURN_RET_LOG(paIndex != OPEN_PORT_FAILURE, ERR_OPERATION_FAILED,
        "OpenAudioPort failed paId[%{public}u]", paIndex);
    audioIOHandleMap_.AddIOHandleInfo(pipeInfo->moduleInfo_.name, id);
    AUDIO_INFO_LOG("[PipeExecInfo] Get HDI id: %{public}u, paIndex %{public}u", id, paIndex);
    return id;
}

bool AudioCoreService::IsPaRoute(uint32_t routeFlag)
{
    if ((routeFlag & AUDIO_OUTPUT_FLAG_DIRECT) ||
        (routeFlag & AUDIO_OUTPUT_FLAG_FAST) ||
        (routeFlag & AUDIO_INPUT_FLAG_FAST)) {
        return false;
    }
    return true;
}

int32_t AudioCoreService::HandleScoOutputDeviceFetched(
    shared_ptr<AudioDeviceDescriptor> &desc, const AudioStreamDeviceChangeReasonExt reason)
{
    AUDIO_INFO_LOG("In");
    Trace trace("AudioCoreService::HandleScoOutputDeviceFetched");
#ifdef BLUETOOTH_ENABLE
    int32_t ret = Bluetooth::AudioHfpManager::SetActiveHfpDevice(desc->macAddress_);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Active hfp device failed, retrigger fetch output device.");
        desc->exceptionFlag_ = true;
        audioDeviceManager_.UpdateDevicesListInfo(
            std::make_shared<AudioDeviceDescriptor>(*desc), EXCEPTION_FLAG_UPDATE);
        FetchOutputDeviceAndRoute(reason);
        return ERROR;
    }
    Bluetooth::AudioHfpManager::UpdateAudioScene(audioSceneManager_.GetAudioScene(true));
#endif
    AUDIO_INFO_LOG("out");
    return SUCCESS;
}

int32_t AudioCoreService::HandleScoOutputDeviceFetched(
    shared_ptr<AudioStreamDescriptor> &streamDesc, const AudioStreamDeviceChangeReasonExt reason)
{
    AUDIO_INFO_LOG("In");
    Trace trace("AudioCoreService::HandleScoOutputDeviceFetched");
#ifdef BLUETOOTH_ENABLE
    std::shared_ptr<AudioDeviceDescriptor> desc = streamDesc->newDeviceDescs_.front();
    int32_t ret = Bluetooth::AudioHfpManager::SetActiveHfpDevice(desc->macAddress_);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("Active hfp device failed, retrigger fetch output device.");
        desc->exceptionFlag_ = true;
        audioDeviceManager_.UpdateDevicesListInfo(
            std::make_shared<AudioDeviceDescriptor>(*desc), EXCEPTION_FLAG_UPDATE);
        FetchOutputDeviceAndRoute(reason);
        return ERROR;
    }
    if (streamDesc->streamStatus_ == STREAM_STATUS_STARTED) {
        Bluetooth::AudioHfpManager::UpdateAudioScene(audioSceneManager_.GetAudioScene(true));
    }
#endif
    AUDIO_INFO_LOG("out");
    return SUCCESS;
}

int32_t AudioCoreService::GetRealUid(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    if (streamDesc->callerUid_ == MEDIA_SERVICE_UID) {
        return streamDesc->appInfo_.appUid;
    }
    return streamDesc->callerUid_;
}

void AudioCoreService::UpdateRendererInfoWhenNoPermission(
    const shared_ptr<AudioRendererChangeInfo> &audioRendererChangeInfos, bool hasSystemPermission)
{
    if (!hasSystemPermission) {
        audioRendererChangeInfos->clientUID = 0;
        audioRendererChangeInfos->rendererState = RENDERER_INVALID;
    }
}

void AudioCoreService::UpdateCapturerInfoWhenNoPermission(
    const shared_ptr<AudioCapturerChangeInfo> &audioCapturerChangeInfos, bool hasSystemPermission)
{
    if (!hasSystemPermission) {
        audioCapturerChangeInfos->clientUID = 0;
        audioCapturerChangeInfos->capturerState = CAPTURER_INVALID;
    }
}

void AudioCoreService::SendA2dpConnectedWhileRunning(const RendererState &rendererState, const uint32_t &sessionId)
{
    if ((rendererState == RENDERER_RUNNING) && (audioA2dpOffloadManager_ != nullptr) &&
        !audioA2dpOffloadManager_->IsA2dpOffloadConnecting(sessionId)) {
        AUDIO_INFO_LOG("Notify client not to block.");
        std::thread sendConnectedToClient(&AudioCoreService::UpdateSessionConnectionState, this, sessionId,
            DATA_LINK_CONNECTED);
        sendConnectedToClient.detach();
    }
}

void AudioCoreService::UpdateSessionConnectionState(const int32_t &sessionID, const int32_t &state)
{
    AudioServerProxy::GetInstance().UpdateSessionConnectionStateProxy(sessionID, state);
}

void AudioCoreService::UpdateTrackerDeviceChange(const vector<std::shared_ptr<AudioDeviceDescriptor>> &desc)
{
    AUDIO_INFO_LOG("Start");

    DeviceType curOutputDeviceType = audioActiveDevice_.GetCurrentOutputDeviceType();
    for (std::shared_ptr<AudioDeviceDescriptor> deviceDesc : desc) {
        if (deviceDesc->deviceRole_ == OUTPUT_DEVICE) {
            DeviceType type = curOutputDeviceType;
            std::string macAddress = audioActiveDevice_.GetCurrentOutputDeviceMacAddr();
            auto itr = audioConnectedDevice_.CheckExistOutputDevice(type, macAddress);
            if (itr != nullptr) {
                AudioDeviceDescriptor outputDevice(AudioDeviceDescriptor::DEVICE_INFO);
                audioDeviceCommon_.UpdateDeviceInfo(outputDevice, itr, true, true);
                streamCollector_.UpdateTracker(AUDIO_MODE_PLAYBACK, outputDevice);
            }
        }
        if (deviceDesc->deviceRole_ == INPUT_DEVICE) {
            DeviceType type = audioActiveDevice_.GetCurrentInputDeviceType();
            auto itr = audioConnectedDevice_.CheckExistInputDevice(type);
            if (itr != nullptr) {
                AudioDeviceDescriptor inputDevice(AudioDeviceDescriptor::DEVICE_INFO);
                audioDeviceCommon_.UpdateDeviceInfo(inputDevice, itr, true, true);
                audioMicrophoneDescriptor_.UpdateAudioCapturerMicrophoneDescriptor(itr->deviceType_);
                streamCollector_.UpdateTracker(AUDIO_MODE_RECORD, inputDevice);
            }
        }
    }
}

bool AudioCoreService::GetFastControlParam()
{
    int32_t fastControlFlag = 1; // default 1, set isFastControlled_ true
    GetSysPara("persist.multimedia.audioflag.fastcontrolled", fastControlFlag);
    if (fastControlFlag == 0) {
        isFastControlled_ = false;
    }
    return isFastControlled_;
}

void AudioCoreService::StoreDistributedRoutingRoleInfo(
    const std::shared_ptr<AudioDeviceDescriptor> descriptor, CastType type)
{
    distributedRoutingInfo_.descriptor = descriptor;
    distributedRoutingInfo_.type = type;
}

int32_t AudioCoreService::GetSystemVolumeLevel(AudioStreamType streamType)
{
    return audioVolumeManager_.GetSystemVolumeLevel(streamType);
}

float AudioCoreService::GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel,
    DeviceType deviceType) const
{
    return audioPolicyManager_.GetSystemVolumeInDb(volumeType, volumeLevel, deviceType);
}

bool AudioCoreService::IsStreamSupportLowpower(std::shared_ptr<AudioStreamDescriptor> streamDesc)
{
    Trace trace("IsStreamSupportLowpower");
    if (pipeManager_->PcmOffloadSessionCount() > 0) {
        AUDIO_INFO_LOG("PIPE_TYPE_OFFLOAD already exist.");
        return false;
    }
    if (!streamDesc->rendererInfo_.isOffloadAllowed) {
        AUDIO_INFO_LOG("normal stream beacuse renderInfo not support offload.");
        return false;
    }
    if (streamDesc->streamInfo_.channels > STEREO &&
        (streamDesc->rendererInfo_.streamUsage != STREAM_USAGE_MOVIE ||
         streamDesc->rendererInfo_.originalFlag != AUDIO_FLAG_PCM_OFFLOAD)) {
        AUDIO_INFO_LOG("normal stream beacuse channels.");
        return false;
    }

    if (streamDesc->rendererInfo_.streamUsage != STREAM_USAGE_MUSIC &&
        streamDesc->rendererInfo_.streamUsage != STREAM_USAGE_AUDIOBOOK &&
        (streamDesc->rendererInfo_.streamUsage != STREAM_USAGE_MOVIE ||
         streamDesc->rendererInfo_.originalFlag != AUDIO_FLAG_PCM_OFFLOAD)) {
        AUDIO_INFO_LOG("normal stream beacuse streamUsage.");
        return false;
    }

    AudioSpatializationState spatialState =
        AudioSpatializationService::GetAudioSpatializationService().GetSpatializationState();
    bool effectOffloadFlag = AudioServerProxy::GetInstance().GetEffectOffloadEnabledProxy();
    if (spatialState.spatializationEnabled && !effectOffloadFlag) {
        AUDIO_INFO_LOG("spatialization effect in arm, Skipped.");
        return false;
    }

    if (streamDesc->newDeviceDescs_[0]->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP) {
        // a2dp offload
        return true;
    }

    // LowPower: Speaker, USB headset, a2dp offload
    if (streamDesc->newDeviceDescs_[0]->deviceType_ != DEVICE_TYPE_SPEAKER &&
        streamDesc->newDeviceDescs_[0]->deviceType_ != DEVICE_TYPE_USB_HEADSET &&
        (streamDesc->newDeviceDescs_[0]->deviceType_ != DEVICE_TYPE_BLUETOOTH_A2DP ||
        streamDesc->newDeviceDescs_[0]->a2dpOffloadFlag_ != A2DP_OFFLOAD)) {
        AUDIO_INFO_LOG("normal stream, deviceType: %{public}d", streamDesc->newDeviceDescs_[0]->deviceType_);
        return false;
    }
    return true;
}

int32_t AudioCoreService::SetDefaultOutputDevice(const DeviceType deviceType, const uint32_t sessionID,
    const StreamUsage streamUsage, bool isRunning)
{
    CHECK_AND_RETURN_RET_LOG(policyConfigMananger_.GetHasEarpiece(), ERR_NOT_SUPPORTED, "the device has no earpiece");
    CHECK_AND_RETURN_RET_LOG(pipeManager_->GetStreamDescById(sessionID) != nullptr, ERR_NOT_SUPPORTED,
        "sessionId is not exist");

    AUDIO_INFO_LOG("[ADeviceEvent] device %{public}d for %{public}s stream %{public}u", deviceType,
        isRunning ? "running" : "not running", sessionID);
    int32_t ret = audioDeviceManager_.SetDefaultOutputDevice(deviceType, sessionID, streamUsage, isRunning);
    if (ret == NEED_TO_FETCH) {
        FetchOutputDeviceAndRoute(AudioStreamDeviceChangeReasonExt::ExtEnum::SET_DEFAULT_OUTPUT_DEVICE);
        return SUCCESS;
    }
    return ret;
}

int32_t AudioCoreService::HandleFetchOutputWhenNoRunningStream()
{
    AUDIO_PRERELEASE_LOGI("No running stream need update several device state");
    vector<std::shared_ptr<AudioDeviceDescriptor>> descs =
        audioRouterCenter_.FetchOutputDevices(STREAM_USAGE_MEDIA, -1);
    CHECK_AND_RETURN_RET_LOG(!descs.empty(), ERROR, "descs is empty");
    AudioDeviceDescriptor tmpOutputDeviceDesc = audioActiveDevice_.GetCurrentOutputDevice();
    if (descs.front()->deviceType_ == DEVICE_TYPE_NONE || IsSameDevice(descs.front(), tmpOutputDeviceDesc)) {
        AUDIO_DEBUG_LOG("output device is not change");
        return SUCCESS;
    }
    audioActiveDevice_.SetCurrentOutputDevice(*descs.front());
    AUDIO_DEBUG_LOG("currentActiveDevice %{public}d", audioActiveDevice_.GetCurrentOutputDeviceType());
    audioVolumeManager_.SetVolumeForSwitchDevice(*descs.front());
    if (descs.front()->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP) {
        SwitchActiveA2dpDevice(std::make_shared<AudioDeviceDescriptor>(*descs.front()));
    }
    OnPreferredOutputDeviceUpdated(audioActiveDevice_.GetCurrentOutputDevice());
    return SUCCESS;
}

int32_t AudioCoreService::HandleFetchInputWhenNoRunningStream()
{
    AUDIO_PRERELEASE_LOGI("No running stream need update several device state");
    std::shared_ptr<AudioDeviceDescriptor> desc;
    AudioDeviceDescriptor tempDesc = audioActiveDevice_.GetCurrentInputDevice();
    if (tempDesc.deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO && Bluetooth::AudioHfpManager::IsRecognitionStatus()) {
        desc = audioRouterCenter_.FetchInputDevice(SOURCE_TYPE_VOICE_RECOGNITION, -1);
    } else {
        desc = audioRouterCenter_.FetchInputDevice(SOURCE_TYPE_MIC, -1);
    }
    CHECK_AND_RETURN_RET_LOG(desc != nullptr, ERROR, "desc is nullptr");

    if (desc->deviceType_ == DEVICE_TYPE_NONE || IsSameDevice(desc, tempDesc)) {
        AUDIO_DEBUG_LOG("input device is not change");
        return SUCCESS;
    }
    audioActiveDevice_.SetCurrentInputDevice(*desc);
    if (desc->deviceType_ == DEVICE_TYPE_USB_ARM_HEADSET) {
        audioEcManager_.PresetArmIdleInput(desc->macAddress_);
    }
    DeviceType deviceType = audioActiveDevice_.GetCurrentInputDeviceType();
    AUDIO_DEBUG_LOG("currentActiveInputDevice update %{public}d", deviceType);
    OnPreferredInputDeviceUpdated(deviceType, ""); // networkId is not used
    return SUCCESS;
}

bool AudioCoreService::UpdateOutputDevice(std::shared_ptr<AudioDeviceDescriptor> &desc, int32_t uid,
    const AudioStreamDeviceChangeReasonExt reason)
{
    std::shared_ptr<AudioDeviceDescriptor> preferredDesc = audioAffinityManager_.GetRendererDevice(uid);
    AudioDeviceDescriptor tmpOutputDeviceDesc = audioActiveDevice_.GetCurrentOutputDevice();
    if (((preferredDesc->deviceType_ != DEVICE_TYPE_NONE) && !desc->IsSameDeviceInfo(tmpOutputDeviceDesc)
        && desc->deviceType_ != preferredDesc->deviceType_)
        || ((preferredDesc->deviceType_ == DEVICE_TYPE_NONE) && !desc->IsSameDeviceInfo(tmpOutputDeviceDesc))) {
        WriteOutputRouteChangeEvent(desc, reason);
        audioActiveDevice_.SetCurrentOutputDevice(*desc);
        AUDIO_DEBUG_LOG("currentActiveDevice update %{public}d", audioActiveDevice_.GetCurrentOutputDeviceType());
        return true;
    }
    return false;
}

bool AudioCoreService::UpdateInputDevice(std::shared_ptr<AudioDeviceDescriptor> &desc, int32_t uid,
    const AudioStreamDeviceChangeReasonExt reason)
{
    std::shared_ptr<AudioDeviceDescriptor> preferredDesc = audioAffinityManager_.GetCapturerDevice(uid);
    if (((preferredDesc->deviceType_ != DEVICE_TYPE_NONE) &&
        !IsSameDevice(desc, audioActiveDevice_.GetCurrentInputDevice())
        && desc->deviceType_ != preferredDesc->deviceType_)
        || ((preferredDesc->deviceType_ == DEVICE_TYPE_NONE)
        && !IsSameDevice(desc, audioActiveDevice_.GetCurrentInputDevice()))) {
        WriteInputRouteChangeEvent(desc, reason);
        audioActiveDevice_.SetCurrentInputDevice(*desc);
        AUDIO_DEBUG_LOG("currentActiveInputDevice update %{public}d",
            audioActiveDevice_.GetCurrentInputDeviceType());
        return true;
    }
    return false;
}

void AudioCoreService::WriteOutputRouteChangeEvent(std::shared_ptr<AudioDeviceDescriptor> &desc,
    const AudioStreamDeviceChangeReason reason)
{
    int64_t timeStamp = AudioPolicyUtils::GetInstance().GetCurrentTimeMS();
    std::shared_ptr<Media::MediaMonitor::EventBean> bean = std::make_shared<Media::MediaMonitor::EventBean>(
        Media::MediaMonitor::AUDIO, Media::MediaMonitor::AUDIO_ROUTE_CHANGE,
        Media::MediaMonitor::BEHAVIOR_EVENT);
    DeviceType curOutputDeviceType = audioActiveDevice_.GetCurrentOutputDeviceType();
    bean->Add("REASON", static_cast<int32_t>(reason));
    bean->Add("TIMESTAMP", static_cast<uint64_t>(timeStamp));
    bean->Add("DEVICE_TYPE_BEFORE_CHANGE", curOutputDeviceType);
    bean->Add("DEVICE_TYPE_AFTER_CHANGE", desc->deviceType_);
    Media::MediaMonitor::MediaMonitorManager::GetInstance().WriteLogMsg(bean);
}

void AudioCoreService::WriteInputRouteChangeEvent(std::shared_ptr<AudioDeviceDescriptor> &desc,
    const AudioStreamDeviceChangeReason reason)
{
    int64_t timeStamp = AudioPolicyUtils::GetInstance().GetCurrentTimeMS();
    std::shared_ptr<Media::MediaMonitor::EventBean> bean = std::make_shared<Media::MediaMonitor::EventBean>(
        Media::MediaMonitor::AUDIO, Media::MediaMonitor::AUDIO_ROUTE_CHANGE,
        Media::MediaMonitor::BEHAVIOR_EVENT);
    bean->Add("REASON", static_cast<int32_t>(reason));
    bean->Add("TIMESTAMP", static_cast<uint64_t>(timeStamp));
    bean->Add("DEVICE_TYPE_BEFORE_CHANGE", audioActiveDevice_.GetCurrentInputDeviceType());
    bean->Add("DEVICE_TYPE_AFTER_CHANGE", desc->deviceType_);
    Media::MediaMonitor::MediaMonitorManager::GetInstance().WriteLogMsg(bean);
}

int32_t AudioCoreService::HandleDeviceChangeForFetchOutputDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    if (streamDesc->oldDeviceDescs_.size() == 0) {
        AUDIO_INFO_LOG("No old device info");
        return SUCCESS;
    }
    std::shared_ptr<AudioDeviceDescriptor> desc = streamDesc->newDeviceDescs_.front();

    if (desc->deviceType_ == DEVICE_TYPE_NONE || (IsSameDevice(desc, streamDesc->oldDeviceDescs_.front()) &&
        !NeedRehandleA2DPDevice(desc) && desc->connectState_ != DEACTIVE_CONNECTED &&
        audioSceneManager_.IsSameAudioScene() && !shouldUpdateDeviceDueToDualTone_)) {
        AUDIO_WARNING_LOG("stream %{public}d device not change, no need move device", streamDesc->sessionId_);
        AudioDeviceDescriptor tmpOutputDeviceDesc = audioActiveDevice_.GetCurrentOutputDevice();
        std::shared_ptr<AudioDeviceDescriptor> preferredDesc =
            audioAffinityManager_.GetRendererDevice(GetRealUid(streamDesc));
        if (((preferredDesc->deviceType_ != DEVICE_TYPE_NONE) && !IsSameDevice(desc, tmpOutputDeviceDesc)
            && desc->deviceType_ != preferredDesc->deviceType_)
            || ((preferredDesc->deviceType_ == DEVICE_TYPE_NONE) && !IsSameDevice(desc, tmpOutputDeviceDesc))) {
            audioActiveDevice_.SetCurrentOutputDevice(*desc);
            AudioDeviceDescriptor curOutputDevice = audioActiveDevice_.GetCurrentOutputDevice();
            audioVolumeManager_.SetVolumeForSwitchDevice(curOutputDevice);
            audioActiveDevice_.UpdateActiveDeviceRoute(curOutputDevice.deviceType_, DeviceFlag::OUTPUT_DEVICES_FLAG);
            OnPreferredOutputDeviceUpdated(audioActiveDevice_.GetCurrentOutputDevice());
        }
        return ERR_NEED_NOT_SWITCH_DEVICE;
    }
    return SUCCESS;
}

int32_t AudioCoreService::HandleDeviceChangeForFetchInputDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    if (streamDesc->oldDeviceDescs_.size() == 0) {
        AUDIO_INFO_LOG("No old device info");
        return SUCCESS;
    }
    std::shared_ptr<AudioDeviceDescriptor> desc = streamDesc->newDeviceDescs_.front();
    std::shared_ptr<AudioDeviceDescriptor> oldDeviceDesc = streamDesc->oldDeviceDescs_.front();

    if (desc->deviceType_ == DEVICE_TYPE_NONE ||
        (IsSameDevice(desc, oldDeviceDesc) && desc->connectState_ != DEACTIVE_CONNECTED)) {
        AUDIO_WARNING_LOG("stream %{public}d device not change, no need move device", streamDesc->sessionId_);
        AudioDeviceDescriptor tempDesc = audioActiveDevice_.GetCurrentInputDevice();
        std::shared_ptr<AudioDeviceDescriptor> preferredDesc =
            audioAffinityManager_.GetCapturerDevice(GetRealUid(streamDesc));
        if (((preferredDesc->deviceType_ != DEVICE_TYPE_NONE) && !IsSameDevice(desc, tempDesc) &&
            desc->deviceType_ != preferredDesc->deviceType_) ||
            IsSameDevice(desc, oldDeviceDesc)) {
            audioActiveDevice_.SetCurrentInputDevice(*desc);
            // networkId is not used.
            OnPreferredInputDeviceUpdated(audioActiveDevice_.GetCurrentInputDeviceType(), "");
            audioActiveDevice_.UpdateActiveDeviceRoute(audioActiveDevice_.GetCurrentInputDeviceType(),
                DeviceFlag::INPUT_DEVICES_FLAG, audioActiveDevice_.GetCurrentInputDevice().deviceName_);
        }
        return ERR_NEED_NOT_SWITCH_DEVICE;
    }
    return SUCCESS;
}

bool AudioCoreService::NeedRehandleA2DPDevice(std::shared_ptr<AudioDeviceDescriptor> &desc)
{
    if (desc->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP
        && audioIOHandleMap_.CheckIOHandleExist(BLUETOOTH_SPEAKER) == false) {
        AUDIO_WARNING_LOG("A2DP module is not loaded, need rehandle");
        return true;
    }
    return false;
}

bool AudioCoreService::IsDeviceSwitching(const AudioStreamDeviceChangeReasonExt reason)
{
    return reason.IsOverride() || reason.IsOldDeviceUnavaliable() || reason.IsNewDeviceAvailable();
}

void AudioCoreService::UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
    RendererState rendererState)
{
    const StreamUsage streamUsage = streamChangeInfo.audioRendererChangeInfo.rendererInfo.streamUsage;

    if (mode == AUDIO_MODE_PLAYBACK && (rendererState == RENDERER_STOPPED || rendererState == RENDERER_PAUSED ||
        rendererState == RENDERER_RELEASED)) {
        audioDeviceManager_.UpdateDefaultOutputDeviceWhenStopping(streamChangeInfo.audioRendererChangeInfo.sessionId);
        if (rendererState == RENDERER_RELEASED) {
            audioDeviceManager_.RemoveSelectedDefaultOutputDevice(streamChangeInfo.audioRendererChangeInfo.sessionId);
        }
        bool isRemoved = true;
        sleAudioDeviceManager_.UpdateSleStreamTypeCount(pipeManager_->GetStreamDescById(
            streamChangeInfo.audioRendererChangeInfo.sessionId), isRemoved);
        FetchOutputDeviceAndRoute();
    }

    const auto &capturerState = streamChangeInfo.audioCapturerChangeInfo.capturerState;
    if (mode == AUDIO_MODE_RECORD && capturerState == CAPTURER_RELEASED) {
        AUDIO_INFO_LOG("[ADeviceEvent] fetch device for capturer stream %{public}d released",
            streamChangeInfo.audioCapturerChangeInfo.sessionId);
        audioDeviceManager_.RemoveSelectedInputDevice(streamChangeInfo.audioCapturerChangeInfo.sessionId);
        FetchInputDeviceAndRoute();
    }

    if (enableDualHalToneState_ && (mode == AUDIO_MODE_PLAYBACK)
        && (rendererState == RENDERER_STOPPED || rendererState == RENDERER_RELEASED)) {
        const int32_t sessionId = streamChangeInfo.audioRendererChangeInfo.sessionId;
        if ((sessionId == enableDualHalToneSessionId_) && Util::IsRingerOrAlarmerStreamUsage(streamUsage)) {
            AUDIO_INFO_LOG("disable dual hal tone when ringer/alarm renderer stop/release.");
            UpdateDualToneState(false, enableDualHalToneSessionId_);
        }
    }

    AUDIO_INFO_LOG("isRingDualToneOnPrimarySpeaker: %{public}d , usage: %{public}d",
        isRingDualToneOnPrimarySpeaker_, streamUsage);
    if (isRingDualToneOnPrimarySpeaker_ && AudioCoreServiceUtils::IsOverRunPlayback(mode, rendererState) &&
        Util::IsRingerOrAlarmerStreamUsage(streamUsage)) {
        AUDIO_INFO_LOG("[ADeviceEvent] disable primary speaker dual tone when ringer renderer run over");
        isRingDualToneOnPrimarySpeaker_ = false;
        // Add delay between end of double ringtone and device switch.
        // After the ringtone ends, there may still be residual audio data in the pipeline.
        // Switching the device immediately can cause pop noise due the undrained buffers.
        usleep(RING_DUAL_END_DELAY_US);
        FetchOutputDeviceAndRoute();
        for (std::pair<AudioStreamType, StreamUsage> stream :  streamsWhenRingDualOnPrimarySpeaker_) {
            audioPolicyManager_.SetInnerStreamMute(stream.first, false, stream.second);
        }
        streamsWhenRingDualOnPrimarySpeaker_.clear();
        audioPolicyManager_.SetInnerStreamMute(STREAM_MUSIC, false, STREAM_USAGE_MUSIC);
    }
}

void AudioCoreService::HandleCommonSourceOpened(std::shared_ptr<AudioPipeInfo> &pipeInfo)
{
    if (pipeInfo->pipeRole_ != PIPE_ROLE_INPUT || pipeInfo->streamDescriptors_.size() == 0) {
        return;
    }
    auto streamDesc = pipeInfo->streamDescriptors_.front();
    CHECK_AND_RETURN_LOG(streamDesc != nullptr, "streamDesc is null");
    SourceType sourceType = streamDesc->capturerInfo_.sourceType;
    if (specialSourceTypeSet_.count(sourceType) == 0) {
        audioEcManager_.PrepareNormalSource(pipeInfo->moduleInfo_, streamDesc);
    }
}

void AudioCoreService::DelayReleaseOffloadPipe(AudioIOHandle id, uint32_t paIndex)
{
    AUDIO_INFO_LOG("In");
    CHECK_AND_RETURN_LOG(isOffloadOpened_.load(), "Offload is already released");
    isOffloadOpened_.store(false);
    auto unloadOffloadThreadFuc = [this, id, paIndex] { this->ReleaseOffloadPipe(id, paIndex); };
    std::thread unloadOffloadThread(unloadOffloadThreadFuc);
    unloadOffloadThread.detach();
}

int32_t AudioCoreService::ReleaseOffloadPipe(AudioIOHandle id, uint32_t paIndex)
{
    AUDIO_INFO_LOG("unload offload module");
    std::unique_lock<std::mutex> lock(offloadCloseMutex_);
    // Try to wait 10 seconds before unloading the module, because the audio driver takes some time to process
    // the shutdown process..
    offloadCloseCondition_.wait_for(lock, std::chrono::seconds(WAIT_OFFLOAD_CLOSE_TIME_SEC), [this] () {
        return isOffloadOpened_.load();
    });

    {
        std::lock_guard<std::mutex> lk(offloadReOpenMutex_);
        AUDIO_INFO_LOG("After wait, isOffloadOpened: %{public}d", isOffloadOpened_.load());
        if (isOffloadOpened_.load()) {
            AUDIO_INFO_LOG("offload restart");
            return ERROR;
        }
        AUDIO_INFO_LOG("Close hdi port id: %{public}u, index %{public}u", id, paIndex);
        audioPolicyManager_.CloseAudioPort(id, paIndex);
        pipeManager_->RemoveAudioPipeInfo(id);
    }
    return SUCCESS;
}

void AudioCoreService::CheckOffloadStream(AudioStreamChangeInfo &streamChangeInfo)
{
    std::string adapterName = GetAdapterNameBySessionId(streamChangeInfo.audioRendererChangeInfo.sessionId);
    AUDIO_INFO_LOG("session: %{public}u, adapter name: %{public}s",
        streamChangeInfo.audioRendererChangeInfo.sessionId, adapterName.c_str());
    if (adapterName != OFFLOAD_PRIMARY_SPEAKER) {
        return;
    }

    if (streamChangeInfo.audioRendererChangeInfo.rendererState == RENDERER_PAUSED ||
        streamChangeInfo.audioRendererChangeInfo.rendererState == RENDERER_STOPPED ||
        streamChangeInfo.audioRendererChangeInfo.rendererState == RENDERER_RELEASED) {
        audioOffloadStream_.ResetOffloadStatus(streamChangeInfo.audioRendererChangeInfo.sessionId);
    }
    if (streamChangeInfo.audioRendererChangeInfo.rendererState == RENDERER_RUNNING) {
        audioOffloadStream_.SetOffloadStatus(streamChangeInfo.audioRendererChangeInfo.sessionId);
    }
}

void AudioCoreService::ReConfigOffloadStatus(uint32_t sessionId,
    std::shared_ptr<AudioPipeInfo> &pipeInfo, std::string &oldSinkName)
{
    AUDIO_INFO_LOG("new sink: %{public}s, old sink: %{public}s, sessionId: %{public}u",
        pipeInfo->moduleInfo_.name.c_str(), oldSinkName.c_str(), sessionId);
    if (pipeInfo->moduleInfo_.name == OFFLOAD_PRIMARY_SPEAKER) {
        audioOffloadStream_.SetOffloadStatus(sessionId);
    } else if (oldSinkName == OFFLOAD_PRIMARY_SPEAKER) {
        audioOffloadStream_.ResetOffloadStatus(sessionId);
    }
}

void AudioCoreService::PrepareMoveAttrs(std::shared_ptr<AudioStreamDescriptor> &streamDesc, DeviceType &oldDeviceType,
    bool &isNeedTriggerCallback, std::string &oldSinkName, const AudioStreamDeviceChangeReasonExt reason)
{
    std::shared_ptr<AudioDeviceDescriptor> newDeviceDesc = streamDesc->newDeviceDescs_.front();
    oldDeviceType = streamDesc->oldDeviceDescs_.front()->deviceType_;
    if (streamDesc->oldDeviceDescs_.front()->IsSameDeviceDesc(newDeviceDesc)) {
        isNeedTriggerCallback = false;
    }
    oldSinkName = AudioPolicyUtils::GetInstance().GetSinkName(streamDesc->oldDeviceDescs_.front(),
        streamDesc->sessionId_);

    AUDIO_INFO_LOG("[StreamExecInfo] Move stream %{public}u, [%{public}d][%{public}s] to [%{public}d][%{public}s]" \
        " reason %{public}d",
        streamDesc->sessionId_, streamDesc->oldDeviceDescs_.front()->deviceType_,
        GetEncryptAddr(streamDesc->oldDeviceDescs_.front()->macAddress_).c_str(), newDeviceDesc->deviceType_,
        GetEncryptAddr(newDeviceDesc->macAddress_).c_str(), static_cast<int32_t>(reason));
}

void AudioCoreService::MuteSinkForSwitchGeneralDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    CHECK_AND_RETURN_LOG(streamDesc->oldDeviceDescs_.size() > 0, "No old device(s)");
    if (streamDesc->newDeviceDescs_.front() != nullptr &&
        streamDesc->newDeviceDescs_.front()->deviceType_ != DEVICE_TYPE_BLUETOOTH_A2DP &&
        streamDesc->newDeviceDescs_.front()->deviceType_ != DEVICE_TYPE_BLUETOOTH_SCO) {
        MuteSinkPortForSwitchDevice(streamDesc, reason);
    }
}

void AudioCoreService::MuteSinkForSwitchBluetoothDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    if (streamDesc->newDeviceDescs_.front() != nullptr &&
        (streamDesc->newDeviceDescs_.front()->deviceType_ == DEVICE_TYPE_BLUETOOTH_A2DP ||
        streamDesc->newDeviceDescs_.front()->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO)) {
        MuteSinkPortForSwitchDevice(streamDesc, reason);
    }
}

void AudioCoreService::MuteSinkPortForSwitchDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    Trace trace("AudioCoreService::MuteSinkPortForSwitchDevice");
    // After media playback is interrupted by the alarm,
    // a delay is required before switching to dual output (e.g., speaker + headset).
    // This ensures that the remaining audio buffer is drained,
    // preventing any residual media sound from leaking through the speaker.
    if (streamDesc != nullptr && streamDesc->newDeviceDescs_.size() > 1 && !IsDeviceSwitching(reason) &&
        streamCollector_.IsMediaPlaying() && streamDesc->rendererInfo_.streamUsage == STREAM_USAGE_ALARM) {
        usleep(MEDIA_PAUSE_TO_DOUBLE_RING_DELAY_US);
        return;
    }
    if (streamDesc->newDeviceDescs_.front()->IsSameDeviceDesc(streamDesc->oldDeviceDescs_.front())) {
        return;
    }

    audioIOHandleMap_.SetMoveFinish(false);

    if (!isVoiceCallMuted_ && audioSceneManager_.GetAudioScene(true) == AUDIO_SCENE_PHONE_CALL) {
        if (streamDesc->rendererInfo_.streamUsage == STREAM_USAGE_VOICE_MODEM_COMMUNICATION &&
            audioSceneManager_.CheckVoiceCallActive(streamDesc->sessionId_)) {
            isVoiceCallMuted_ = true;
            return SetVoiceCallMuteForSwitchDevice();
        } else if (streamCollector_.IsVoiceCallActive() && IsDeviceSwitching(reason)) {
            isVoiceCallMuted_ = true;
            SetVoiceCallMuteForSwitchDevice();
        }
    }

    std::string oldSinkName = AudioPolicyUtils::GetInstance().GetSinkName(streamDesc->oldDeviceDescs_.front(),
        streamDesc->sessionId_);
    std::string newSinkName = AudioPolicyUtils::GetInstance().GetSinkName(*streamDesc->newDeviceDescs_.front(),
        streamDesc->sessionId_);
    AUDIO_INFO_LOG("mute sink old:[%{public}s] new:[%{public}s]", oldSinkName.c_str(), newSinkName.c_str());
    MuteSinkPort(oldSinkName, newSinkName, reason);
}

void AudioCoreService::SetVoiceCallMuteForSwitchDevice()
{
    Trace trace("AudioCoreService::SetVoiceMuteForSwitchDevice");
    AudioServerProxy::GetInstance().SetVoiceVolumeProxy(0);

    AUDIO_INFO_LOG("%{public}" PRId64" us for modem call update route", WAIT_MODEM_CALL_SET_VOLUME_TIME_US);
    usleep(WAIT_MODEM_CALL_SET_VOLUME_TIME_US);
    // Unmute in SetVolumeForSwitchDevice after update route.
}

void AudioCoreService::MuteSinkPort(const std::string &oldSinkName, const std::string &newSinkName,
    AudioStreamDeviceChangeReasonExt reason)
{
    if (reason.IsOverride() || reason.IsSetDefaultOutputDevice()) {
        int64_t muteTime = SELECT_DEVICE_MUTE_MS;
        if (newSinkName == OFFLOAD_PRIMARY_SPEAKER || oldSinkName == OFFLOAD_PRIMARY_SPEAKER) {
            muteTime = SELECT_OFFLOAD_DEVICE_MUTE_MS;
        }
        MutePrimaryOrOffloadSink(newSinkName, muteTime);
        audioIOHandleMap_.MuteSinkPort(newSinkName, SELECT_DEVICE_MUTE_MS, true);
        audioIOHandleMap_.MuteSinkPort(oldSinkName, muteTime, true);
    } else if (reason == AudioStreamDeviceChangeReason::NEW_DEVICE_AVAILABLE) {
        int64_t muteTime = NEW_DEVICE_AVALIABLE_MUTE_MS;
        if (newSinkName == OFFLOAD_PRIMARY_SPEAKER || oldSinkName == OFFLOAD_PRIMARY_SPEAKER) {
            muteTime = NEW_DEVICE_AVALIABLE_OFFLOAD_MUTE_MS;
        }
        MutePrimaryOrOffloadSink(oldSinkName, muteTime);
        audioIOHandleMap_.MuteSinkPort(newSinkName, NEW_DEVICE_AVALIABLE_MUTE_MS, true);
        audioIOHandleMap_.MuteSinkPort(oldSinkName, muteTime, true);
    }
    MuteSinkPortLogic(oldSinkName, newSinkName, reason);
}

void AudioCoreService::MutePrimaryOrOffloadSink(const std::string &sinkName, int64_t muteTime)
{
    // fix pop when switching devices during multiple concurrent streams
    if (sinkName == OFFLOAD_PRIMARY_SPEAKER) {
        audioIOHandleMap_.MuteSinkPort(PRIMARY_SPEAKER, muteTime, true);
    } else if (sinkName == PRIMARY_SPEAKER) {
        audioIOHandleMap_.MuteSinkPort(OFFLOAD_PRIMARY_SPEAKER, muteTime, true);
    }
}

void AudioCoreService::MuteSinkPortLogic(const std::string &oldSinkName, const std::string &newSinkName,
    AudioStreamDeviceChangeReasonExt reason)
{
    auto ringermode = audioPolicyManager_.GetRingerMode();
    AudioScene scene = audioSceneManager_.GetAudioScene(true);
    if (reason == DISTRIBUTED_DEVICE) {
        audioIOHandleMap_.MuteSinkPort(newSinkName, DISTRIBUTED_DEVICE_UNAVALIABLE_MUTE_MS, true);
        usleep(DISTRIBUTED_DEVICE_UNAVALIABLE_SLEEP_US);
    } else if (reason.IsOldDeviceUnavaliable() && ((scene == AUDIO_SCENE_DEFAULT) ||
        ((scene == AUDIO_SCENE_RINGING || scene == AUDIO_SCENE_VOICE_RINGING) &&
        ringermode != RINGER_MODE_NORMAL) || (scene == AUDIO_SCENE_PHONE_CHAT))) {
        MutePrimaryOrOffloadSink(newSinkName, OLD_DEVICE_UNAVALIABLE_MUTE_MS);
        audioIOHandleMap_.MuteSinkPort(newSinkName, OLD_DEVICE_UNAVALIABLE_MUTE_MS, true);
        usleep(OLD_DEVICE_UNAVALIABLE_MUTE_SLEEP_MS); // sleep fix data cache pop.
    } else if (reason.IsOldDeviceUnavaliableExt() && ((scene == AUDIO_SCENE_DEFAULT) ||
        ((scene == AUDIO_SCENE_RINGING || scene == AUDIO_SCENE_VOICE_RINGING) &&
        ringermode != RINGER_MODE_NORMAL) || (scene == AUDIO_SCENE_PHONE_CHAT))) {
        audioIOHandleMap_.MuteSinkPort(newSinkName, OLD_DEVICE_UNAVALIABLE_EXT_MUTE_MS, true);
        usleep(OLD_DEVICE_UNAVALIABLE_MUTE_SLEEP_MS); // sleep fix data cache pop.
    } else if (reason == AudioStreamDeviceChangeReason::UNKNOWN &&
        oldSinkName == REMOTE_CAST_INNER_CAPTURER_SINK_NAME) {
        // remote cast -> earpiece 300ms fix sound leak
        audioIOHandleMap_.MuteSinkPort(newSinkName, NEW_DEVICE_REMOTE_CAST_AVALIABLE_MUTE_MS, true);
    }
}

int32_t AudioCoreService::ActivateOutputDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    const AudioStreamDeviceChangeReasonExt reason)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr, ERR_NULL_POINTER, "Stream desc is nullptr");
    std::shared_ptr<AudioDeviceDescriptor> deviceDesc = streamDesc->newDeviceDescs_.front();

    std::string encryptMacAddr = GetEncryptAddr(deviceDesc->macAddress_);
    int32_t bluetoothFetchResult = BluetoothDeviceFetchOutputHandle(streamDesc, reason, encryptMacAddr);
    CHECK_AND_RETURN_RET(bluetoothFetchResult == BLUETOOTH_FETCH_RESULT_DEFAULT, ERR_OPERATION_FAILED);

    int32_t nearlinkFetchResult = ActivateNearlinkDevice(streamDesc);
    CHECK_AND_RETURN_RET_LOG(nearlinkFetchResult == SUCCESS, ERROR, "nearlink fetch output device failed");

    if (deviceDesc->deviceType_ == DEVICE_TYPE_USB_ARM_HEADSET) {
        audioEcManager_.ActivateArmDevice(deviceDesc->macAddress_, deviceDesc->deviceRole_);
    }
    return SUCCESS;
}

int32_t AudioCoreService::ActivateInputDevice(std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr && streamDesc->newDeviceDescs_.size() > 0 &&
        streamDesc->newDeviceDescs_[0] != nullptr, ERR_INVALID_PARAM, "Invalid stream desc");
    if (streamDesc->newDeviceDescs_[0]->deviceType_ == DEVICE_TYPE_BLUETOOTH_SCO) {
        BluetoothScoFetch(streamDesc);
    }

    int32_t nearlinkFetchResult = ActivateNearlinkDevice(streamDesc);
    CHECK_AND_RETURN_RET_LOG(nearlinkFetchResult == SUCCESS, ERROR, "nearlink fetch input device failed");
    return SUCCESS;
}

void AudioCoreService::OnAudioSceneChange(const AudioScene& audioScene)
{
    Trace trace("AudioCoreService::OnAudioSceneChange:" + std::to_string(audioScene));
    AUDIO_INFO_LOG("scene change to %{public}d", audioScene);
    CHECK_AND_RETURN_LOG(audioPolicyServerHandler_ != nullptr, "audio policy server handler is null");
    audioPolicyServerHandler_->SendAudioSceneChangeEvent(audioScene);
}

bool AudioCoreService::HandleOutputStreamInRunning(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    AudioStreamDeviceChangeReasonExt reason)
{
    if (streamDesc->streamStatus_ != STREAM_STATUS_STARTED) {
        return true;
    }
    if (HandleDeviceChangeForFetchOutputDevice(streamDesc) == ERR_NEED_NOT_SWITCH_DEVICE &&
        !Util::IsRingerOrAlarmerStreamUsage(streamDesc->rendererInfo_.streamUsage)) {
        return false;
    }
    return true;
}

bool AudioCoreService::HandleInputStreamInRunning(std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    if (streamDesc->streamStatus_ != STREAM_STATUS_STARTED) {
        return true;
    }
    if (HandleDeviceChangeForFetchInputDevice(streamDesc) == ERR_NEED_NOT_SWITCH_DEVICE) {
        return false;
    }
    return true;
}

void AudioCoreService::HandleDualStartClient(std::vector<std::pair<DeviceType, DeviceFlag>> &activeDevices,
    std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    CHECK_AND_RETURN_LOG(streamDesc != nullptr && streamDesc->newDeviceDescs_.size() > 1, "Invalid params");
    std::string firstSinkName =
        AudioPolicyUtils::GetInstance().GetSinkName(streamDesc->newDeviceDescs_[0], streamDesc->sessionId_);
    std::string secondSinkName =
        AudioPolicyUtils::GetInstance().GetSinkName(streamDesc->newDeviceDescs_[1], streamDesc->sessionId_);
    AUDIO_INFO_LOG("firstSinkName %{public}s, secondSinkName %{public}s",
        firstSinkName.c_str(), secondSinkName.c_str());
    if (firstSinkName == secondSinkName) {
        activeDevices.push_back(
            make_pair(streamDesc->newDeviceDescs_[0]->deviceType_, DeviceFlag::OUTPUT_DEVICES_FLAG));
        activeDevices.push_back(
            make_pair(streamDesc->newDeviceDescs_[1]->deviceType_, DeviceFlag::OUTPUT_DEVICES_FLAG));
    }
}

void AudioCoreService::HandlePlaybackStreamInA2dp(std::shared_ptr<AudioStreamDescriptor> &streamDesc,
    bool isCreateProcess)
{
#ifdef BLUETOOTH_ENABLE
    CHECK_AND_RETURN_LOG(streamDesc != nullptr && streamDesc->newDeviceDescs_.size() > 0 &&
        streamDesc->newDeviceDescs_[0] != nullptr, "Invalid stream desc");
    vector<Bluetooth::A2dpStreamInfo> allSessionInfos;
    Bluetooth::A2dpStreamInfo a2dpStreamInfo;
    vector<shared_ptr<AudioRendererChangeInfo>> audioRendererChangeInfos;
    streamCollector_.GetCurrentRendererChangeInfos(audioRendererChangeInfos);
    AUDIO_INFO_LOG("Current renderer number: %{public}zu, isCreateProcess: %{public}d",
        audioRendererChangeInfos.size(), isCreateProcess);
    for (auto &changeInfo : audioRendererChangeInfos) {
        a2dpStreamInfo.sessionId = changeInfo->sessionId;
        a2dpStreamInfo.streamType = streamCollector_.GetStreamType(changeInfo->sessionId);
        StreamUsage tempStreamUsage = changeInfo->rendererInfo.streamUsage;
        AudioSpatializationState spatialState =
            AudioSpatializationService::GetAudioSpatializationService().GetSpatializationState(tempStreamUsage);
        a2dpStreamInfo.isSpatialAudio = spatialState.spatializationEnabled;
        allSessionInfos.push_back(a2dpStreamInfo);
    }
    if (isCreateProcess) {
        a2dpStreamInfo.sessionId = static_cast<int32_t>(streamDesc->sessionId_);
        StreamUsage tempStreamUsage = streamDesc->rendererInfo_.streamUsage;
        a2dpStreamInfo.streamType =
            streamCollector_.GetStreamType(streamDesc->rendererInfo_.contentType, tempStreamUsage);
        AudioSpatializationState spatialState =
            AudioSpatializationService::GetAudioSpatializationService().GetSpatializationState(tempStreamUsage);
        a2dpStreamInfo.isSpatialAudio = spatialState.spatializationEnabled;
        allSessionInfos.push_back(a2dpStreamInfo);
    }
    auto receiveOffloadFlag =
        static_cast<BluetoothOffloadState>(Bluetooth::AudioA2dpManager::A2dpOffloadSessionRequest(allSessionInfos));
    AUDIO_INFO_LOG("A2dp offload flag: %{public}d", receiveOffloadFlag);
    if (receiveOffloadFlag != A2DP_OFFLOAD) {
        streamDesc->newDeviceDescs_[0]->a2dpOffloadFlag_ = receiveOffloadFlag;
        return;
    }
    streamDesc->newDeviceDescs_[0]->a2dpOffloadFlag_ = A2DP_OFFLOAD;
#endif
}

bool AudioCoreService::GetDisableFastStreamParam()
{
    int32_t disableFastStream = 1; // default 1, set disableFastStream true
    GetSysPara("persist.multimedia.audioflag.fastcontrolled", disableFastStream);
    return disableFastStream == 0 ? false : true;
}

bool AudioCoreService::IsFastAllowed(std::string &bundleName)
{
    CHECK_AND_RETURN_RET(bundleName != "", true);
    std::string bundleNamePre = CHECK_FAST_BLOCK_PREFIX + bundleName;
    std::string result = AudioServerProxy::GetInstance().GetAudioParameterProxy(bundleNamePre);
    if (result == "true") { // "true" means in control
        AUDIO_INFO_LOG("%{public}s not in fast list", bundleName.c_str());
        return false;
    }
    return true;
}

int32_t AudioCoreService::ActivateNearlinkDevice(const std::shared_ptr<AudioStreamDescriptor> &streamDesc)
{
    CHECK_AND_RETURN_RET_LOG(streamDesc != nullptr, ERR_INVALID_PARAM, "Stream desc is nullptr");
    auto deviceDesc = streamDesc->newDeviceDescs_.front();
    CHECK_AND_RETURN_RET_LOG(deviceDesc != nullptr, ERR_INVALID_PARAM, "Device desc is nullptr");

    std::variant<StreamUsage, SourceType> audioStreamConfig;
    bool isRunning = streamDesc->streamStatus_ == STREAM_STATUS_STARTED;
    if (streamDesc->audioMode_ == AUDIO_MODE_PLAYBACK) {
        audioStreamConfig = streamDesc->rendererInfo_.streamUsage;
    } else {
        audioStreamConfig = streamDesc->capturerInfo_.sourceType;
    }
    if (deviceDesc->deviceType_ == DEVICE_TYPE_NEARLINK || deviceDesc->deviceType_ == DEVICE_TYPE_NEARLINK_IN) {
        auto runDeviceActivationFlow = [this, &deviceDesc, &isRunning](auto &&config) -> int32_t {
            int32_t ret = sleAudioDeviceManager_.SetActiveDevice(deviceDesc->macAddress_, config);
            CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ret, "Activating Nearlink device fails");
            CHECK_AND_RETURN_RET_LOG(isRunning, ret, "Stream is not runningf, no needs start playing");
            return sleAudioDeviceManager_.StartPlaying(*deviceDesc, config);
        };

        int32_t result = std::visit(runDeviceActivationFlow, audioStreamConfig);
        CHECK_AND_RETURN_RET_LOG(result == SUCCESS, ERROR,
            "Nearlink device activation failed, macAddress: %{public}s",
                GetEncryptAddr(deviceDesc->macAddress_).c_str());
    }
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
