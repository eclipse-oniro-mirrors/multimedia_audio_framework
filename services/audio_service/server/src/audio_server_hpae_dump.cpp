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
#define LOG_TAG "AudioServerHpaeDump"
#endif

#include "audio_server_hpae_dump.h"
#include "audio_utils.h"
#include "audio_errors.h"
#include "audio_service.h"
#include "audio_dump_pcm.h"
#include "audio_performance_monitor.h"
#include "manager/hdi_adapter_manager.h"
#include "i_hpae_manager.h"

using namespace std;
using namespace OHOS::AudioStandard::HPAE;
namespace OHOS {
namespace AudioStandard {
static const int32_t OPERATION_TIMEOUT_IN_MS = 1000;  // 1000ms
static const std::string BT_SINK_NAME = "Bt_Speaker";
static const std::string MCH_SINK_NAME = "MCH_Speaker";
static const std::string OFFLOAD_SINK_NAME = "Offload_Speaker";
static const std::string DP_SINK_NAME = "DP_speaker";
static const std::string DEFAULT_SINK_NAME = "Speaker";
static const std::string PRIMARY_SOURCE_NAME = "Built_in_mic";
static const std::string BT_SOURCE_NAME = "Bt_Mic";
static const std::string USB_SOURCE_NAME = "Usb_arm_mic";
static const std::string PRIMARY_WAKEUP_SOURCE_NAME = "Built_in_wakeup";

AudioServerHpaeDump::AudioServerHpaeDump()
{
    AUDIO_DEBUG_LOG("AudioServerHpaeDump construct");
    InitDumpFuncMap();
}

AudioServerHpaeDump::~AudioServerHpaeDump()
{}

void AudioServerHpaeDump::InitDumpFuncMap()
{
    dumpFuncMap[u"-h"] = &AudioServerHpaeDump::HelpInfoDump;
    dumpFuncMap[u"-p"] = &AudioServerHpaeDump::PlaybackSinkDump;
    dumpFuncMap[u"-r"] = &AudioServerHpaeDump::RecordSourceDump;
    dumpFuncMap[u"-m"] = &AudioServerHpaeDump::HDFModulesDump;
    dumpFuncMap[u"-ep"] = &AudioServerHpaeDump::PolicyHandlerDump;
    dumpFuncMap[u"-ct"] = &AudioServerHpaeDump::AudioCacheTimeDump;
    dumpFuncMap[u"-cm"] = &AudioServerHpaeDump::AudioCacheMemoryDump;
    dumpFuncMap[u"-pm"] = &AudioServerHpaeDump::AudioPerformMonitorDump;
    dumpFuncMap[u"-ha"] = &AudioServerHpaeDump::HdiAdapterDump;
}

void AudioServerHpaeDump::AudioDataDump(std::string &dumpString, std::queue<std::u16string> &argQue)
{
    ArgDataDump(dumpString, argQue);
}

void AudioServerHpaeDump::ServerDataDump(string &dumpString)
{
    PlaybackSinkDump(dumpString);
    RecordSourceDump(dumpString);
    HDFModulesDump(dumpString);
    PolicyHandlerDump(dumpString);
}

void AudioServerHpaeDump::GetDeviceSinkInfo(std::string &dumpString, std::string deviceName)
{
    lock_guard<mutex> lock(lock_);
    AUDIO_INFO_LOG("GetDeviceSinkInfo %{public}s start", deviceName.c_str());
    isFinishGetSinkInfo_ = false;
    IHpaeManager::GetHpaeManager().DumpSinkInfo(deviceName);
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    dumpHpaeSinkInfo_.clear();
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishGetSinkInfo_;  // will be true when got notified.
    });
    if (!stopWaiting) {
        AUDIO_ERR_LOG("GetDeviceSinkInfo timeout");
        return;
    }
    AUDIO_INFO_LOG("GetDeviceSinkInfo %{public}s end", deviceName.c_str());
    dumpString += dumpHpaeSinkInfo_;
}

void AudioServerHpaeDump::PlaybackSinkDump(std::string &dumpString)
{
    dumpString += "Hpae AudioServer Playback sink Dump:\n\n";
    dumpString += DEFAULT_SINK_NAME + ":\n";
    GetDeviceSinkInfo(dumpString, DEFAULT_SINK_NAME);
    dumpString += OFFLOAD_SINK_NAME + ":\n";
    GetDeviceSinkInfo(dumpString, OFFLOAD_SINK_NAME);
    dumpString += MCH_SINK_NAME + ":\n";
    GetDeviceSinkInfo(dumpString, MCH_SINK_NAME);
    dumpString += BT_SINK_NAME + ":\n";
    GetDeviceSinkInfo(dumpString, BT_SINK_NAME);
    dumpString += DP_SINK_NAME + ":\n";
    GetDeviceSinkInfo(dumpString, DP_SINK_NAME);
    dumpString += "\n";
}

void AudioServerHpaeDump::OnDumpSinkInfoCb(std::string &dumpStr, int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    dumpHpaeSinkInfo_ = dumpStr;
    isFinishGetSinkInfo_ = true;
    AUDIO_INFO_LOG(
        "AudioServerHpaeDump OnDumpSinkInfoCb %{public}s, result %{public}d", dumpHpaeSinkInfo_.c_str(), result);
    callbackCV_.notify_all();
}

void AudioServerHpaeDump::GetDeviceSourceInfo(std::string &dumpString, std::string deviceName)
{
    lock_guard<mutex> lock(lock_);
    AUDIO_INFO_LOG("GetDeviceSourceInfo %{public}s start", deviceName.c_str());
    isFinishGetSourceInfo_ = false;
    IHpaeManager::GetHpaeManager().DumpSourceInfo(deviceName);
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    dumpHpaeSourceInfo_.clear();
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishGetSourceInfo_;  // will be true when got notified.
    });
    if (!stopWaiting) {
        AUDIO_ERR_LOG("GetDeviceSourceInfo timeout");
        return;
    }
    AUDIO_INFO_LOG("GetDeviceSourceInfo %{public}s end", deviceName.c_str());
    dumpString += dumpHpaeSourceInfo_;
}

void AudioServerHpaeDump::RecordSourceDump(std::string &dumpString)
{
    dumpString += "Hpae AudioServer Record source Dump:\n\n";
    dumpString += PRIMARY_SOURCE_NAME + ":\n";
    GetDeviceSourceInfo(dumpString, PRIMARY_SOURCE_NAME);
    dumpString += BT_SOURCE_NAME + ":\n";
    GetDeviceSourceInfo(dumpString, BT_SOURCE_NAME);
    dumpString += USB_SOURCE_NAME + ":\n";
    GetDeviceSourceInfo(dumpString, USB_SOURCE_NAME);
    dumpString += PRIMARY_WAKEUP_SOURCE_NAME + ":\n";
    GetDeviceSourceInfo(dumpString, PRIMARY_WAKEUP_SOURCE_NAME);
    dumpString += "\n";
}

void AudioServerHpaeDump::OnDumpSourceInfoCb(std::string &dumpStr, int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    dumpHpaeSourceInfo_ = dumpStr;
    isFinishGetSourceInfo_ = true;
    AUDIO_INFO_LOG(
        "AudioServerHpaeDump OnDumpSourceInfoCb %{public}s, result %{public}d", dumpHpaeSourceInfo_.c_str(), result);
    callbackCV_.notify_all();
}

void AudioServerHpaeDump::ArgDataDump(std::string &dumpString, std::queue<std::u16string> &argQue)
{
    dumpString += "Hpae AudioServer Data Dump:\n\n";
    if (argQue.empty()) {
        ServerDataDump(dumpString);
        return;
    }
    while (!argQue.empty()) {
        std::u16string para = argQue.front();
        if (para == u"-h") {
            dumpString.clear();
            (this->*dumpFuncMap[para])(dumpString);
            return;
        } else if (para == u"-p") {
            dumpString.clear();
            (this->*dumpFuncMap[para])(dumpString);
            return;
        } else if (dumpFuncMap.count(para) == 0) {
            dumpString.clear();
            AppendFormat(dumpString, "Please input correct param:\n");
            HelpInfoDump(dumpString);
            return;
        } else {
            (this->*dumpFuncMap[para])(dumpString);
        }
        argQue.pop();
    }
}

void AudioServerHpaeDump::HelpInfoDump(string &dumpString)
{
    AppendFormat(dumpString, "usage:\n");
    AppendFormat(dumpString, "  -h\t\t\t|help text for hidumper audio\n");
    AppendFormat(dumpString, "  -p\t\t\t|dump hpae playback streams\n");
    AppendFormat(dumpString, "  -r\t\t\t|dump hpae record streams\n");
}

int32_t AudioServerHpaeDump::Initialize()
{
    AUDIO_INFO_LOG("AudioServerHpaeDump Initialize");
    IHpaeManager::GetHpaeManager().RegisterHpaeDumpCallback(weak_from_this());
    return SUCCESS;
}

void AudioServerHpaeDump::HDFModulesDump(std::string &dumpString)
{
    IHpaeManager::GetHpaeManager().DumpAllAvailableDevice(devicesInfo_);
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    isFinishGetHdfModulesInfo_ = false;
    dumpHdfModulesInfo_.clear();
    bool stopWaiting = callbackCV_.wait_for(waitLock, std::chrono::milliseconds(OPERATION_TIMEOUT_IN_MS), [this] {
        return isFinishGetHdfModulesInfo_;  // will be true when got notified.
    });
    if (!stopWaiting) {
        AUDIO_ERR_LOG("DumpAllAvailableDevice timeout");
        return;
    }
    
    dumpHdfModulesInfo_ += "\nHDF Input Modules\n";
    AppendFormat(dumpHdfModulesInfo_, "- %zu HDF Input Modules (s) available:\n", devicesInfo_.sourceInfos.size());

    for (auto it = devicesInfo_.sourceInfos.begin(); it != devicesInfo_.sourceInfos.end(); it++) {
        HpaeSinkSourceInfo &sourceInfo = *it;
        AppendFormat(dumpHdfModulesInfo_, "  Module %d\n", it - devicesInfo_.sourceInfos.begin() + 1);
        AppendFormat(dumpHdfModulesInfo_, "  - Module Name: %s\n", (sourceInfo.deviceName).c_str());
        AppendFormat(dumpHdfModulesInfo_, "  - Module Configuration: %s\n\n", sourceInfo.config.c_str());
    }

    dumpHdfModulesInfo_ += "HDF Output Modules\n";
    AppendFormat(dumpHdfModulesInfo_, "- %zu HDF Output Modules (s) available:\n", devicesInfo_.sinkInfos.size());

    for (auto it = devicesInfo_.sinkInfos.begin(); it != devicesInfo_.sinkInfos.end(); it++) {
        HpaeSinkSourceInfo &sinkInfo = *it;
        AppendFormat(dumpHdfModulesInfo_, "  Module %d\n", it - devicesInfo_.sinkInfos.begin() + 1);
        AppendFormat(dumpHdfModulesInfo_, "  - Module Name: %s\n", (sinkInfo.deviceName).c_str());
        AppendFormat(dumpHdfModulesInfo_, "  - Module Configuration: %s\n\n", sinkInfo.config.c_str());
    }

    AUDIO_INFO_LOG("HDFModulesDump : \n%{public}s end", dumpHdfModulesInfo_.c_str());
    dumpString += dumpHdfModulesInfo_;
}

void AudioServerHpaeDump::OnDumpAllAvailableDeviceCb(int32_t result)
{
    std::unique_lock<std::mutex> waitLock(callbackMutex_);
    isFinishGetHdfModulesInfo_ = true;
    AUDIO_INFO_LOG(
        "sink count %{public}zu, source count %{public}zu, result %{public}d",
        devicesInfo_.sinkInfos.size(), devicesInfo_.sourceInfos.size(), result);
    callbackCV_.notify_all();
}

void AudioServerHpaeDump::PolicyHandlerDump(std::string &dumpString)
{
    AUDIO_INFO_LOG("PolicyHandlerDump");
    AudioService::GetInstance()->Dump(dumpString);
}

void AudioServerHpaeDump::AudioCacheTimeDump(std::string &dumpString)
{
    AUDIO_INFO_LOG("AudioCacheTimeDump");
    dumpString += "\nAudioCached Time\n";

    int64_t startTime = 0;
    int64_t endTime = 0;
    AudioCacheMgr::GetInstance().GetCachedDuration(startTime, endTime);
    dumpString += "Call dump get time: [ " + ClockTime::NanoTimeToString(startTime) + " ~ " +
        ClockTime::NanoTimeToString(endTime) + " ], cur: [ " +
        ClockTime::NanoTimeToString(ClockTime::GetRealNano()) + " ] \n";
}

void AudioServerHpaeDump::AudioCacheMemoryDump(std::string &dumpString)
{
    AUDIO_INFO_LOG("AudioCacheMemoryDump");
    dumpString += "\nAudioCached Memory\n";

    size_t dataLength = 0;
    size_t bufferLength = 0;
    size_t structLength = 0;
    AudioCacheMgr::GetInstance().GetCurMemoryCondition(dataLength, bufferLength, structLength);
    dumpString += "dataLength: " + std::to_string(dataLength / BYTE_TO_KB_SIZE) + " KB, " +
                    "bufferLength: " + std::to_string(bufferLength / BYTE_TO_KB_SIZE) + " KB, " +
                    "structLength: " + std::to_string(structLength / BYTE_TO_KB_SIZE) + " KB \n";
}

void AudioServerHpaeDump::AudioPerformMonitorDump(std::string &dumpString)
{
    AUDIO_INFO_LOG("AudioPerformMonitorDump");
    dumpString += "\n Dump Audio Performance Monitor Record Infos\n";
    AudioPerformanceMonitor::GetInstance().DumpMonitorInfo(dumpString);
}

void AudioServerHpaeDump::HdiAdapterDump(std::string &dumpString)
{
    AUDIO_INFO_LOG("HdiAdapterDump");
    dumpString += "\nHdiAdapter Info\n";
    HdiAdapterManager::GetInstance().DumpInfo(dumpString);
}
}  // namespace AudioStandard
}  // namespace OHOS
