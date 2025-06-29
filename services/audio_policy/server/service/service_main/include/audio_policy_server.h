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

#ifndef ST_AUDIO_POLICY_SERVER_H
#define ST_AUDIO_POLICY_SERVER_H

#include <mutex>
#include <pthread.h>

#include "singleton.h"
#include "system_ability_definition.h"
#include "ipc_skeleton.h"
#include "system_ability.h"
#include "iservice_registry.h"

#include "accesstoken_kit.h"
#include "perm_state_change_callback_customize.h"
#include "power_state_callback_stub.h"
#include "power_state_listener.h"
#include "common_event_subscriber.h"
#include "common_event_support.h"

#include "bundle_mgr_interface.h"
#include "bundle_mgr_proxy.h"

#include "audio_policy_service.h"
#include "audio_policy_utils.h"
#include "audio_stream_removed_callback.h"
#include "audio_interrupt_callback.h"
#include "audio_policy_manager_stub.h"
#include "audio_policy_client_proxy.h"
#include "audio_server_death_recipient.h"
#include "session_processor.h"
#include "audio_collaborative_service.h"
#include "audio_spatialization_service.h"
#include "audio_policy_server_handler.h"
#include "audio_interrupt_service.h"
#include "audio_device_manager.h"
#include "audio_policy_dump.h"
#include "app_state_listener.h"
#include "audio_core_service.h"
#include "audio_converter_parser.h"
#include "audio_usb_manager.h"

namespace OHOS {
namespace AudioStandard {

class AudioPolicyService;
class AudioInterruptService;
class AudioPolicyServerHandler;
class AudioSessionService;
class BluetoothEventSubscriber;

class AudioPolicyServer : public SystemAbility,
                          public AudioPolicyManagerStub,
                          public AudioStreamRemovedCallback {
    DECLARE_SYSTEM_ABILITY(AudioPolicyServer);

public:
    DISALLOW_COPY_AND_MOVE(AudioPolicyServer);

    enum DeathRecipientId {
        TRACKER_CLIENT = 0,
        LISTENER_CLIENT
    };

    explicit AudioPolicyServer(int32_t systemAbilityId, bool runOnCreate = true);

    virtual ~AudioPolicyServer()
    {
        AUDIO_WARNING_LOG("dtor should not happen");
    };

    void OnDump() override;
    void OnStart() override;
    void OnStop() override;

    int32_t GetMaxVolumeLevel(AudioVolumeType volumeType) override;

    int32_t GetMinVolumeLevel(AudioVolumeType volumeType) override;

    int32_t SetSystemVolumeLevelLegacy(AudioVolumeType volumeType, int32_t volumeLevel) override;

    int32_t SetSystemVolumeLevel(AudioVolumeType volumeType, int32_t volumeLevel, int32_t volumeFlag = 0) override;

    int32_t SetSystemVolumeLevelWithDevice(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType,
        int32_t volumeFlag = 0) override;

    int32_t SetAppVolumeLevel(int32_t appUid, int32_t volumeLevel, int32_t volumeFlag = 0) override;

    int32_t IsAppVolumeMute(int32_t appUid, bool owned, bool &isMute) override;

    int32_t SetAppVolumeMuted(int32_t appUid, bool muted, int32_t volumeFlag = 0) override;

    int32_t SetSelfAppVolumeLevel(int32_t volumeLevel, int32_t volumeFlag = 0) override;

    AudioStreamType GetSystemActiveVolumeType(const int32_t clientUid) override;

    int32_t GetSystemVolumeLevel(AudioStreamType streamType) override;

    int32_t GetAppVolumeLevel(int32_t appUid, int32_t &volumeLevel) override;

    int32_t GetSelfAppVolumeLevel(int32_t &volumeLevel) override;

    int32_t SetLowPowerVolume(int32_t streamId, float volume) override;

    float GetLowPowerVolume(int32_t streamId) override;

    float GetSingleStreamVolume(int32_t streamId) override;

    int32_t SetStreamMuteLegacy(AudioStreamType streamType, bool mute,
        const DeviceType &deviceType = DEVICE_TYPE_NONE) override;

    int32_t SetStreamMute(AudioStreamType streamType, bool mute,
        const DeviceType &deviceType = DEVICE_TYPE_NONE) override;

    bool GetStreamMute(AudioStreamType streamType) override;

    bool IsStreamActive(AudioStreamType streamType) override;

    bool IsStreamActiveByStreamUsage(StreamUsage streamUsage) override;

    bool IsFastPlaybackSupported(AudioStreamInfo &streamInfo, StreamUsage usage) override;
    bool IsFastRecordingSupported(AudioStreamInfo &streamInfo, SourceType source) override;

    bool IsVolumeUnadjustable() override;

    int32_t AdjustVolumeByStep(VolumeAdjustType adjustType) override;

    int32_t AdjustSystemVolumeByStep(AudioVolumeType volumeType, VolumeAdjustType adjustType) override;

    float GetSystemVolumeInDb(AudioVolumeType volumeType, int32_t volumeLevel, DeviceType deviceType) override;

    bool IsArmUsbDevice(const AudioDeviceDescriptor &desc) override;

    void MapExternalToInternalDeviceType(AudioDeviceDescriptor &desc) override;

    int32_t SelectOutputDevice(sptr<AudioRendererFilter> audioRendererFilter,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors) override;

    std::string GetSelectedDeviceInfo(int32_t uid, int32_t pid, AudioStreamType streamType) override;

    int32_t SelectInputDevice(sptr<AudioCapturerFilter> audioCapturerFilter,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> audioDeviceDescriptors) override;

    int32_t ExcludeOutputDevices(AudioDeviceUsage audioDevUsage,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors) override;

    int32_t UnexcludeOutputDevices(AudioDeviceUsage audioDevUsage,
        std::vector<std::shared_ptr<AudioDeviceDescriptor>> &audioDeviceDescriptors) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetExcludedDevices(AudioDeviceUsage audioDevUsage) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetDevices(DeviceFlag deviceFlag) override;

    std::vector<std::shared_ptr<AudioDeviceDescriptor>> GetDevicesInner(DeviceFlag deviceFlag) override;

    int32_t SetDeviceActive(InternalDeviceType deviceType, bool active, const int32_t uid = INVALID_UID) override;

    bool IsDeviceActive(InternalDeviceType deviceType) override;

    InternalDeviceType GetActiveOutputDevice() override;

    uint16_t GetDmDeviceType() override;

    InternalDeviceType GetActiveInputDevice() override;

    int32_t SetRingerModeLegacy(AudioRingerMode ringMode) override;

    int32_t SetRingerMode(AudioRingerMode ringMode) override;

#ifdef FEATURE_DTMF_TONE
    std::vector<int32_t> GetSupportedTones(const std::string &countryCode) override;

    std::shared_ptr<ToneInfo> GetToneConfig(int32_t ltonetype, const std::string &countryCode) override;
#endif

    AudioRingerMode GetRingerMode() override;

    int32_t SetAudioScene(AudioScene audioScene) override;

    int32_t SetMicrophoneMuteCommon(bool isMute, bool isLegacy);

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

    int32_t SetAudioInterruptCallback(const uint32_t sessionID,
        const sptr<IRemoteObject> &object, uint32_t clientUid, const int32_t zoneId = 0) override;

    int32_t UnsetAudioInterruptCallback(const uint32_t sessionID, const int32_t zoneId = 0) override;

    int32_t ActivateAudioInterrupt(AudioInterrupt &audioInterrupt, const int32_t zoneId = 0,
        const bool isUpdatedAudioStrategy = false) override;

    int32_t DeactivateAudioInterrupt(const AudioInterrupt &audioInterrupt, const int32_t zoneId = 0) override;

    int32_t ActivatePreemptMode(void) override;

    int32_t DeactivatePreemptMode(void) override;

    int32_t SetAudioManagerInterruptCallback(const int32_t clientId, const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioManagerInterruptCallback(const int32_t clientId) override;

    int32_t SetQueryClientTypeCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetAudioClientInfoMgrCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetQueryBundleNameListCallback(const sptr<IRemoteObject> &object) override;

    int32_t RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt) override;

    int32_t AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt) override;

    AudioStreamType GetStreamInFocus(const int32_t zoneId = 0) override;

    AudioStreamType GetStreamInFocusByUid(const int32_t uid, const int32_t zoneId = 0) override;

    int32_t GetSessionInfoInFocus(AudioInterrupt &audioInterrupt, const int32_t zoneId = 0) override;

    void OnAudioStreamRemoved(const uint64_t sessionID) override;

    void ProcessSessionRemoved(const uint64_t sessionID, const int32_t zoneId = 0);

    void ProcessSessionAdded(SessionEvent sessionEvent);

    void ProcessorCloseWakeupSource(const uint64_t sessionID);

    int32_t Dump(int32_t fd, const std::vector<std::u16string> &args) override;

    int32_t ReconfigureAudioChannel(const uint32_t &count, DeviceType deviceType) override;

    int32_t GetPreferredOutputStreamType(AudioRendererInfo &rendererInfo) override;

    int32_t GetPreferredInputStreamType(AudioCapturerInfo &capturerInfo) override;

    int32_t CreateRendererClient(
        std::shared_ptr<AudioStreamDescriptor> streamDesc, uint32_t &flag, uint32_t &sessionId) override;

    int32_t CreateCapturerClient(
        std::shared_ptr<AudioStreamDescriptor> streamDesc, uint32_t &flag, uint32_t &sessionId) override;

    int32_t RegisterTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo,
        const sptr<IRemoteObject> &object) override;

    int32_t UpdateTracker(AudioMode &mode, AudioStreamChangeInfo &streamChangeInfo) override;

    int32_t GetCurrentRendererChangeInfos(
        std::vector<std::shared_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;

    int32_t GetCurrentCapturerChangeInfos(
        std::vector<std::shared_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos) override;

    void RegisterClientDeathRecipient(const sptr<IRemoteObject> &object, DeathRecipientId id);

    void RegisteredTrackerClientDied(int pid, int uid);

    void RegisteredStreamListenerClientDied(int pid, int uid);

    int32_t ResumeStreamState();

    int32_t UpdateStreamState(const int32_t clientUid, StreamSetState streamSetState,
        StreamUsage streamUsage) override;

    int32_t GetVolumeGroupInfos(std::string networkId, std::vector<sptr<VolumeGroupInfo>> &infos) override;

    int32_t GetSupportedAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray) override;
    int32_t SetAudioEffectProperty(const AudioEffectPropertyArrayV3 &propertyArray) override;
    int32_t GetAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray) override;

    int32_t GetSupportedAudioEffectProperty(AudioEffectPropertyArray &propertyArray) override;
    int32_t GetSupportedAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray) override;
    int32_t SetAudioEffectProperty(const AudioEffectPropertyArray &propertyArray) override;
    int32_t GetAudioEffectProperty(AudioEffectPropertyArray &propertyArray) override;
    int32_t SetAudioEnhanceProperty(const AudioEnhancePropertyArray &propertyArray) override;
    int32_t GetAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray) override;
    bool IsAcousticEchoCancelerSupported(SourceType sourceType) override;
    bool IsAudioLoopbackSupported(AudioLoopbackMode mode) override;
    bool SetKaraokeParameters(const std::string &parameters) override;

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
        const int32_t zoneId = 0) override;

    int32_t SetSystemSoundUri(const std::string &key, const std::string &uri) override;

    std::string GetSystemSoundUri(const std::string &key) override;

    float GetMinStreamVolume(void) override;

    float GetMaxStreamVolume(void) override;

    int32_t GetMaxRendererInstances() override;

    void GetStreamVolumeInfoMap(StreamVolumeInfoMap& streamVolumeInfos);

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

    int32_t SetAvailableDeviceChangeCallback(const int32_t /*clientId*/, const AudioDeviceUsage usage,
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

    int32_t ConfigDistributedRoutingRole(
        const std::shared_ptr<AudioDeviceDescriptor> descriptor, CastType type) override;

    int32_t SetDistributedRoutingRoleCallback(const sptr<IRemoteObject> &object) override;

    int32_t UnsetDistributedRoutingRoleCallback() override;

    int32_t UnregisterSpatializationStateEventListener(const uint32_t sessionID) override;

    int32_t RegisterPolicyCallbackClient(const sptr<IRemoteObject> &object, const int32_t zoneId = 0) override;

    int32_t CreateAudioInterruptZone(const std::set<int32_t> &pids, const int32_t zoneId) override;

    int32_t AddAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneId) override;

    int32_t RemoveAudioInterruptZonePids(const std::set<int32_t> &pids, const int32_t zoneId) override;

    int32_t ReleaseAudioInterruptZone(const int32_t zoneId) override;

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

    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioInterruptForZone(
        int32_t zoneId, const std::string &deviceTag) override;

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

    AudioSpatializationSceneType GetSpatializationSceneType() override;

    int32_t SetSpatializationSceneType(const AudioSpatializationSceneType spatializationSceneType) override;

    float GetMaxAmplitude(const int32_t deviceId) override;

    int32_t DisableSafeMediaVolume() override;

    bool IsHeadTrackingDataRequested(const std::string &macAddress) override;

    int32_t SetAudioDeviceRefinerCallback(const sptr<IRemoteObject> &object) override;

    int32_t UnsetAudioDeviceRefinerCallback() override;

    int32_t TriggerFetchDevice(
        AudioStreamDeviceChangeReasonExt reason = AudioStreamDeviceChangeReason::UNKNOWN) override;

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

    int32_t SetInputDevice(const DeviceType deviceType, const uint32_t sessionID,
        const SourceType sourceType, bool isRunning) override;

    int32_t LoadSplitModule(const std::string &splitArgs, const std::string &networkId) override;

    bool IsAllowedPlayback(const int32_t &uid, const int32_t &pid) override;

    int32_t SetVoiceRingtoneMute(bool isMute) override;

    int32_t NotifySessionStateChange(const int32_t uid, const int32_t pid, const bool hasSession) override;

    int32_t NotifyFreezeStateChange(const std::set<int32_t> &pidList, const bool isFreeze) override;

    int32_t ResetAllProxy() override;

    int32_t SetVirtualCall(const bool isVirtual) override;

    int32_t SetDeviceConnectionStatus(const std::shared_ptr<AudioDeviceDescriptor> &desc,
        const bool isConnected) override;

    int32_t SetQueryAllowedPlaybackCallback(const sptr<IRemoteObject> &object) override;

    int32_t SetBackgroundMuteCallback(const sptr<IRemoteObject> &object) override;

    DirectPlaybackMode GetDirectPlaybackSupport(const AudioStreamInfo &streamInfo,
        const StreamUsage &streamUsage) override;

    int32_t GetMaxVolumeLevelByUsage(StreamUsage streamUsage) override;

    int32_t GetMinVolumeLevelByUsage(StreamUsage streamUsage) override;

    int32_t GetVolumeLevelByUsage(StreamUsage streamUsage) override;

    bool GetStreamMuteByUsage(StreamUsage streamUsage) override;

    float GetVolumeInDbByStream(StreamUsage streamUsage, int32_t volumeLevel, DeviceType deviceType) override;

    std::vector<AudioVolumeType> GetSupportedAudioVolumeTypes() override;

    AudioVolumeType GetAudioVolumeTypeByStreamUsage(StreamUsage streamUsage) override;

    std::vector<StreamUsage> GetStreamUsagesByVolumeType(AudioVolumeType audioVolumeType) override;

    int32_t SetCallbackStreamUsageInfo(const std::set<StreamUsage> &streamUsages) override;

    int32_t ForceStopAudioStream(StopAudioType audioType) override;

    bool IsCapturerFocusAvailable(const AudioCapturerInfo &capturerInfo) override;

    void ProcessRemoteInterrupt(std::set<int32_t> sessionIds, InterruptEventInternal interruptEvent);

    void SendVolumeKeyEventCbWithUpdateUiOrNot(AudioStreamType streamType, const bool& isUpdateUi = false);
    void SendMuteKeyEventCbWithUpdateUiOrNot(AudioStreamType streamType, const bool& isUpdateUi = false);
    void UpdateMuteStateAccordingToVolLevel(AudioStreamType streamType, int32_t volumeLevel,
        bool mute, const bool& isUpdateUi = false);

    void ProcUpdateRingerMode();
    uint32_t TranslateErrorCode(int32_t result);

    int32_t SetCollaborativePlaybackEnabledForDevice(
        const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice, bool enabled) override;
    
    bool IsCollaborativePlaybackEnabledForDevice(
        const std::shared_ptr<AudioDeviceDescriptor> &selectedAudioDevice) override;

    bool IsCollaborativePlaybackSupported() override;

    class RemoteParameterCallback : public AudioParameterCallback {
    public:
        RemoteParameterCallback(sptr<AudioPolicyServer> server);
        // AudioParameterCallback
        void OnAudioParameterChange(const std::string networkId, const AudioParamKey key, const std::string& condition,
            const std::string& value) override;
    private:
        sptr<AudioPolicyServer> server_;
        void VolumeOnChange(const std::string networkId, const std::string& condition);
        void InterruptOnChange(const std::string networkId, const std::string& condition);
        void StateOnChange(const std::string networkId, const std::string& condition, const std::string& value);
    };

    std::shared_ptr<RemoteParameterCallback> remoteParameterCallback_;

    class PerStateChangeCbCustomizeCallback : public Security::AccessToken::PermStateChangeCallbackCustomize {
    public:
        explicit PerStateChangeCbCustomizeCallback(const Security::AccessToken::PermStateChangeScope &scopeInfo,
            sptr<AudioPolicyServer> server) : PermStateChangeCallbackCustomize(scopeInfo),
            ready_(false), server_(server) {}
        ~PerStateChangeCbCustomizeCallback() {}

        void PermStateChangeCallback(Security::AccessToken::PermStateChangeInfo& result);
        void UpdateMicPrivacyByCapturerState(bool targetMuteState, uint32_t targetTokenId, int32_t appUid);

        bool ready_;
    private:
        sptr<AudioPolicyServer> server_;
    };

    bool IsHighResolutionExist() override;

    int32_t SetHighResolutionExist(bool highResExist) override;

    void NotifyAccountsChanged(const int &id);

    // for hidump
    void AudioDevicesDump(std::string &dumpString);
    void AudioModeDump(std::string &dumpString);
    void AudioInterruptZoneDump(std::string &dumpString);
    void AudioPolicyParserDump(std::string &dumpString);
    void AudioVolumeDump(std::string &dumpString);
    void AudioStreamDump(std::string &dumpString);
    void OffloadStatusDump(std::string &dumpString);
    void XmlParsedDataMapDump(std::string &dumpString);
    void EffectManagerInfoDump(std::string &dumpString);
    void MicrophoneMuteInfoDump(std::string &dumpString);
    void AudioSessionInfoDump(std::string &dumpString);
    void AudioPipeManagerDump(std::string &dumpString);

    // for hibernate callback
    void CheckHibernateState(bool hibernate);
    // for S4 reboot update safevolume
    void UpdateSafeVolumeByS4();

    void CheckConnectedDevice();
    void SetDeviceConnectedFlagFalseAfterDuration();

    int32_t UpdateDeviceInfo(const std::shared_ptr<AudioDeviceDescriptor> &deviceDesc,
        const DeviceInfoUpdateCommand command) override;
    int32_t SetSleAudioOperationCallback(const sptr<IRemoteObject> &object) override;

protected:
    void OnAddSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
    void RegisterParamCallback();

    void OnRemoveSystemAbility(int32_t systemAbilityId, const std::string& deviceId) override;
    int32_t GetApiTargetVersion() override;

private:
    friend class AudioInterruptService;

    static constexpr int32_t MAX_VOLUME_LEVEL = 15;
    static constexpr int32_t MIN_VOLUME_LEVEL = 0;
    static constexpr int32_t VOLUME_CHANGE_FACTOR = 1;
    static constexpr int32_t VOLUME_KEY_DURATION = 0;
    static constexpr int32_t VOLUME_MUTE_KEY_DURATION = 0;
    static constexpr int32_t MEDIA_SERVICE_UID = 1013;
    static constexpr int32_t EDM_SERVICE_UID = 3057;
    static constexpr char DAUDIO_DEV_TYPE_SPK = '1';
    static constexpr char DAUDIO_DEV_TYPE_MIC = '2';
    static constexpr int32_t AUDIO_UID = 1041;
    static constexpr uint32_t MICPHONE_CALLER = 0;
    static constexpr int32_t ROOT_UID = 0;
    static constexpr int32_t PREEMPT_UID = 7015;

    static const std::list<uid_t> RECORD_ALLOW_BACKGROUND_LIST;
    static const std::list<uid_t> RECORD_PASS_APPINFO_LIST;
    static constexpr const char* MICROPHONE_CONTROL_PERMISSION = "ohos.permission.MICROPHONE_CONTROL";

    class AudioPolicyServerPowerStateCallback : public PowerMgr::PowerStateCallbackStub {
    public:
        AudioPolicyServerPowerStateCallback(AudioPolicyServer *policyServer);
        void OnAsyncPowerStateChanged(PowerMgr::PowerState state) override;

    private:
        AudioPolicyServer *policyServer_;
    };

    int32_t VerifyVoiceCallPermission(uint64_t fullTokenId, Security::AccessToken::AccessTokenID tokenId);

    // offload session
    void CheckSubscribePowerStateChange();
    void CheckStreamMode(const int64_t activateSessionId);
    bool CheckAudioSessionStrategy(const AudioSessionStrategy &sessionStrategy);

    // for audio volume and mute status
    int32_t SetRingerModeInternal(AudioRingerMode inputRingerMode, bool hasUpdatedVolume = false);
    int32_t SetSystemVolumeLevelInternal(AudioStreamType streamType, int32_t volumeLevel, bool isUpdateUi);
    int32_t SetAppVolumeLevelInternal(int32_t appUid, int32_t volumeLevel, bool isUpdateUi);
    int32_t SetAppVolumeMutedInternal(int32_t appUid, bool muted, bool isUpdateUi);
    int32_t SetSystemVolumeLevelWithDeviceInternal(AudioStreamType streamType, int32_t volumeLevel,
        bool isUpdateUi, DeviceType deviceType);
    int32_t SetSingleStreamVolume(AudioStreamType streamType, int32_t volumeLevel, bool isUpdateUi, bool mute);
    int32_t SetAppSingleStreamVolume(int32_t streamType, int32_t volumeLevel, bool isUpdateUi);
    int32_t SetSingleStreamVolumeWithDevice(AudioStreamType streamType, int32_t volumeLevel, bool isUpdateUi,
        DeviceType deviceType);
    AudioStreamType GetSystemActiveVolumeTypeInternal(const int32_t clientUid);
    int32_t GetSystemVolumeLevelInternal(AudioStreamType streamType);
    int32_t GetAppVolumeLevelInternal(int32_t appUid, int32_t &volumeLevel);
    int32_t GetSystemVolumeLevelNoMuteState(AudioStreamType streamType);
    float GetSystemVolumeDb(AudioStreamType streamType);
    int32_t SetStreamMuteInternal(AudioStreamType streamType, bool mute, bool isUpdateUi,
        const DeviceType &deviceType = DEVICE_TYPE_NONE);
    void UpdateSystemMuteStateAccordingMusicState(AudioStreamType streamType, bool mute, bool isUpdateUi);
    void ProcUpdateRingerModeForMute(bool updateRingerMode, bool mute);
    int32_t SetSingleStreamMute(AudioStreamType streamType, bool mute, bool isUpdateUi,
        const DeviceType &deviceType = DEVICE_TYPE_NONE);
    bool GetStreamMuteInternal(AudioStreamType streamType);
    bool IsVolumeTypeValid(AudioStreamType streamType);
    bool IsVolumeLevelValid(AudioStreamType streamType, int32_t volumeLevel);
    bool CheckCanMuteVolumeTypeByStep(AudioVolumeType volumeType, int32_t volumeLevel);

    // Permission and privacy
    bool VerifyPermission(const std::string &permission, uint32_t tokenId = 0, bool isRecording = false);
    bool VerifyBluetoothPermission();
    int32_t OffloadStopPlaying(const AudioInterrupt &audioInterrupt);
    int32_t SetAudioSceneInternal(AudioScene audioScene, const int32_t uid = INVALID_UID,
        const int32_t pid = INVALID_PID);

    // externel function call
#ifdef FEATURE_MULTIMODALINPUT_INPUT
    bool MaxOrMinVolumeOption(const int32_t &volLevel, const int32_t keyType, const AudioStreamType &streamInFocus);
    int32_t RegisterVolumeKeyEvents(const int32_t keyType);
    int32_t RegisterVolumeKeyMuteEvents();
    void SubscribeVolumeKeyEvents();
    int32_t ProcessVolumeKeyEvents(const int32_t keyType);
#endif
    void AddAudioServiceOnStart();
    void SubscribeOsAccountChangeEvents();
    void SubscribePowerStateChangeEvents();
    void SubscribeCommonEvent(const std::string event);
    void OnReceiveEvent(const EventFwk::CommonEventData &eventData);
    void HandleKvDataShareEvent();
    void InitMicrophoneMute();
    void InitKVStore();
    void NotifySettingsDataReady();
    void ConnectServiceAdapter();
    void LoadEffectLibrary();
    void RegisterBluetoothListener();
    void SubscribeAccessibilityConfigObserver();
    void RegisterDataObserver();
    void RegisterPowerStateListener();
    void UnRegisterPowerStateListener();
    void RegisterSyncHibernateListener();
    void UnRegisterSyncHibernateListener();
    void RegisterAppStateListener();
    void AddRemoteDevstatusCallback();
    void OnDistributedRoutingRoleChange(const std::shared_ptr<AudioDeviceDescriptor> descriptor, const CastType type);
    void SubscribeSafeVolumeEvent();
    void SubscribeCommonEventExecute();
    void SubscribeBackgroundTask();
    void SendMonitrtEvent(const int32_t keyType, int32_t resultOfVolumeKey);
    void RegisterDefaultVolumeTypeListener();

    void InitPolicyDumpMap();
    void PolicyDataDump(std::string &dumpString);
    void ArgInfoDump(std::string &dumpString, std::queue<std::u16string> &argQue);
    void InfoDumpHelp(std::string &dumpString);

    int32_t SetRingerModeInner(AudioRingerMode ringMode);
    void AddSystemAbilityListeners();
    void OnAddSystemAbilityExtract(int32_t systemAbilityId, const std::string& deviceId);

    // for updating default device selection state when game audio stream is muted
    void UpdateDefaultOutputDeviceWhenStarting(const uint32_t sessionID);
    void UpdateDefaultOutputDeviceWhenStopping(const uint32_t sessionID);
    void ChangeVolumeOnVoiceAssistant(AudioStreamType &streamInFocus);

    AudioEffectService &audioEffectService_;
    AudioAffinityManager &audioAffinityManager_;
    AudioCapturerSession &audioCapturerSession_;
    AudioStateManager &audioStateManager_;
    AudioToneManager &audioToneManager_;
    AudioMicrophoneDescriptor &audioMicrophoneDescriptor_;
    AudioDeviceStatus &audioDeviceStatus_;
    AudioPolicyConfigManager &audioConfigManager_;
    AudioSceneManager &audioSceneManager_;
    AudioConnectedDevice &audioConnectedDevice_;
    AudioDeviceLock &audioDeviceLock_;
    AudioStreamCollector &streamCollector_;
    AudioOffloadStream &audioOffloadStream_;
    AudioBackgroundManager &audioBackgroundManager_;
    AudioVolumeManager &audioVolumeManager_;
    AudioDeviceCommon &audioDeviceCommon_;
    IAudioPolicyInterface &audioPolicyManager_;
    AudioPolicyConfigManager &audioPolicyConfigManager_;
    AudioPolicyService &audioPolicyService_;
    AudioPolicyUtils &audioPolicyUtils_;
    AudioDeviceManager &audioDeviceManager_;
    AudioSpatializationService &audioSpatializationService_;
    AudioCollaborativeService &audioCollaborativeService_;
    AudioRouterCenter &audioRouterCenter_;
    AudioPolicyDump &audioPolicyDump_;
    AudioActiveDevice &audioActiveDevice_;
    AudioUsbManager &usbManager_;

    std::shared_ptr<AudioInterruptService> interruptService_;
    std::shared_ptr<AudioCoreService> coreService_;
    std::shared_ptr<AudioCoreService::EventEntry> eventEntry_;

    int32_t volumeStep_;
    std::atomic<bool> isFirstAudioServiceStart_ = false;
    std::atomic<bool> isInitMuteState_ = false;
    std::atomic<bool> isInitSettingsData_ = false;
    std::atomic<bool> isScreenOffOrLock_ = false;
#ifdef FEATURE_MULTIMODALINPUT_INPUT
    std::atomic<bool> hasSubscribedVolumeKeyEvents_ = false;
#endif
    std::vector<pid_t> clientDiedListenerState_;
    sptr<PowerStateListener> powerStateListener_;
    sptr<SyncHibernateListener> syncHibernateListener_;
    bool powerStateCallbackRegister_;
    AppExecFwk::AppMgrClient appManager_;
    sptr<AppStateListener> appStateListener_;

    std::mutex systemVolumeMutex_;
    std::mutex micStateChangeMutex_;
    std::mutex clientDiedListenerStateMutex_;
    std::mutex subscribeVolumeKey_;

    SessionProcessor sessionProcessor_{
        [this] (const uint64_t sessionID, const int32_t zoneID) { this->ProcessSessionRemoved(sessionID, zoneID); },
        [this] (SessionEvent sessionEvent) { this->ProcessSessionAdded(sessionEvent); },
        [this] (const uint64_t sessionID) {this->ProcessorCloseWakeupSource(sessionID); }};

    std::shared_ptr<AudioPolicyServerHandler> audioPolicyServerHandler_;
    bool volumeApplyToAll_ = false;
    bool supportVibrator_ = false;

    bool isHighResolutionExist_ = false;
    std::mutex descLock_;

    using DumpFunc = void(AudioPolicyServer::*)(std::string &dumpString);
    std::map<std::u16string, DumpFunc> dumpFuncMap;
    pid_t lastMicMuteSettingPid_ = 0;
    std::shared_ptr<AudioOsAccountInfo> accountObserver_ = nullptr;

    int32_t sessionIdByRemote_ = -1;
    sptr<IStandardAudioPolicyManagerListener> queryBundleNameListCallback_ = nullptr;
};

class AudioOsAccountInfo : public AccountSA::OsAccountSubscriber {
public:
    explicit AudioOsAccountInfo(const AccountSA::OsAccountSubscribeInfo &subscribeInfo,
        AudioPolicyServer *audioPolicyServer) : AccountSA::OsAccountSubscriber(subscribeInfo),
        audioPolicyServer_(audioPolicyServer) {}

    ~AudioOsAccountInfo()
    {
        AUDIO_WARNING_LOG("Destructor AudioOsAccountInfo");
    }

    void OnAccountsChanged(const int &id) override
    {
        AUDIO_INFO_LOG("OnAccountsChanged received, id: %{public}d", id);
    }

    void OnAccountsSwitch(const int &newId, const int &oldId) override
    {
        CHECK_AND_RETURN_LOG(oldId >= LOCAL_USER_ID, "invalid id");
        AUDIO_INFO_LOG("OnAccountsSwitch received, newid: %{public}d, oldid: %{public}d", newId, oldId);
        if (audioPolicyServer_ != nullptr) {
            audioPolicyServer_->NotifyAccountsChanged(newId);
        }
    }
private:
    static constexpr int32_t LOCAL_USER_ID = 100;
    AudioPolicyServer *audioPolicyServer_;
};

class AudioCommonEventSubscriber : public EventFwk::CommonEventSubscriber {
public:
    explicit AudioCommonEventSubscriber(const EventFwk::CommonEventSubscribeInfo &subscribeInfo,
        std::function<void(const EventFwk::CommonEventData&)> receiver)
        : EventFwk::CommonEventSubscriber(subscribeInfo), eventReceiver_(receiver) {}
    ~AudioCommonEventSubscriber() {}
    void OnReceiveEvent(const EventFwk::CommonEventData &eventData) override;
private:
    AudioCommonEventSubscriber() = default;
    std::function<void(const EventFwk::CommonEventData&)> eventReceiver_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_POLICY_SERVER_H
