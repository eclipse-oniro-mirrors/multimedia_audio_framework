/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ST_AUDIO_INTERRUPT_SERVICE_H
#define ST_AUDIO_INTERRUPT_SERVICE_H

#include <mutex>
#include <list>
#include <functional>
#include <unordered_map>

#include "iremote_object.h"

#include "i_audio_interrupt_event_dispatcher.h"
#include "audio_interrupt_info.h"
#include "audio_policy_server_handler.h"
#include "audio_policy_server.h"
#include "audio_session_service.h"
#include "client_type_manager.h"
#include "audio_interrupt_dfx_collector.h"
#include "audio_zone_info.h"
#include "audio_interrupt_zone.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {

class AudioPolicyServerHandler;

class SessionTimeOutCallback;

class AudioInterruptService : public std::enable_shared_from_this<AudioInterruptService>,
                              public IAudioInterruptEventDispatcher,
                              public SessionTimeOutCallback {
public:
    AudioInterruptService();
    virtual ~AudioInterruptService();

    const sptr<IStandardAudioService> GetAudioServerProxy();

    // callback run in handler thread
    void DispatchInterruptEventWithStreamId(
        uint32_t streamId, InterruptEventInternal &interruptEvent) override;

    void Init(sptr<AudioPolicyServer> server);
    void AddDumpInfo(std::unordered_map<int32_t, std::shared_ptr<AudioInterruptZone>> &audioInterruptZonesMapDump);
    void SetCallbackHandler(std::shared_ptr<AudioPolicyServerHandler> handler);

    // interfaces of SessionTimeOutCallback
    void OnSessionTimeout(const int32_t pid) override;

    // interfaces for AudioSessionService
    int32_t ActivateAudioSession(const int32_t callerPid, const AudioSessionStrategy &strategy);
    int32_t DeactivateAudioSession(const int32_t callerPid);
    bool IsAudioSessionActivated(const int32_t callerPid);

    // deprecated interrupt interfaces
    int32_t SetAudioManagerInterruptCallback(const sptr<IRemoteObject> &object);
    int32_t UnsetAudioManagerInterruptCallback();
    int32_t RequestAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt);
    int32_t AbandonAudioFocus(const int32_t clientId, const AudioInterrupt &audioInterrupt);

    // modern interrupt interfaces
    int32_t SetAudioInterruptCallback(const int32_t zoneId, const uint32_t streamId,
        const sptr<IRemoteObject> &object, uint32_t uid);
    int32_t UnsetAudioInterruptCallback(const int32_t zoneId, const uint32_t streamId);
    bool AudioInterruptIsActiveInFocusList(const int32_t zoneId, const uint32_t incomingStreamId);
    int32_t ActivateAudioInterrupt(
        const int32_t zoneId, const AudioInterrupt &audioInterrupt, const bool isUpdatedAudioStrategy = false);
    int32_t DeactivateAudioInterrupt(const int32_t zoneId, const AudioInterrupt &audioInterrupt);
    bool IsCapturerFocusAvailable(int32_t zoneId, const AudioCapturerInfo &capturerInfo);

    // preempt audio focus interfaces
    int32_t ActivatePreemptMode();
    int32_t DeactivatePreemptMode();

    // zone debug interfaces
    int32_t FindZoneByPid(int32_t pid);
    int32_t CreateAudioInterruptZone(const int32_t zoneId, AudioZoneFocusStrategy focusStrategy =
        AudioZoneFocusStrategy::LOCAL_FOCUS_STRATEGY);
    int32_t ReleaseAudioInterruptZone(const int32_t zoneId, GetZoneIdFunc func);
    int32_t MigrateAudioInterruptZone(const int32_t zoneId, GetZoneIdFunc func);
    int32_t InjectInterruptToAudioZone(const int32_t zoneId, const AudioFocusList &interrupts);
    int32_t InjectInterruptToAudioZone(const int32_t zoneId, const std::string &deviceTag,
        const AudioFocusList &interrupts);
    int32_t GetAudioFocusInfoList(const int32_t zoneId, AudioFocusList &focusInfoList);
    int32_t GetAudioFocusInfoList(const int32_t zoneId, const std::string &deviceTag,
        AudioFocusList &focusInfoList);

    int32_t SetAudioFocusInfoCallback(const int32_t zoneId, const sptr<IRemoteObject> &object);
    int32_t GetStreamTypePriority(AudioStreamType streamType);
    unordered_map<AudioStreamType, int> GetStreamPriorityMap() const;
    AudioStreamType GetStreamInFocus(const int32_t zoneId);
    AudioStreamType GetStreamInFocusByUid(const int32_t uid, const int32_t zoneId);
    int32_t GetSessionInfoInFocus(AudioInterrupt &audioInterrupt, const int32_t zoneId);
    void ClearAudioFocusInfoListOnAccountsChanged(const int &id);
    int32_t ClearAudioFocusInfoList();
    void AudioInterruptZoneDump(std::string &dumpString);
    void AudioSessionInfoDump(std::string &dumpString);
    AudioScene GetHighestPriorityAudioScene(const int32_t zoneId) const;

    void ProcessRemoteInterrupt(std::set<int32_t> streamIds, InterruptEventInternal interruptEvent);
    int32_t SetQueryBundleNameListCallback(const sptr<IRemoteObject> &object);
    void RegisterDefaultVolumeTypeListener();

private:
    static constexpr int32_t ZONEID_DEFAULT = 0;
    static constexpr float DUCK_FACTOR = 0.2f;
    static constexpr int32_t DEFAULT_APP_PID = -1;
    static constexpr int64_t OFFLOAD_NO_SESSION_ID = -1;
    static constexpr int32_t STREAM_DEFAULT_PRIORITY = 100;

    using InterruptIterator = std::list<std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator>;

    // Inner class for death handler
    class AudioInterruptDeathRecipient : public IRemoteObject::DeathRecipient {
    public:
        explicit AudioInterruptDeathRecipient(
            const std::shared_ptr<AudioInterruptService> &service,
            uint32_t streamId);
        virtual ~AudioInterruptDeathRecipient() = default;

        DISALLOW_COPY_AND_MOVE(AudioInterruptDeathRecipient);

        void OnRemoteDied(const wptr<IRemoteObject> &remote);

    private:
        const std::weak_ptr<AudioInterruptService> service_;
        const uint32_t streamId_;
    };

    // Inner class for callback
    class AudioInterruptClient {
    public:
        explicit AudioInterruptClient(
            const std::shared_ptr<AudioInterruptCallback> &callback,
            const sptr<IRemoteObject> &object,
            const sptr<AudioInterruptDeathRecipient> &deathRecipient);
        virtual ~AudioInterruptClient();

        DISALLOW_COPY_AND_MOVE(AudioInterruptClient);

        void OnInterrupt(const InterruptEventInternal &interruptEvent);

        void SetCallingUid(uint32_t uid);
        uint32_t GetCallingUid();

    private:
        const std::shared_ptr<AudioInterruptCallback> callback_;
        const sptr<IRemoteObject> object_;
        sptr<AudioInterruptDeathRecipient> deathRecipient_;
        uint32_t callingUid_ = 0;
    };

    // deprecated interrupt interfaces
    void NotifyFocusGranted(const int32_t clientId, const AudioInterrupt &audioInterrupt);
    int32_t NotifyFocusAbandoned(const int32_t clientId, const AudioInterrupt &audioInterrupt);
    int32_t AbandonAudioFocusInternal(const int32_t clientId, const AudioInterrupt &audioInterrupt);

    // modern interrupt interfaces
    int32_t ActivateAudioInterruptInternal(const int32_t zoneId, const AudioInterrupt &audioInterrupt,
        const bool isUpdatedAudioStrategy, bool &updateScene);
    void ProcessAudioScene(const AudioInterrupt &audioInterrupt, const uint32_t &incomingStreamId,
        const int32_t &zoneId, bool &shouldReturnSuccess);
    bool IsAudioSourceConcurrency(const SourceType &existSourceType, const SourceType &incomingSourceType,
        const std::vector<SourceType> &existConcurrentSources,
        const std::vector<SourceType> &incomingConcurrentSources);
    bool IsMediaStream(AudioStreamType audioStreamType);
    std::string GetRealBundleName(uint32_t uid);
    void UpdateAudioFocusStrategy(const AudioInterrupt &currentInterrupt, const AudioInterrupt &incomingInterrupt,
        AudioFocusEntry &focusEntry);
    void UpdateMuteAudioFocusStrategy(const AudioInterrupt &currentInterrupt, const AudioInterrupt &incomingInterrupt,
        AudioFocusEntry &focusEntry);
    bool FocusEntryContinue(std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterActive,
        AudioFocusEntry &focusEntry, const AudioInterrupt &incomingInterrupt);
    int32_t ProcessFocusEntry(const int32_t zoneId, const AudioInterrupt &incomingInterrupt);
    void SendInterruptEventToIncomingStream(InterruptEventInternal &interruptEvent,
        const AudioInterrupt &incomingInterrupt);
    void AddToAudioFocusInfoList(std::shared_ptr<AudioInterruptZone> &audioInterruptZone,
        const int32_t &zoneId, const AudioInterrupt &incomingInterrupt, const AudioFocuState &incomingState);
    void HandleIncomingState(const int32_t &zoneId, const AudioFocuState &incomingState,
        InterruptEventInternal &interruptEvent, const AudioInterrupt &incomingInterrupt);
    void ProcessExistInterrupt(std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator
        &iterActive, AudioFocusEntry &focusEntry, const AudioInterrupt &incomingInterrupt,
        bool &removeFocusInfo, InterruptEventInternal &interruptEvent);
    void ProcessActiveInterrupt(const int32_t zoneId, const AudioInterrupt &incomingInterrupt);
    std::list<std::pair<AudioInterrupt, AudioFocuState>> GetAudioFocusInfoList(const int32_t zoneId);
    void ResumeAudioFocusList(const int32_t zoneId, bool isSessionTimeout = false);
    bool EvaluateWhetherContinue(const AudioInterrupt &incoming, const AudioInterrupt
        &inprocessing, AudioFocusEntry &focusEntry, bool bConcurrency);
    std::list<std::pair<AudioInterrupt, AudioFocuState>> SimulateFocusEntry(const int32_t zoneId);
    void SendActiveInterruptEvent(const uint32_t activeStreamId, const InterruptEventInternal &interruptEvent,
        const AudioInterrupt &incomingInterrupt, const AudioInterrupt &activeInterrupt);
    void DeactivateAudioInterruptInternal(const int32_t zoneId, const AudioInterrupt &audioInterrupt,
        bool isSessionTimeout = false);
    void SendInterruptEvent(AudioFocuState oldState, AudioFocuState newState,
        std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterActive, bool &removeFocusInfo);
    void SendInterruptEventCallback(const InterruptEventInternal &interruptEvent,
        const uint32_t &streamId, const AudioInterrupt &audioInterrupt);
    bool IsSameAppInShareMode(const AudioInterrupt incomingInterrupt, const AudioInterrupt activeInterrupt);
    void UpdateAudioSceneFromInterrupt(const AudioScene audioScene, AudioInterruptChangeType changeType,
        int32_t zoneId = ZONEID_DEFAULT);
    void SendFocusChangeEvent(const int32_t zoneId, int32_t callbackCategory, const AudioInterrupt &audioInterrupt);
    void SendActiveVolumeTypeChangeEvent(const int32_t zoneId);
    void RemoveClient(const int32_t zoneId, uint32_t streamId);
    void RemoveFocusInfo(std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterActive,
        std::list<std::pair<AudioInterrupt, AudioFocuState>> &tmpFocusInfoList,
        std::shared_ptr<AudioInterruptZone> &zoneInfo,
        std::list<int32_t> &removeFocusInfoPidList);
    void PrintLogsOfFocusStrategyBaseMusic(const AudioInterrupt &audioInterrupt);

    // zone debug interfaces
    void WriteFocusMigrateEvent(const int32_t &toZoneId);
    void WriteServiceStartupError();

    // interfaces about audio session.
    void AddActiveInterruptToSession(const int32_t callerPid);
    void RemovePlaceholderInterruptForSession(const int32_t callerPid, bool isSessionTimeout = false);
    bool CanMixForSession(const AudioInterrupt &incomingInterrupt, const AudioInterrupt &activeInterrupt,
        const AudioFocusEntry &focusEntry);
    bool CanMixForIncomingSession(const AudioInterrupt &incomingInterrupt, const AudioInterrupt &activeInterrupt,
        const AudioFocusEntry &focusEntry);
    bool CanMixForActiveSession(const AudioInterrupt &incomingInterrupt, const AudioInterrupt &activeInterrupt,
        const AudioFocusEntry &focusEntry);
    bool IsIncomingStreamLowPriority(const AudioFocusEntry &focusEntry);
    bool IsActiveStreamLowPriority(const AudioFocusEntry &focusEntry);
    void UpdateHintTypeForExistingSession(const AudioInterrupt &incomingInterrupt, AudioFocusEntry &focusEntry);
    void HandleSessionTimeOutEvent(const int32_t pid);
    bool HandleLowPriorityEvent(const int32_t pid, const uint32_t streamId);
    void SendSessionTimeOutStopEvent(const int32_t zoneId, const AudioInterrupt &audioInterrupt,
        const std::list<std::pair<AudioInterrupt, AudioFocuState>> &audioFocusInfoList);
    bool ShouldCallbackToClient(uint32_t uid, int32_t streamId, InterruptEventInternal &interruptEvent);

    AudioFocuState GetNewIncomingState(InterruptHint hintType, AudioFocuState oldState);
    void RemoveAllPlaceholderInterrupt(std::list<int32_t> &removeFocusInfoPidList);
    bool IsLowestPriorityRecording(const AudioInterrupt &audioInterrupt);
    bool IsRecordingInterruption(const AudioInterrupt &audioInterrupt);
    void SetSessionMuteState(uint32_t sessionId, bool insert, bool muteFlag);
    void SetLatestMuteState(const InterruptEventInternal &interruptEvent, const uint32_t &streamId);
    void CheckIncommingFoucsValidity(AudioFocusEntry &focusEntry, const AudioInterrupt &incomingInterrupt,
        std::vector<SourceType> incomingConcurrentSources);
    bool IsCanMixInterrupt(const AudioInterrupt &incomingInterrupt,
        const AudioInterrupt &activeInterrupt);
    bool HadVoipStatus(const AudioInterrupt &audioInterrupt, const std::list<std::pair<AudioInterrupt, AudioFocuState>>
        &audioFocusInfoList);

    AudioStreamType GetStreamInFocusInternal(const int32_t uid, const int32_t zoneId);
    
    bool CheckAudioSessionExistence(const AudioInterrupt &incomingInterrupt, AudioFocusEntry &focusEntry);

    void SwitchHintType(std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterActive,
        InterruptEventInternal &interruptEvent, std::list<std::pair<AudioInterrupt, AudioFocuState>> &tmpFocusInfoList);

    bool IsHandleIter(std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterActive,
        AudioFocuState oldState, std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &iterNew);
    uint8_t GetAppState(int32_t appPid);
    void WriteStartDfxMsg(InterruptDfxBuilder &dfxBuilder, const AudioInterrupt &audioInterrupt);
    void WriteStopDfxMsg(const AudioInterrupt &audioInterrupt);
    void WriteSessionTimeoutDfxEvent(const int32_t pid);

    bool AudioFocusInfoListRemovalCondition(const AudioInterrupt &audioInterrupt,
        const std::pair<AudioInterrupt, AudioFocuState> &audioFocus);
    AudioScene RefreshAudioSceneFromAudioInterrupt(const AudioInterrupt &audioInterrupt,
        AudioScene &highestPriorityAudioScene);

    void HandleAppStreamType(AudioInterrupt &audioInterrupt);
    bool IsGameAvoidCallbackCase(const AudioInterrupt &audioInterrupt);
    void ResetNonInterruptControl(AudioInterrupt audioInterrupt);
    ClientType GetClientTypeByStreamId(int32_t streamId);

    int32_t ProcessActiveStreamFocus(std::list<std::pair<AudioInterrupt, AudioFocuState>> &audioFocusInfoList,
        const AudioInterrupt &incomingInterrupt, AudioFocuState &incomingState,
        std::list<std::pair<AudioInterrupt, AudioFocuState>>::iterator &activeInterrupt);
    void ReportRecordGetFocusFail(const AudioInterrupt &incomingInterrupt,
        const AudioInterrupt &activeInterrupt, int32_t reason);

    // interrupt members
    sptr<AudioPolicyServer> policyServer_;
    std::shared_ptr<AudioPolicyServerHandler> handler_;
    std::shared_ptr<AudioSessionService> sessionService_;
    friend class AudioInterruptZoneManager;
    AudioInterruptZoneManager zoneManager_;

    std::map<std::pair<AudioFocusType, AudioFocusType>, AudioFocusEntry> focusCfgMap_ = {};
    std::unordered_map<int32_t, std::shared_ptr<AudioInterruptZone>> zonesMap_;

    std::map<int32_t, std::shared_ptr<AudioInterruptClient>> interruptClients_;

    // deprecated interrupt members
    std::unique_ptr<AudioInterrupt> focussedAudioInterruptInfo_;
    int32_t clientOnFocus_ = 0;

    // preempt audio focus mode flag
    bool isPreemptMode_ = false;

    std::mutex mutex_;
    mutable int32_t ownerPid_ = 0;
    mutable int32_t ownerUid_ = 0;
    std::unique_ptr<AudioInterruptDfxCollector> dfxCollector_;
    sptr<IStandardAudioPolicyManagerListener> queryBundleNameListCallback_ = nullptr;
    AudioStreamType activeStreamType_ = STREAM_MUSIC;

    // settingsdata members
    AudioStreamType defaultVolumeType_ = STREAM_MUSIC;

    std::mutex audioServerProxyMutex_;
};
} // namespace AudioStandard
} // namespace OHOS

#endif // ST_AUDIO_INTERRUPT_SERVICE_H
