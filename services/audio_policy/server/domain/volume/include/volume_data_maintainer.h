/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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
#ifndef VOLUME_DATA_MAINTAINER_H
#define VOLUME_DATA_MAINTAINER_H

#include <list>
#include <unordered_map>
#include <mutex>
#include <cinttypes>
#include "errors.h"
#include "ipc_skeleton.h"
#include "ffrt.h"

#include "audio_utils.h"
#include "audio_setting_provider.h"
#include "audio_policy_log.h"
#include "audio_info.h"
#include "audio_setting_provider.h"

namespace OHOS {
namespace AudioStandard {

class VolumeDataMaintainer {
public:
    enum VolumeDataMaintainerStreamType {  // define with Dual framework
        VT_STREAM_DEFAULT = -1,
        VT_STREAM_VOICE_CALL = 0,
        VT_STREAM_SYSTEM  = 1,
        VT_STREAM_RING = 2,
        VT_STREAM_MUSIC  = 3,
        VT_STREAM_ALARM = 4,
        VT_STREAM_NOTIFICATION = 5,
        VT_STREAM_BLUETOOTH_SCO = 6,
        VT_STREAM_SYSTEM_ENFORCED = 7,
        VT_STREAM_DTMF = 8,
        VT_STREAM_TTS = 9,
        VT_STREAM_ACCESSIBILITY = 10,
        VT_STREAM_ASSISTANT = 11,
    };

    static VolumeDataMaintainer& GetVolumeDataMaintainer()
    {
        static VolumeDataMaintainer volumeDataMainTainer;
        return volumeDataMainTainer;
    }
    ~VolumeDataMaintainer();

    void SetDataShareReady(std::atomic<bool> isDataShareReady);
    bool SaveVolume(DeviceType type, AudioStreamType streamType, int32_t volumeLevel,
        std::string networkId = "LocalDevice");
    bool GetVolume(DeviceType deviceType, AudioStreamType streamType, std::string networkId = "LocalDevice");
    void SetStreamVolume(AudioStreamType streamType, int32_t volumeLevel);
    void SetAppVolume(int32_t appUid, int32_t volumeLevel);
    void GetAppMute(int32_t appUid, bool &isMute);
    void GetAppMuteOwned(int32_t appUid, bool &isMute);
    void SetAppVolumeMuted(int32_t appUid, bool muted);
    int32_t GetStreamVolume(AudioStreamType streamType);
    int32_t GetDeviceVolume(DeviceType deviceType, AudioStreamType streamType);
    int32_t GetAppVolume(int32_t appUid);
    bool IsSetAppVolume(int32_t appUid);
    std::unordered_map<AudioStreamType, int32_t> GetVolumeMap();

    bool SaveMuteStatus(DeviceType deviceType, AudioStreamType streamType,
        bool muteStatus);
    bool GetMuteStatus(DeviceType deviceType, AudioStreamType streamType);
    bool SetStreamMuteStatus(AudioStreamType streamType, bool muteStatus);
    bool GetStreamMute(AudioStreamType streamType);

    bool GetMuteAffected(int32_t &affected);
    bool GetMuteTransferStatus(bool &status);
    bool SetMuteAffectedToMuteStatusDataBase(int32_t affected);
    bool SaveMuteTransferStatus(bool status);

    bool SaveRingerMode(AudioRingerMode ringerMode);
    bool GetRingerMode(AudioRingerMode &ringerMode);
    bool SaveSafeStatus(DeviceType deviceType, SafeStatus safeStatus);
    bool GetSafeStatus(DeviceType deviceType, SafeStatus &safeStatus);
    bool SaveSafeVolumeTime(DeviceType deviceType, int64_t time);
    bool GetSafeVolumeTime(DeviceType deviceType, int64_t &time);
    bool SaveSystemSoundUrl(const std::string &key, const std::string &value);
    bool GetSystemSoundUrl(const std::string &key, std::string &value);
    bool SetRestoreVolumeLevel(DeviceType deviceType, int32_t volume);
    bool GetRestoreVolumeLevel(DeviceType deviceType, int32_t &volume);
    void RegisterCloned();
    bool SaveMicMuteState(bool isMute);
    bool GetMicMuteState(bool &isMute);
    bool CheckOsAccountReady();

    void StoreRemoteVolumeLevelMap(void);
    void LoadRemoteVolumeLevelMap(void);

private:
    VolumeDataMaintainer();
    static std::string GetVolumeKeyForDataShare(DeviceType deviceType, AudioStreamType streamType,
        std::string networkId = "LocalDevice");
    static std::string GetMuteKeyForDataShare(DeviceType deviceType, AudioStreamType streamType);
    static std::string GetDeviceTypeName(DeviceType deviceType);
    bool SaveVolumeInternal(DeviceType type, AudioStreamType streamType, int32_t volumeLevel, std::string networkId);
    int32_t GetDeviceVolumeInternal(DeviceType deviceType, AudioStreamType streamType);
    bool GetVolumeInternal(DeviceType deviceType, AudioStreamType streamType, std::string networkId);
    void SetStreamVolumeInternal(AudioStreamType streamType, int32_t volumeLevel);
    bool SaveMuteStatusInternal(DeviceType deviceType, AudioStreamType streamType, bool muteStatus);
    bool GetMuteStatusInternal(DeviceType deviceType, AudioStreamType streamType);
    bool GetStreamMuteInternal(AudioStreamType streamType);
    int32_t GetStreamVolumeInternal(AudioStreamType streamType);

    ffrt::mutex volumeMutex_;
    ffrt::mutex volumeForDbMutex_;
    std::unordered_map<AudioStreamType, bool> muteStatusMap_; // save System volume Mutestatus map
    std::unordered_map<AudioStreamType, int32_t> volumeLevelMap_; // save system volume map
    std::unordered_map<AudioStreamType, int32_t> remoteVolumeLevelMap_; // save system remote volume map
    std::unordered_map<int32_t, int32_t> appVolumeLevelMap_; // save App volume map
    std::unordered_map<int32_t, std::unordered_map<int32_t, bool>> appMuteStatusMap_; // save App volume Mutestatus map
    bool isSettingsCloneHaveStarted_ = false;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // VOLUME_DATA_MAINTAINER_H
