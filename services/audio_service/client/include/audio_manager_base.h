/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef I_ST_AUDIO_MANAGER_BASE_H
#define I_ST_AUDIO_MANAGER_BASE_H

#include "ipc_types.h"
#include "iremote_broker.h"
#include "iremote_proxy.h"
#include "iremote_stub.h"
#include "audio_effect.h"
#include "pulseaudio_ipc_interface_code.h"
#include "audio_asr.h"
#include "hdi_adapter_type.h"
#include "hdi_adapter_info.h"
#include "audio_stutter.h"

namespace OHOS {
namespace AudioStandard {
class AudioDeviceDescriptor;
class IStandardAudioService : public IRemoteBroker {
public:
    /**
     * Sets Microphone Mute status.
     *
     * @param isMute Mute status true or false to be set.
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t SetMicrophoneMute(bool isMute) = 0;

    /**
     * @brief Set the Voice Volume.
     *
     * @param volume Voice colume to be set.
     * @return int32_t Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t SetVoiceVolume(float volume) = 0;

    virtual int32_t OffloadSetVolume(float volume) = 0;
    virtual int32_t SuspendRenderSink(const std::string &sinkName) = 0;
    virtual int32_t RestoreRenderSink(const std::string &sinkName) = 0;

    /**
     * Sets Audio modes.
     *
     * @param audioScene Audio scene type.
     * @param activeDevice Currently active priority device
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t SetAudioScene(AudioScene audioScene, std::vector<DeviceType> &activeOutputDevices,
        DeviceType activeInputDevice, BluetoothOffloadState a2dpOffloadFlag, bool scoExcludeFlag = false) = 0;

    /**
     * Set Audio Parameter.
     *
     * @param  key for the audio parameter to be set
     * @param  value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual void SetAudioParameter(const std::string &key, const std::string &value) = 0;

    /**
     * Set Asr Aec Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t SetAsrAecMode(AsrAecMode asrAecMode) = 0;

    /**
     * Set Asr Aec Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t GetAsrAecMode(AsrAecMode &asrAecMode) = 0;

    /**
     * Set Asr Aec Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t SetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode asrNoiseSuppressionMode) = 0;

    /**
     * Set Asr Aec Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t GetAsrNoiseSuppressionMode(AsrNoiseSuppressionMode &asrNoiseSuppressionMode) = 0;

    /**
     * Set Asr WhisperDetection Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t SetAsrWhisperDetectionMode(AsrWhisperDetectionMode asrWhisperDetectionMode) = 0;

    /**
     * Get Asr WhisperDetection Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t GetAsrWhisperDetectionMode(AsrWhisperDetectionMode &asrWhisperDetectionMode) = 0;

    /**
     * Set Voice Control Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t SetAsrVoiceControlMode(AsrVoiceControlMode asrVoiceControlMode, bool on) = 0;

    /**
     * Set Voice Mute Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t SetAsrVoiceMuteMode(AsrVoiceMuteMode asrVoiceMuteMode, bool on) = 0;

    /**
     * Set Asr Aec Mode.
     *
     * @param key for the audio parameter to be set
     * @param value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual int32_t IsWhispering() = 0;

    /**
     * Set Audio Parameter.
     *
     * @param  networkId for the distributed device
     * @param  key for the audio parameter to be set
     * @param  condition for the audio parameter to be set
     * @param  value associated with the key for the audio parameter to be set
     * @return none.
     */
    virtual void SetAudioParameter(const std::string& networkId, const AudioParamKey key, const std::string& condition,
        const std::string& value) = 0;

    /**
     * Get Audio Parameter.
     *
     * @param  key for the audio parameter to be set
     * @return Returns value associated to the key requested.
     */
    virtual const std::string GetAudioParameter(const std::string &key) = 0;

    /**
     * Set Audio Parameter.
     *
     * @param  networkId for the distributed device
     * @param  key for the audio parameter to be set
     * @param  condition for the audio parameter to be set
     * @return none.
     */
    virtual const std::string GetAudioParameter(const std::string& networkId, const AudioParamKey key,
        const std::string& condition) = 0;

    /**
     * Set Extra Audio Parameters.
     *
     * @param key main key for the extra audio parameter to be set
     * @param kvpairs associated with the sub keys and values for the extra audio parameter to be set
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t SetExtraParameters(const std::string &key,
        const std::vector<std::pair<std::string, std::string>> &kvpairs) = 0;

    /**
     * Get Extra Audio Parameters.
     *
     * @param mainKey main key for the extra audio parameter to be get
     * @param subKeys associated with the key for the extra audio parameter to be get
     * @param result value of sub key parameters
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t GetExtraParameters(const std::string &mainKey,
        const std::vector<std::string> &subKyes, std::vector<std::pair<std::string, std::string>> &result) = 0;

    /**
     * Update the audio route after device is detected and route is decided
     *
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t UpdateActiveDeviceRoute(DeviceType type, DeviceFlag flag,
        BluetoothOffloadState a2dpOffloadFlag) = 0;

    /**
     * Set device dmDevice type.
     *
     * @return none.
     */
    virtual void SetDmDeviceType(uint16_t dmDeviceType) = 0;

    /**
     * Update the audio route after devices is detected and route is decided
     *
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t UpdateActiveDevicesRoute(std::vector<std::pair<DeviceType, DeviceFlag>> &activeDevices,
        BluetoothOffloadState a2dpOffloadFlag, const std::string &deviceName = "") = 0;

    /**
     * Update the audio dual tone state after devices is detected and route is decided
     *
     * @return Returns 0 if success. Otherwise returns Errocode defined in audio_errors.h.
     */
    virtual int32_t UpdateDualToneState(bool enable, int32_t sessionId) = 0;

    /**
     * Get the transaction Id
     *
     * @return Returns transaction id.
     */
    virtual uint64_t GetTransactionId(DeviceType deviceType, DeviceRole deviceRole)
    {
        (void)deviceType;
        (void)deviceRole;
        return 0;
    }

    /**
     * Notify device connect info
     *
     * @return Returns transaction id.
     */
    virtual void NotifyDeviceInfo(std::string networkId, bool connected) = 0;

    /**
     * Check remote device state.
     *
     * @return Returns transaction id.
     */
    virtual int32_t CheckRemoteDeviceState(std::string networkId, DeviceRole deviceRole, bool isStartDevice) = 0;

    /**
     * Set parameter callback
     *
     * @return Returns the setting result
     */
    virtual int32_t SetParameterCallback(const sptr<IRemoteObject>& object) = 0;

    /**
     * Set wakeupSource callback
     *
     * @return Returns the setting result
     */
    virtual int32_t SetWakeupSourceCallback(const sptr<IRemoteObject>& object) = 0;

    /**
     * Set audio mono state for accessibility
     *
     * @param  audioMono the state of mono audio for accessibility
     * @return none.
     */
    virtual void SetAudioMonoState(bool audioMono) = 0;

    /**
     * Set audio balance value for accessibility
     *
     * @param  audioBalance the value of audio balance for accessibility
     * @return none.
     */
    virtual void SetAudioBalanceValue(float audioBalance) = 0;

    /**
     * Create AudioProcess for play.
     *
     * @param config the config of the AudioProcess
     *
     * @return Returns AudioProcess client.
     */
    virtual sptr<IRemoteObject> CreateAudioProcess(const AudioProcessConfig &config, int32_t &errorCode,
        const AudioPlaybackCaptureConfig &filterConfig = AudioPlaybackCaptureConfig()) = 0;

    /**
     * Use effect manager information to load effect libraries.
     *
     * @return true/false.
     */
    virtual bool LoadAudioEffectLibraries(std::vector<Library> libraries, std::vector<Effect> effects,
        std::vector<Effect> &successEffects) = 0;

    /**
     * Create effect chain manager for audio effect processing.
     *
     * @return true/false.
     */
    virtual bool CreateEffectChainManager(std::vector<EffectChain> &effectChains,
        const EffectChainManagerParam &effectParam, const EffectChainManagerParam &enhanceParam) = 0;

    /**
     * Set output device sink for effect chain manager.
     *
     * @return none.
     */
    virtual void SetOutputDeviceSink(int32_t device, std::string &sinkName) = 0;

    /**
     * Set active output device
     *
     * @return none.
     */
    virtual void SetActiveOutputDevice(DeviceType deviceType) = 0;

    /**
     * Regiest policy provider.
     *
     * @return result code.
     */
    virtual int32_t RegiestPolicyProvider(const sptr<IRemoteObject> &object) = 0;

    /**
     * Regiest CoreService provider.
     *
     * @return result code.
     */
    virtual int32_t RegistCoreServiceProvider(const sptr<IRemoteObject> &object) = 0;

    /**
     * Create playback capturer manager.
     *
     * @return true/false.
     */
    virtual bool CreatePlaybackCapturerManager() = 0;

    /**
     * Update spatialization enabled state and head tracking enabled state.
     *
     * @param state identify the enabled state
     *
     * @return result of setting. 0 if success, error number else.
     */
    virtual int32_t UpdateSpatializationState(AudioSpatializationState spatializationState) = 0;

    /**
     * Update spatial device type.
     *
     * @param spatialDeviceType identify the spatial device type.
     *
     * @return result of setting. 0 if success, error number else.
    */
    virtual int32_t UpdateSpatialDeviceType(AudioSpatialDeviceType spatialDeviceType) = 0;

    /**
     * Notify Stream volume changed.
     *
     * @param streamType specified streamType whose volume to be notified
     * @param volume stream volume in float
     *
     * @return result of notify. 0 if success, error number else.
     */
    virtual int32_t NotifyStreamVolumeChanged(AudioStreamType streamType, float volume) = 0;

    /**
     * Set spatialization rendering scene type.
     *
     * @param spatializationSceneType identify the spatialization rendering scene type to be set.
     *
     * @return result of setting. 0 if success, error number else.
     */
    virtual int32_t SetSpatializationSceneType(AudioSpatializationSceneType spatializationSceneType) = 0;

    virtual int32_t ResetRouteForDisconnect(DeviceType type) = 0;

    /**
     * get the effect algorithmic latency value for a specified audio stream.
     *
     * @param sessionId the session ID value for the stream
     *
     * @return Returns the effect algorithmic latency in ms.
     */
    virtual uint32_t GetEffectLatency(const std::string &sessionId) = 0;

    /**
     * Get max amplitude for device.
     *
     * @param isOutputDevice specified if the device is output device
     * @param deviceClass specified deviceClass to get max amplitude
     * @param sourceType specified sourceType when capture
     *
     * @return result of max amplitude.
     */
    virtual float GetMaxAmplitude(bool isOutputDevice, std::string deviceClass, SourceType sourceType) = 0;

    /**
     * Release old endpoint and re-create one.
     */
    virtual void ResetAudioEndpoint() = 0;

    virtual void UpdateLatencyTimestamp(std::string &timestamp, bool isRenderer) = 0;

    // Check if the multi-channel sound effect is working on the DSP
    virtual bool GetEffectOffloadEnabled() = 0;
    // for effect V3
    virtual int32_t SetAudioEffectProperty(const AudioEffectPropertyArrayV3 &propertyArray,
        const DeviceType& deviceType = DEVICE_TYPE_NONE) = 0;
    virtual int32_t GetAudioEffectProperty(AudioEffectPropertyArrayV3 &propertyArray,
        const DeviceType& deviceType = DEVICE_TYPE_NONE) = 0;
    // for effect
    virtual int32_t SetAudioEffectProperty(const AudioEffectPropertyArray &propertyArray) = 0;
    virtual int32_t GetAudioEffectProperty(AudioEffectPropertyArray &propertyArray) = 0;
    // for enhance
    virtual int32_t SetAudioEnhanceProperty(const AudioEnhancePropertyArray &propertyArray,
        DeviceType deviceType = DEVICE_TYPE_NONE) = 0;
    virtual int32_t GetAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray,
        DeviceType deviceType = DEVICE_TYPE_NONE) = 0;

    /**
     * Load effect hdi model when audio_host online.
     */
    virtual void LoadHdiEffectModel() = 0;

    /**
     * Update Effect BtOffload Supported state.
     */
    virtual void UpdateEffectBtOffloadSupported(const bool &isSupported) = 0;

    /**
     * Set Sink Mute For Switch Device.
     */
    virtual int32_t SetSinkMuteForSwitchDevice(const std::string &devceClass, int32_t durationUs, bool mute) = 0;

    /**
     * Restore Session
     */
    virtual void RestoreSession(const uint32_t &sessionID, RestoreInfo restoreInfo) = 0;

    /**
     * Set Rotation To Effect.
     */
    virtual void SetRotationToEffect(const uint32_t rotate) = 0;

    /**
     * Update Session Connection State
     */
    virtual void UpdateSessionConnectionState(const int32_t &sessionID, const int32_t &state) = 0;

    /**
     * Set Non Interrupt Mute
     */
    virtual void SetNonInterruptMute(const uint32_t sessionId, const bool muteFlag) = 0;

    virtual int32_t SetOffloadMode(uint32_t sessionId, int32_t state, bool isAppBack) = 0;

    virtual int32_t UnsetOffloadMode(uint32_t sessionId) = 0;

    virtual void CheckHibernateState(bool onHibernate) = 0;

    /**
     * Create IpcOfflineStream for audio edition.
     *
     * @return Returns IpcOfflineStream client.
     */
    virtual sptr<IRemoteObject> CreateIpcOfflineStream(int32_t &errorCode) = 0;

    /**
     * Get all offline audio effect chain names for audio edition.
     *
     * @return Returns result of querying, 0 if success, error number else.
     */
    virtual int32_t GetOfflineAudioEffectChains(std::vector<std::string> &effectChains) = 0;

    /**
     * set foregroun list.
     *
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t SetForegroundList(std::vector<std::string> list) = 0;

    /**
     * check standby status.
     *
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t GetStandbyStatus(uint32_t sessionId, bool &isStandby, int64_t &enterStandbyTime) = 0;

    /**
     * generate sessionId.
     *
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t GenerateSessionId(uint32_t &sessionId) = 0;

    virtual void NotifyAccountsChanged() = 0;

    virtual void NotifySettingsDataReady() = 0;

    virtual void GetAllSinkInputs(std::vector<SinkInput> &sinkInputs) = 0;

    virtual void SetDefaultAdapterEnable(bool isEnable) = 0;

    virtual void NotifyAudioPolicyReady() = 0;

    virtual bool IsAcousticEchoCancelerSupported(SourceType sourceType) = 0;

    virtual int32_t ForceStopAudioStream(StopAudioType audioType) = 0;

    virtual bool SetKaraokeParameters(const std::string &parameters) = 0;

    virtual bool IsAudioLoopbackSupported(AudioLoopbackMode mode) = 0;

#ifdef HAS_FEATURE_INNERCAPTURER
    /**
     * set inner capture limit.
     * @param innerCapLimit inner capture limit num
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t SetInnerCapLimit(uint32_t innerCapLimit) = 0;
    /**
     * check inner capture limit
     * @param AudioPlaybackCaptureConfig inner capture filter info
     * @param innerCapId unique identifier of inner capture
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t CheckCaptureLimit(const AudioPlaybackCaptureConfig &config, int32_t &innerCapId) = 0;
    /**
     * release inner capture limit
     * @param innerCapId unique identifier of inner capture
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t ReleaseCaptureLimit(int32_t innerCapId) = 0;
#endif

    /**
     * Load adapter of hal.
     *
     * @param devMgrType specify which manager to load adapter, include local, bt, remote.
     * @param adapterName name of adapter to load.
     *
     * @return Returns result 0 if success, error number else.
     */
    virtual int32_t LoadHdiAdapter(uint32_t devMgrType, const std::string &adapterName) = 0;

    /**
     * Unload adapter of hal.
     *
     * @param devMgrType specify which manager to unload adapter, include local, bt, remote.
     * @param adapterName name of adapter to unload.
     * @param force need to force unload adapter.
     *
     * @return none.
     */
    virtual void UnloadHdiAdapter(uint32_t devMgrType, const std::string &adapterName, bool force) = 0;

    /**
     * Create render of hal.
     *
     * @param deviceClass specify render type.
     * @param idInfo info of render id.
     * @param attr attribute string of render.
     *
     * @return Returns render id if success, HDI_INVALID_ID else.
     */
    virtual uint32_t CreateHdiSinkPort(const std::string &deviceClass, const std::string &idInfo,
        const IAudioSinkAttr &attr) = 0;

    /**
     * Create render of hal.
     *
     * @param idBase specify render type.
     * @param idType specify sink type.
     * @param info extra info of render.
     * @param attr attribute string of render.
     *
     * @return Returns render id if success, HDI_INVALID_ID else.
     */
    virtual uint32_t CreateSinkPort(HdiIdBase idBase, HdiIdType idType, const std::string &idInfo,
        const IAudioSinkAttr &attr) = 0;

    /**
     * Create capture of hal.
     *
     * @param deviceClass specify capture type.
     * @param idInfo info of capture id.
     * @param attr attribute string of capture.
     *
     * @return Returns capture id if success, HDI_INVALID_ID else.
     */
    virtual uint32_t CreateHdiSourcePort(const std::string &deviceClass, const std::string &idInfo,
        const IAudioSourceAttr &attr) = 0;

    /**
     * Create capture of hal.
     *
     * @param idBase specify capture type.
     * @param idType specify sink type.
     * @param info extra info of capture.
     * @param attr attribute string of capture.
     *
     * @return Returns capture id if success, HDI_INVALID_ID else.
     */
    virtual uint32_t CreateSourcePort(HdiIdBase idBase, HdiIdType idType, const std::string &idInfo,
        const IAudioSourceAttr &attr) = 0;

    /**
     * Destroy render/capture of hal.
     *
     * @param id specify which render or capture to destroy.
     *
     * @return none.
     */
    virtual void DestroyHdiPort(uint32_t id) = 0;

    virtual void SetDeviceConnectedFlag(bool flag) = 0;

    virtual void SetSessionMuteState(const uint32_t sessionId, const bool insert, const bool muteFlag) = 0;

    virtual void SetLatestMuteState(const uint32_t sessionId, const bool muteFlag) = 0;
    /**
     * Create AudioWorkgroup.
     *
     * @return Returns workgroup id for current process.
     */
    virtual int32_t CreateAudioWorkgroup(int32_t pid) = 0;
    virtual int32_t ReleaseAudioWorkgroup(int32_t pid, int32_t workgroupId) = 0;
    virtual int32_t AddThreadToGroup(int32_t pid, int32_t workgroupId, int32_t tokenId) = 0;
    virtual int32_t RemoveThreadFromGroup(int32_t pid, int32_t workgroupId, int32_t tokenId) = 0;
    virtual int32_t StartGroup(int32_t pid, int32_t workgroupId, uint64_t startTime, uint64_t deadlineTime) = 0;
    virtual int32_t StopGroup(int32_t pid, int32_t workgroupId) = 0;
    /**
     * Register data transfer callback.
     *
     * @return result code.
     */
    virtual int32_t RegisterDataTransferCallback(const sptr<IRemoteObject> &object) = 0;
    virtual int32_t RegisterDataTransferMonitorParam(const int32_t &callbackId,
        const DataTransferMonitorParam &param) = 0;
    virtual int32_t UnregisterDataTransferMonitorParam(const int32_t &callbackId) = 0;

    /**
     * Set bluetooth hdi invalid state
     *
     * @return none.
     */
     virtual void SetBtHdiInvalidState() = 0;
public:
    DECLARE_INTERFACE_DESCRIPTOR(u"IStandardAudioService");
};

class DataTransferStateChangeCallbackInner {
public:
    virtual ~DataTransferStateChangeCallbackInner() = default;
    
    virtual void OnDataTransferStateChange(const int32_t &callbackId,
        const AudioRendererDataTransferStateChangeInfo &info) = 0;
};

class AudioManagerStub : public IRemoteStub<IStandardAudioService> {
public:
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option) override;

private:
    int HandleGetAudioParameter(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioParameter(MessageParcel &data, MessageParcel &reply);
    int HandleGetExtraAudioParameters(MessageParcel &data, MessageParcel &reply);
    int HandleSetExtraAudioParameters(MessageParcel &data, MessageParcel &reply);
    int HandleSetMicrophoneMute(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioScene(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateActiveDeviceRoute(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateActiveDevicesRoute(MessageParcel &data, MessageParcel &reply);
    int HandleSetDmDeviceType(MessageParcel &data, MessageParcel &reply);
    int HandleDualToneState(MessageParcel &data, MessageParcel &reply);
    int HandleGetTransactionId(MessageParcel &data, MessageParcel &reply);
    int HandleSetParameterCallback(MessageParcel &data, MessageParcel &reply);
    int HandleGetRemoteAudioParameter(MessageParcel &data, MessageParcel &reply);
    int HandleSetRemoteAudioParameter(MessageParcel &data, MessageParcel &reply);
    int HandleNotifyDeviceInfo(MessageParcel &data, MessageParcel &reply);
    int HandleCheckRemoteDeviceState(MessageParcel &data, MessageParcel &reply);
    int HandleSetVoiceVolume(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioMonoState(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioBalanceValue(MessageParcel &data, MessageParcel &reply);
    int HandleCreateAudioProcess(MessageParcel &data, MessageParcel &reply);
    int HandleLoadAudioEffectLibraries(MessageParcel &data, MessageParcel &reply);
    int HandleCreateAudioEffectChainManager(MessageParcel &data, MessageParcel &reply);
    int HandleSetOutputDeviceSink(MessageParcel &data, MessageParcel &reply);
    int HandleSetActiveOutputDevice(MessageParcel &data, MessageParcel &reply);
    int HandleCreatePlaybackCapturerManager(MessageParcel &data, MessageParcel &reply);
    int HandleRegiestPolicyProvider(MessageParcel &data, MessageParcel &reply);
    int HandleRegistCoreServiceProvider(MessageParcel &data, MessageParcel &reply);
    int HandleSetWakeupSourceCallback(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateSpatializationState(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateSpatialDeviceType(MessageParcel& data, MessageParcel& reply);
    int HandleOffloadSetVolume(MessageParcel &data, MessageParcel &reply);
    int HandleNotifyStreamVolumeChanged(MessageParcel &data, MessageParcel &reply);
    int HandleSetSpatializationSceneType(MessageParcel &data, MessageParcel &reply);
    int HandleGetMaxAmplitude(MessageParcel &data, MessageParcel &reply);
    int HandleResetAudioEndpoint(MessageParcel &data, MessageParcel &reply);
    int HandleResetRouteForDisconnect(MessageParcel &data, MessageParcel &reply);
    int HandleGetEffectLatency(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateLatencyTimestamp(MessageParcel &data, MessageParcel &reply);
    int HandleSetAsrAecMode(MessageParcel &data, MessageParcel &reply);
    int HandleGetAsrAecMode(MessageParcel &data, MessageParcel &reply);
    int HandleSetAsrNoiseSuppressionMode(MessageParcel &data, MessageParcel &reply);
    int HandleSetOffloadMode(MessageParcel &data, MessageParcel &reply);
    int HandleUnsetOffloadMode(MessageParcel &data, MessageParcel &reply);
    int HandleCheckHibernateState(MessageParcel &data, MessageParcel &reply);
    int HandleGetAsrNoiseSuppressionMode(MessageParcel &data, MessageParcel &reply);
    int HandleSetAsrWhisperDetectionMode(MessageParcel &data, MessageParcel &reply);
    int HandleGetAsrWhisperDetectionMode(MessageParcel &data, MessageParcel &reply);
    int HandleSetAsrVoiceControlMode(MessageParcel &data, MessageParcel &reply);
    int HandleSetAsrVoiceMuteMode(MessageParcel &data, MessageParcel &reply);
    int HandleIsWhispering(MessageParcel &data, MessageParcel &reply);
    int HandleGetEffectOffloadEnabled(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioEffectPropertyV3(MessageParcel &data, MessageParcel &reply);
    int HandleGetAudioEffectPropertyV3(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioEffectProperty(MessageParcel &data, MessageParcel &reply);
    int HandleGetAudioEffectProperty(MessageParcel &data, MessageParcel &reply);
    int HandleSetAudioEnhanceProperty(MessageParcel &data, MessageParcel &reply);
    int HandleGetAudioEnhanceProperty(MessageParcel &data, MessageParcel &reply);
    int HandleSuspendRenderSink(MessageParcel &data, MessageParcel &reply);
    int HandleRestoreRenderSink(MessageParcel &data, MessageParcel &reply);
    int HandleLoadHdiEffectModel(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateEffectBtOffloadSupported(MessageParcel &data, MessageParcel &reply);
    int HandleSetSinkMuteForSwitchDevice(MessageParcel &data, MessageParcel &reply);
    int HandleSetRotationToEffect(MessageParcel &data, MessageParcel &reply);
    int HandleUpdateSessionConnectionState(MessageParcel &data, MessageParcel &reply);
    int HandleSetNonInterruptMute(MessageParcel &data, MessageParcel &reply);
    int HandleRestoreSession(MessageParcel &data, MessageParcel &reply);
    int HandleCreateIpcOfflineStream(MessageParcel &data, MessageParcel &reply);
    int HandleGetOfflineAudioEffectChains(MessageParcel &data, MessageParcel &reply);
    int HandleGetStandbyStatus(MessageParcel &data, MessageParcel &reply);
    int HandleGenerateSessionId(MessageParcel &data, MessageParcel &reply);
    int HandleNotifyAccountsChanged(MessageParcel &data, MessageParcel &reply);
    int HandleNotifySettingsDataReady(MessageParcel &data, MessageParcel &reply);
    int HandleGetAllSinkInputs(MessageParcel &data, MessageParcel &reply);
    int HandleSetDefaultAdapterEnable(MessageParcel &data, MessageParcel &reply);
    int HandleNotifyAudioPolicyReady(MessageParcel &data, MessageParcel &reply);
#ifdef HAS_FEATURE_INNERCAPTURER
    int HandleSetInnerCapLimit(MessageParcel &data, MessageParcel &reply);
    int HandleCheckCaptureLimit(MessageParcel &data, MessageParcel &reply);
    int HandleReleaseCaptureLimit(MessageParcel &data, MessageParcel &reply);
#endif
    int HandleLoadHdiAdapter(MessageParcel &data, MessageParcel &reply);
    int HandleUnloadHdiAdapter(MessageParcel &data, MessageParcel &reply);
    int HandleCreateHdiSinkPort(MessageParcel &data, MessageParcel &reply);
    int HandleCreateSinkPort(MessageParcel &data, MessageParcel &reply);
    int HandleCreateHdiSourcePort(MessageParcel &data, MessageParcel &reply);
    int HandleCreateSourcePort(MessageParcel &data, MessageParcel &reply);
    int HandleDestroyHdiPort(MessageParcel &data, MessageParcel &reply);
    int HandleDeviceConnectedFlag(MessageParcel &data, MessageParcel &reply);
    int HandleIsAcousticEchoCancelerSupported(MessageParcel &data, MessageParcel &reply);
    int HandleSetKaraokeParameters(MessageParcel &data, MessageParcel &reply);
    int HandleIsAudioLoopbackSupported(MessageParcel &data, MessageParcel &reply);
    int HandleSetSessionMuteState(MessageParcel &data, MessageParcel &reply);
    int HandleOnMuteStateChange(MessageParcel &data, MessageParcel &reply);
    int HandleForceStopAudioStream(MessageParcel &data, MessageParcel &reply);
    int HandleRegisterDataTransferCallback(MessageParcel &data, MessageParcel &reply);
    int HandleRegisterDataTransferMonitorParam(MessageParcel &data, MessageParcel &reply);
    int HandleUnregisterDataTransferMonitorParam(MessageParcel &data, MessageParcel &reply);
    int HandleSetBtHdiInvalidState(MessageParcel &data, MessageParcel &reply);

    int HandleSecondPartCode(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int HandleThirdPartCode(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int HandleFourthPartCode(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int HandleFifthPartCode(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int HandleSixthPartCode(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option);
    int HandleCreateAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
    int HandleReleaseAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
    int HandleAddThreadToAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
    int HandleRemoveThreadFromAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
    int HandleStartAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
    int HandleStopAudioWorkgroup(MessageParcel &data, MessageParcel &reply);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // I_ST_AUDIO_MANAGER_BASE_H
