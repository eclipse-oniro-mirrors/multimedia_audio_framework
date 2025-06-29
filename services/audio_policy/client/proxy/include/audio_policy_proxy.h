/*
 * Copyright (c) 2021-2025 Huawei Device Co., Ltd.
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

#ifndef ST_AUDIO_POLICY_PROXY_H
#define ST_AUDIO_POLICY_PROXY_H

#include <memory>
#include "iremote_proxy.h"
#include "audio_policy_base.h"
#include "audio_errors.h"
#include "microphone_descriptor.h"

namespace OHOS {
namespace AudioStandard {
class AudioPolicyProxy : public IRemoteProxy<IAudioPolicy> {
public:
    explicit AudioPolicyProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioPolicyProxy() = default;

    int32_t GetMaxVolumeLevel(AudioVolumeType volumeType) override;

    int32_t GetMinVolumeLevel(AudioVolumeType volumeType) override;

    int32_t SetSystemVolumeLevelLegacy(AudioVolumeType volumeType, int32_t volumeLevel) override;

    int32_t SetSystemVolumeLevel(AudioVolumeType volumeType, int32_t volumeLevel, int32_t volumeFlag = 0) override;

    int32_t SetSystemVolumeLevelWithDevice(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType,
        int32_t volumeFlag = 0) override;

    int32_t SetAppVolumeLevel(int32_t appUid, int32_t volumeLevel, int32_t volumeFlag = 0) override;

    int32_t SetAppVolumeMuted(int32_t appUid, bool muted, int32_t volumeFlag = 0) override;

    int32_t IsAppVolumeMute(int32_t appUid, bool owned, bool &isMute) override;

    int32_t SetSelfAppVolumeLevel(int32_t volumeLevel, int32_t volumeFlag = 0) override;

    int32_t GetAppVolumeLevel(int32_t appUid, int32_t &volumeLevel) override;

    int32_t GetSelfAppVolumeLevel(int32_t &volumeLevel) override;

    AudioStreamType GetSystemActiveVolumeType(const int32_t clientUid) override;

    int32_t GetSystemVolumeLevel(AudioVolumeType volumeType) override;

    int32_t SetLowPowerVolume(int32_t streamId, float volume) override;

    float GetLowPowerVolume(int32_t streamId) override;

    float GetSingleStreamVolume(int32_t streamId) override;

    int32_t SetStreamMuteLegacy(AudioVolumeType volumeType, bool mute,
        const DeviceType &deviceType = DEVICE_TYPE_NONE) override;

    int32_t SetStreamMute(AudioVolumeType volumeType, bool mute,
        const DeviceType &deviceType = DEVICE_TYPE_NONE) override;

    bool GetStreamMute(AudioVolumeType volumeType) override;

    bool IsStreamActive(AudioVolumeType volumeType) override;

    bool IsStreamActiveByStreamUsage(StreamUsage streamUsage) override;

    bool IsFastPlaybackSupported(AudioStreamInfo &streamInfo, StreamUsage usage) override;
    bool IsFastRecordingSupported(AudioStreamInfo &streamInfo, SourceType source) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetDevices(DeviceFlag deviceFlag) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetDevicesInner(DeviceFlag deviceFlag) override;

    int32_t SetDeviceActive(InternalDeviceType deviceType, bool active, const int32_t uid = INVALID_UID) override;

    bool IsDeviceActive(InternalDeviceType deviceType) override;

    DeviceType GetActiveOutputDevice() override;

    uint16_t GetDmDeviceType() override;

    DeviceType GetActiveInputDevice() override;

    int32_t SelectOutputDevice(sptr<AudioRendererFilter> audioRendererFilter,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors) override;

    std::string GetSelectedDeviceInfo(int32_t uid, int32_t pid, AudioStreamType streamType) override;

    int32_t SelectInputDevice(sptr<AudioCapturerFilter> audioCapturerFilter,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors) override;

    int32_t ExcludeOutputDevices(AudioDeviceUsage audioDevUsage,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors) override;

    int32_t UnexcludeOutputDevices(AudioDeviceUsage audioDevUsage,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetExcludedDevices(
        AudioDeviceUsage audioDevUsage) override;

    int32_t SetRingerModeLegacy(AudioRingerMode ringMode) override;

    int32_t SetRingerMode(AudioRingerMode ringMode) override;

    int32_t ConfigDistributedRoutingRole(
        const std::shared_ptr<AudioDeviceDescriptor> descriptor, CastType type) override;

    int32_t SetDistributedRoutingRoleCallback(const sptr<IRemoteObject> &object) override;

    int32_t UnsetDistributedRoutingRoleCallback() override;

#ifdef FEATURE_DTMF_TONE
    std::vector<int32_t> GetSupportedTones(const std::string &countryCode) override;

    std::shared_ptr<ToneInfo> GetToneConfig(int32_t ltonetype, const std::string &countryCode) override;
#endif

    AudioRingerMode GetRingerMode() override;

    int32_t SetAudioScene(AudioScene scene) override;

    int32_t SetMicrophoneMute(bool isMute) override;

    int32_t SetMicrophoneMuteAudioConfig(bool isMute) override;

    int32_t SetMicrophoneMutePersistent(const bool isMute, const PolicyType type) override;

    bool GetPersistentMicMuteState() override;

    bool IsMicrophoneMuteLegacy() override;

    bool IsMicrophoneMute() override;

    AudioScene GetAudioScene() override;

    int32_t ActivateAudioSession(const AudioSessionStrategy &strategy) override;

    int32_t DeactivateAudioSession() override;

    bool IsAudioSessionActivated() override;

    int32_t SetInputDevice(const DeviceType deviceType, const uint32_t sessionID,
        const SourceType sourceType, bool isRunning) override;

    int32_t SetAudioInterruptCallback(const uint32_t sessionID,
        const sptr<IRemoteObject> &object, uint32_t clientUid, const int32_t zoneID = 0) override;

    int32_t UnsetAudioInterruptCallback(const uint32_t sessionID, const int32_t zoneID = 0) override;

    int32_t ActivateAudioInterrupt(AudioInterrupt &audioInterrupt, const int32_t zoneID = 0,
        const bool isUpdatedAudioStrategy = false) override;

    int32_t DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneID = 0) override;

    int32_t ActivatePreemptMode(void) override;

    int32_t DeactivatePreemptMode(void) override;

    int32_t SetAudioManagerInterruptCallback(const int32_t clientId, const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioManagerInterruptCallback(const int32_t clientId) override;

    int32_t SetQueryClientTypeCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetAudioClientInfoMgrCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetQueryBundleNameListCallback(const sptr<IRemoteObject> &object) override;

    int32_t RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt) override;

    int32_t AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt) override;

    AudioStreamType GetStreamInFocus(const int32_t zoneID = 0) override;

    AudioStreamType GetStreamInFocusByUid(const int32_t uid, const int32_t zoneID = 0) override;

    int32_t GetSessionInfoInFocus(AudioInterrupt &audioInterrupt, const int32_t zoneID = 0) override;

    int32_t ReconfigureAudioChannel(const uint32_t &count, DeviceType deviceType) override;

    int32_t GetPreferredOutputStreamType(AudioRendererInfo &rendererInfo) override;

    int32_t GetPreferredInputStreamType(AudioCapturerInfo &capturerInfo) override;

    int32_t CreateRendererClient(
        std::shared_ptr<AudioStreamDescriptor> streamDesc, uint32_t &flag, uint32_t &sessionId) override;

    int32_t CreateCapturerClient(
        std::shared_ptr<AudioStreamDescriptor> streamDesc, uint32_t &flag, uint32_t &sessionId) override;

    int32_t RegisterTracker(AudioMode &mode,
        AudioStreamChangeInfo &streamChangeInfo, const sptr<IRemoteObject> &object) override;

    int32_t UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo) override;

    int32_t GetCurrentRendererChangeInfos(
        std::vector<std::shared_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;

    int32_t GetCurrentCapturerChangeInfos(
        std::vector<std::shared_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos) override;

    int32_t UpdateStreamState(const int32_t clientUid, StreamSetState streamSetState,
        StreamUsage streamUsage) override;

    int32_t GetVolumeGroupInfos(std::string networkId, std::vector<sptr<VolumeGroupInfo>> &infos) override;

    int32_t GetNetworkIdByGroupId(int32_t groupId, std::string &networkId) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetPreferredOutputDeviceDescriptors(
        AudioRendererInfo &rendererInfo, bool forceNoBTPermission) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetPreferredInputDeviceDescriptors(
        AudioCapturerInfo &captureInfo) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetOutputDevice(
        sptr<AudioRendererFilter> audioRendererFilter) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetInputDevice(
        sptr<AudioCapturerFilter> audioCapturerFilter) override;

    int32_t SetClientCallbacksEnable(const CallbackChange &callbackchange, const bool &enable) override;

    int32_t SetCallbackRendererInfo(const AudioRendererInfo &rendererInfo) override;

    int32_t SetCallbackCapturerInfo(const AudioCapturerInfo &capturerInfo) override;

    int32_t GetAudioFocusInfoList(std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList,
        const int32_t zoneID = 0) override;

    int32_t SetSystemSoundUri(const std::string &key, const std::string &uri) override;

    std::string GetSystemSoundUri(const std::string &key) override;

    float GetMinStreamVolume(void) override;

    float GetMaxStreamVolume(void) override;

    int32_t GetMaxRendererInstances() override;

    bool IsVolumeUnadjustable() override;

    int32_t AdjustVolumeByStep(VolumeAdjustType adjustType) override;

    int32_t AdjustSystemVolumeByStep(AudioVolumeType volumeType, VolumeAdjustType adjustType) override;

    float GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType) override;

    int32_t QueryEffectSceneMode(SupportedEffectConfig &supportedEffectConfig) override;

    int32_t GetHardwareOutputSamplingRate(const std::shared_ptr<AudioDeviceDescriptor> &desc) override;

    std::vector<sptr<MicrophoneDescriptor>> GetAudioCapturerMicrophoneDescriptors(int32_t sessionId) override;

    std::vector<sptr<MicrophoneDescriptor>> GetAvailableMicrophones() override;

    int32_t SetDeviceAbsVolumeSupported(const std::string &macAddress, const bool support) override;

    bool IsAbsVolumeScene() override;

    int32_t SetA2dpDeviceVolume(const std::string &macAddress, const int32_t volume, const bool updateUi) override;

    int32_t SetNearlinkDeviceVolume(const std::string &macAddress, AudioVolumeType volumeType,
        const int32_t volume, const bool updateUi) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetAvailableDevices(AudioDeviceUsage usage) override;

    int32_t SetAvailableDeviceChangeCallback(const int32_t clientId, const AudioDeviceUsage usage,
        const sptr<IRemoteObject> &object) override;

    int32_t UnsetAvailableDeviceChangeCallback(const int32_t clientId, AudioDeviceUsage usage) override;

    bool IsSpatializationEnabled() override;

    bool IsSpatializationEnabled(const std::string address) override;

    bool IsSpatializationEnabledForCurrentDevice() override;

    int32_t SetSpatializationEnabled(const bool enable) override;

    int32_t SetSpatializationEnabled(const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice,
        const bool enable) override;

    bool IsHeadTrackingEnabled() override;

    bool IsHeadTrackingEnabled(const std::string address) override;

    int32_t SetHeadTrackingEnabled(const bool enable) override;

    int32_t SetHeadTrackingEnabled(
        const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice, const bool enable) override;

    AudioSpatializationState GetSpatializationState(const StreamUsage streamUsage) override;

    bool IsSpatializationSupported() override;

    bool IsSpatializationSupportedForDevice(const std::string address) override;

    bool IsHeadTrackingSupported() override;

    bool IsHeadTrackingSupportedForDevice(const std::string address) override;

    int32_t UpdateSpatialDeviceState(const AudioSpatialDeviceState audioSpatialDeviceState) override;

    int32_t RegisterSpatializationStateEventListener(const uint32_t sessionID, const StreamUsage streamUsage,
        const sptr<IRemoteObject> &object) override;

    int32_t UnregisterSpatializationStateEventListener(const uint32_t sessionID) override;

    int32_t RegisterPolicyCallbackClient(const sptr<IRemoteObject> &object, const int32_t zoneID = 0) override;

    int32_t CreateAudioInterruptZone(const std::set<int32_t> &pids, const int32_t zoneID) override;

    int32_t AddAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneID) override;

    int32_t RemoveAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneID) override;

    int32_t ReleaseAudioInterruptZone(const int32_t zoneID) override;

    int32_t RegisterAudioZoneClient(const sptr<IRemoteObject>& object) override;

    int32_t CreateAudioZone(const std::string &name, const AudioZoneContext &context) override;

    void ReleaseAudioZone(int32_t zoneId) override;

    const std::vector<std::shared_ptr<AudioZoneDescriptor>> GetAllAudioZone() override;

    const std::shared_ptr<AudioZoneDescriptor> GetAudioZone(int32_t zoneId) override;

    int32_t BindDeviceToAudioZone(int32_t zoneId,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices) override;

    int32_t UnBindDeviceToAudioZone(int32_t zoneId,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> devices) override;

    int32_t EnableAudioZoneReport (bool enable) override;

    int32_t EnableAudioZoneChangeReport(int32_t zoneId, bool enable) override;

    int32_t AddUidToAudioZone(int32_t zoneId, int32_t uid) override;

    int32_t RemoveUidFromAudioZone(int32_t zoneId, int32_t uid) override;

    int32_t EnableSystemVolumeProxy(int32_t zoneId, bool enable) override;

    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioInterruptForZone(int32_t zoneId) override;

    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioInterruptForZone(int32_t zoneId,
        const std::string &deviceTag) override;

    int32_t EnableAudioZoneInterruptReport(int32_t zoneId, const std::string &deviceTag, bool enable) override;

    int32_t InjectInterruptToAudioZone(int32_t zoneId,
        const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts) override;

    int32_t InjectInterruptToAudioZone(int32_t zoneId, const std::string &deviceTag,
        const std::list<std::pair<AudioInterrupt, AudioFocuState>> &interrupts) override;

    int32_t SetCallDeviceActive(InternalDeviceType deviceType, bool active, std::string address,
        const int32_t uid = INVALID_UID) override;

    std::shared_ptr<AudioDeviceDescriptor> GetActiveBluetoothDevice() override;

    ConverterConfig GetConverterConfig() override;

    void FetchOutputDeviceForTrack(AudioStreamChangeInfo &streamChangeInfo,
        const AudioStreamDeviceChangeReasonExt reason) override;

    void FetchInputDeviceForTrack(AudioStreamChangeInfo &streamChangeInfo) override;

    bool IsHighResolutionExist() override;

    int32_t SetHighResolutionExist(bool highResExist) override;

    AudioSpatializationSceneType GetSpatializationSceneType() override;

    int32_t SetSpatializationSceneType(const AudioSpatializationSceneType spatializationSceneType) override;

    float GetMaxAmplitude(const int32_t deviceId) override;

    int32_t DisableSafeMediaVolume() override;

    bool IsHeadTrackingDataRequested(const std::string &macAddress) override;
    int32_t SetAudioDeviceRefinerCallback(const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioDeviceRefinerCallback() override;

    int32_t TriggerFetchDevice(AudioStreamDeviceChangeReasonExt reason) override;

    int32_t SetPreferredDevice(const PreferredType preferredType,
        const std::shared_ptr<AudioDeviceDescriptor> &desc, const int32_t uid = INVALID_UID) override;

    void SaveRemoteInfo(const std::string &networkId, DeviceType deviceType) override;

    int32_t SetAudioDeviceAnahsCallback(const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioDeviceAnahsCallback() override;

    int32_t MoveToNewPipe(const uint32_t sessionId, const AudioPipeType pipeType) override;

    int32_t SetAudioConcurrencyCallback(const uint32_t sessionID, const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioConcurrencyCallback(const uint32_t sessionID) override;

    int32_t ActivateAudioConcurrency(const AudioPipeType &pipeType) override;

    int32_t InjectInterruption(const std::string networkId, InterruptEvent &event) override;

    int32_t LoadSplitModule(const std::string &splitArgs, const std::string &networkId) override;

    bool IsAllowedPlayback(const int32_t &uid, const int32_t &pid) override;

    int32_t SetVoiceRingtoneMute(bool isMute) override;

    int32_t GetSupportedAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray) override;

    int32_t SetAudioEffectProperty(const AudioEffectPropertyArrayV3 &propertyArray) override;

    int32_t GetAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray) override;

    int32_t GetSupportedAudioEffectProperty(AudioEffectPropertyArray &propertyArray) override;

    int32_t GetSupportedAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray) override;

    int32_t SetAudioEffectProperty(const AudioEffectPropertyArray &propertyArray) override;

    int32_t GetAudioEffectProperty(AudioEffectPropertyArray &propertyArray) override;

    int32_t SetAudioEnhanceProperty(const AudioEnhancePropertyArray &propertyArray) override;

    int32_t GetAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray) override;

    int32_t SetVirtualCall(const bool isVirtual) override;

    int32_t SetDeviceConnectionStatus(const std::shared_ptr<AudioDeviceDescriptor> &desc,
        const bool isConnected) override;

    int32_t SetQueryAllowedPlaybackCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetBackgroundMuteCallback(const sptr<IRemoteObject> &object) override;

    int32_t NotifySessionStateChange(const int32_t uid, const int32_t pid, const bool hasSession) override;

    int32_t NotifyFreezeStateChange(const std::set<int32_t> &pidList, const bool isFreeze) override;

    int32_t ResetAllProxy() override;

    DirectPlaybackMode GetDirectPlaybackSupport(const AudioStreamInfo &streamInfo,
        const StreamUsage &streamUsage) override;

    bool IsAcousticEchoCancelerSupported(SourceType sourceType) override;

    bool IsAudioLoopbackSupported(AudioLoopbackMode mode) override;

    int32_t ForceStopAudioStream(StopAudioType audioType) override;

    bool IsCapturerFocusAvailable(const AudioCapturerInfo &capturerInfo) override;

    bool SetKaraokeParameters(const std::string &parameters) override;

    int32_t GetMaxVolumeLevelByUsage(StreamUsage streamUsage) override;

    int32_t GetMinVolumeLevelByUsage(StreamUsage streamUsage) override;

    int32_t GetVolumeLevelByUsage(StreamUsage streamUsage) override;

    bool GetStreamMuteByUsage(StreamUsage streamUsage) override;

    float GetVolumeInDbByStream(StreamUsage streamUsage, int32_t volumeLevel, DeviceType deviceType) override;

    std::vector<AudioVolumeType> GetSupportedAudioVolumeTypes() override;

    AudioVolumeType GetAudioVolumeTypeByStreamUsage(StreamUsage streamUsage) override;

    std::vector<StreamUsage> GetStreamUsagesByVolumeType(AudioVolumeType audioVolumeType) override;

    int32_t SetCallbackStreamUsageInfo(const std::set<StreamUsage> &streamUsages) override;
    int32_t UpdateDeviceInfo(const std::shared_ptr<AudioDeviceDescriptor> &deviceDesc,
        const DeviceInfoUpdateCommand command) override;
    int32_t SetSleAudioOperationCallback(const sptr<IRemoteObject> &object) override;

    bool IsCollaborativePlaybackSupported() override;

    bool IsCollaborativePlaybackEnabledForDevice(
        const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice) override;

    int32_t SetCollaborativePlaybackEnabledForDevice(
        const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice, const bool enable) override;
private:
    static inline BrokerDelegator<AudioPolicyProxy> mDdelegator;
    void WriteStreamChangeInfo(MessageParcel &data, const AudioMode &mode,
        const AudioStreamChangeInfo &streamChangeInfo);
    void ReadAudioFocusInfo(
        MessageParcel &reply, std::list<std::pair<AudioInterrupt, AudioFocuState>> &focusInfoList);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_POLICY_PROXY_H
