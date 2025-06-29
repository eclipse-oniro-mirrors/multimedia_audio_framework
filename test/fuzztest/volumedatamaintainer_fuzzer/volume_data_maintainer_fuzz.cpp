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

#include "volume_data_maintainer.h"
using namespace std;

namespace OHOS {
namespace AudioStandard {

const int32_t NUM_2 = 2;
const int32_t NUM_3 = 3;
typedef void (*TestPtr)(const uint8_t *, size_t);

const vector<DeviceType> g_testDeviceTypes = {
    DEVICE_TYPE_NONE,
    DEVICE_TYPE_INVALID,
    DEVICE_TYPE_EARPIECE,
    DEVICE_TYPE_SPEAKER,
    DEVICE_TYPE_WIRED_HEADSET,
    DEVICE_TYPE_WIRED_HEADPHONES,
    DEVICE_TYPE_BLUETOOTH_SCO,
    DEVICE_TYPE_BLUETOOTH_A2DP,
    DEVICE_TYPE_BLUETOOTH_A2DP_IN,
    DEVICE_TYPE_MIC,
    DEVICE_TYPE_WAKEUP,
    DEVICE_TYPE_USB_HEADSET,
    DEVICE_TYPE_DP,
    DEVICE_TYPE_REMOTE_CAST,
    DEVICE_TYPE_USB_DEVICE,
    DEVICE_TYPE_ACCESSORY,
    DEVICE_TYPE_REMOTE_DAUDIO,
    DEVICE_TYPE_HDMI,
    DEVICE_TYPE_LINE_DIGITAL,
    DEVICE_TYPE_NEARLINK,
    DEVICE_TYPE_NEARLINK_IN,
    DEVICE_TYPE_FILE_SINK,
    DEVICE_TYPE_FILE_SOURCE,
    DEVICE_TYPE_EXTERN_CABLE,
    DEVICE_TYPE_DEFAULT,
    DEVICE_TYPE_USB_ARM_HEADSET,
    DEVICE_TYPE_MAX
};

const vector<AudioStreamType> g_testStreamTypes = {
    STREAM_DEFAULT,
    STREAM_VOICE_CALL,
    STREAM_MUSIC,
    STREAM_RING,
    STREAM_MEDIA,
    STREAM_VOICE_ASSISTANT,
    STREAM_SYSTEM,
    STREAM_ALARM,
    STREAM_NOTIFICATION,
    STREAM_BLUETOOTH_SCO,
    STREAM_ENFORCED_AUDIBLE,
    STREAM_DTMF,
    STREAM_TTS,
    STREAM_ACCESSIBILITY,
    STREAM_RECORDING,
    STREAM_MOVIE,
    STREAM_GAME,
    STREAM_SPEECH,
    STREAM_SYSTEM_ENFORCED,
    STREAM_ULTRASONIC,
    STREAM_WAKEUP,
    STREAM_VOICE_MESSAGE,
    STREAM_NAVIGATION,
    STREAM_INTERNAL_FORCE_STOP,
    STREAM_SOURCE_VOICE_CALL,
    STREAM_VOICE_COMMUNICATION,
    STREAM_VOICE_RING,
    STREAM_VOICE_CALL_ASSISTANT,
    STREAM_CAMCORDER,
    STREAM_APP,
    STREAM_TYPE_MAX,
    STREAM_ALL,
};

template<class T>
uint32_t GetArrLength(T& arr)
{
    if (arr == nullptr) {
        AUDIO_INFO_LOG("%{public}s: The array length is equal to 0", __func__);
        return 0;
    }
    return sizeof(arr) / sizeof(arr[0]);
}

void VolumeDataMaintainerSaveVolumeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType typeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamTypeRet = g_testStreamTypes[index];
    int32_t volumeLevelRet = static_cast<int32_t>(size);
    volumeDataMaintainerRet.SaveVolume(typeRet, streamTypeRet, volumeLevelRet);
}

void VolumeDataMaintainerGetVolumeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamTypeRet = g_testStreamTypes[index];
    volumeDataMaintainerRet.GetVolume(deviceTypeRet, streamTypeRet);
}

void VolumeDataMaintainerSaveMuteStatusFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamTypeRet = g_testStreamTypes[index];
    bool muteStatusRet = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    volumeDataMaintainerRet.SaveMuteStatus(deviceTypeRet, streamTypeRet, muteStatusRet);
}

void VolumeDataMaintainerGetMuteStatusInternalFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamTypeRet = g_testStreamTypes[index];
    volumeDataMaintainerRet.GetMuteStatusInternal(deviceTypeRet, streamTypeRet);
}

void VolumeDataMaintainerGetMuteTransferStatusFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    int32_t affectedRet;
    bool statusRet;
    volumeDataMaintainerRet.GetMuteAffected(affectedRet);
    volumeDataMaintainerRet.GetMuteTransferStatus(statusRet);
}

void VolumeDataMaintainerGetSafeStatusFuzzTest(const uint8_t *rawData, size_t size)
{
    vector<SafeStatus> testSafeStatus = {
        SAFE_UNKNOWN,
        SAFE_INACTIVE,
        SAFE_ACTIVE,
    };
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % testSafeStatus.size();
    SafeStatus safeStatusRet = testSafeStatus[index];
    volumeDataMaintainerRet.SaveSafeStatus(deviceTypeRet, safeStatusRet);
    volumeDataMaintainerRet.GetSafeStatus(deviceTypeRet, safeStatusRet);
}

void VolumeDataMaintainerGetSafeVolumeTimeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    int64_t timeRet = static_cast<int64_t>(size);
    volumeDataMaintainerRet.SaveSafeVolumeTime(deviceTypeRet, timeRet);
    volumeDataMaintainerRet.GetSafeVolumeTime(deviceTypeRet, timeRet);
}

void VolumeDataMaintainerRegisterClonedFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    std::string keyRet;
    std::string valueRet;
    volumeDataMaintainerRet.SaveSystemSoundUrl(keyRet, valueRet);
    volumeDataMaintainerRet.GetSystemSoundUrl(keyRet, valueRet);
    volumeDataMaintainerRet.RegisterCloned();
}

void VolumeDataMaintainerGetMicMuteStateFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    bool isMuteRet = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    volumeDataMaintainerRet.SaveMicMuteState(isMuteRet);
    volumeDataMaintainerRet.GetMicMuteState(isMuteRet);
}

void VolumeDataMaintainerGetDeviceTypeNameFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    volumeDataMaintainerRet.GetDeviceTypeName(deviceTypeRet);
}

void VolumeDataMaintainerGetVolumeKeyForDataShareFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceTypeRet = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamTypeRet = g_testStreamTypes[index];
    volumeDataMaintainerRet.GetVolumeKeyForDataShare(deviceTypeRet, streamTypeRet);
}

void VolumeDataMaintainerSaveMuteStatusInternalFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceType = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamType = g_testStreamTypes[index];
    bool muteStatus = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    bool result = volumeDataMaintainer.SaveMuteStatusInternal(deviceType, streamType, muteStatus);
}

void VolumeDataMaintainerGetAppMuteFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    int32_t appUid = static_cast<int32_t>(size);
    bool isMute = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    volumeDataMaintainer.appMuteStatusMap_.erase(appUid);

    volumeDataMaintainer.GetAppMute(appUid, isMute);
}

void VolumeDataMaintainerGetAppMuteOwnedFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    int32_t appUid = static_cast<int32_t>(size);
    bool isMute = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    int32_t callingUid = IPCSkeleton::GetCallingUid();
    volumeDataMaintainer.appMuteStatusMap_[appUid][callingUid] = !isMute;

    volumeDataMaintainer.GetAppMuteOwned(appUid, isMute);
}

void VolumeDataMaintainerGetDeviceVolumeInternalFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceType = g_testDeviceTypes[index];
    index = static_cast<uint32_t>(size) % g_testStreamTypes.size();
    AudioStreamType streamType = g_testStreamTypes[index];
    volumeDataMaintainer.GetDeviceVolumeInternal(deviceType, streamType);
}

void VolumeDataMaintainerSetMuteAffectedToMuteStatusDataBaseFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    std::lock_guard<ffrt::mutex> lock(volumeDataMaintainer.volumeMutex_);
    volumeDataMaintainer.volumeLevelMap_.clear();
    volumeDataMaintainer.remoteVolumeLevelMap_.clear();
    volumeDataMaintainer.appVolumeLevelMap_.clear();
    volumeDataMaintainer.appMuteStatusMap_.clear();

    int32_t affected = static_cast<int32_t>(size);
    volumeDataMaintainer.SetMuteAffectedToMuteStatusDataBase(affected);
}

void VolumeDataMaintainerSetRestoreVolumeLevelFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    std::lock_guard<ffrt::mutex> lock(volumeDataMaintainer.volumeMutex_);
    volumeDataMaintainer.volumeLevelMap_.clear();
    volumeDataMaintainer.remoteVolumeLevelMap_.clear();
    volumeDataMaintainer.appVolumeLevelMap_.clear();
    volumeDataMaintainer.appMuteStatusMap_.clear();

    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceType = g_testDeviceTypes[index];
    int32_t volume = static_cast<int32_t>(size);
    volumeDataMaintainer.SetRestoreVolumeLevel(deviceType, volume);
}

void VolumeDataMaintainerGetRestoreVolumeLevelFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainer = VolumeDataMaintainer::GetVolumeDataMaintainer();
    std::lock_guard<ffrt::mutex> lock(volumeDataMaintainer.volumeMutex_);
    volumeDataMaintainer.volumeLevelMap_.clear();
    volumeDataMaintainer.remoteVolumeLevelMap_.clear();
    volumeDataMaintainer.appVolumeLevelMap_.clear();
    volumeDataMaintainer.appMuteStatusMap_.clear();

    uint32_t index = static_cast<uint32_t>(size) % g_testDeviceTypes.size();
    DeviceType deviceType = g_testDeviceTypes[index];
    int32_t volume = static_cast<int32_t>(size);
    volumeDataMaintainer.GetRestoreVolumeLevel(deviceType, volume);
}

void VolumeDataMaintainerGetRingerModeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    int32_t affectedRet = static_cast<int32_t>(size);
    bool statusRet = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    uint32_t index = static_cast<uint32_t>(size) % NUM_3;
    AudioRingerMode ringerModeRet = static_cast<AudioRingerMode>(index);
    volumeDataMaintainerRet.SetMuteAffectedToMuteStatusDataBase(affectedRet);
    volumeDataMaintainerRet.SaveMuteTransferStatus(statusRet);
    volumeDataMaintainerRet.SaveRingerMode(ringerModeRet);
    volumeDataMaintainerRet.GetRingerMode(ringerModeRet);
}

void VolumeDataMaintainerGetDeviceVolumeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size);
    DeviceType deviceType = g_testDeviceTypes[index % g_testDeviceTypes.size()];
    AudioStreamType streamType = g_testStreamTypes[index % g_testStreamTypes.size()];
    volumeDataMaintainerRet.GetDeviceVolume(deviceType, streamType);
}

void VolumeDataMaintainerSetStreamMuteStatusFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size);
    AudioStreamType streamType = g_testStreamTypes[index % g_testStreamTypes.size()];
    bool muteStatus = static_cast<bool>(static_cast<uint32_t>(size) % NUM_2);
    volumeDataMaintainerRet.SetStreamMuteStatus(streamType, muteStatus);
}

void VolumeDataMaintainerGetMuteStatusFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size);
    DeviceType deviceType = g_testDeviceTypes[index % g_testDeviceTypes.size()];
    AudioStreamType streamType = g_testStreamTypes[index % g_testStreamTypes.size()];
    volumeDataMaintainerRet.GetMuteStatus(deviceType, streamType);
}

void VolumeDataMaintainerGetStreamMuteFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    uint32_t index = static_cast<uint32_t>(size);
    AudioStreamType streamType = g_testStreamTypes[index % g_testStreamTypes.size()];
    volumeDataMaintainerRet.GetStreamMute(streamType);
}

void VolumeDataMaintainerSetAppVolumeFuzzTest(const uint8_t *rawData, size_t size)
{
    VolumeDataMaintainer &volumeDataMaintainerRet = VolumeDataMaintainer::GetVolumeDataMaintainer();
    int32_t appUid = static_cast<int32_t>(size);
    int32_t volumeLevel = static_cast<int32_t>(size / NUM_2);
    volumeDataMaintainerRet.SetAppVolume(appUid, volumeLevel);
}

} // namespace AudioStandard
} // namesapce OHOS

OHOS::AudioStandard::TestPtr g_testPtrs[] = {
    OHOS::AudioStandard::VolumeDataMaintainerSaveVolumeFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetVolumeFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSaveMuteStatusFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetMuteStatusInternalFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetMuteTransferStatusFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetSafeStatusFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetSafeVolumeTimeFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerRegisterClonedFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetMicMuteStateFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetDeviceTypeNameFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetVolumeKeyForDataShareFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSaveMuteStatusInternalFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetAppMuteFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetAppMuteOwnedFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetDeviceVolumeInternalFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSetMuteAffectedToMuteStatusDataBaseFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSetRestoreVolumeLevelFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetRestoreVolumeLevelFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetRingerModeFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetDeviceVolumeFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSetStreamMuteStatusFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetMuteStatusFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerGetStreamMuteFuzzTest,
    OHOS::AudioStandard::VolumeDataMaintainerSetAppVolumeFuzzTest,
};

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr || size <= 1) {
        return 0;
    }
    uint32_t len = OHOS::AudioStandard::GetArrLength(g_testPtrs);
    if (len > 0) {
        uint8_t firstByte = *data % len;
        if (firstByte >= len) {
            return 0;
        }
        data = data + 1;
        size = size - 1;
        g_testPtrs[firstByte](data, size);
    }
    return 0;
}