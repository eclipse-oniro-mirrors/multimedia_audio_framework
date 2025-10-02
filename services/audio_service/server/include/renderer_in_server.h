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

#ifndef RENDERER_IN_SERVER_H
#define RENDERER_IN_SERVER_H

#include <mutex>
#include "i_renderer_stream.h"
#include "i_stream_listener.h"
#include "oh_audio_buffer.h"
#include "i_stream_manager.h"
#include "audio_effect.h"
#include "audio_ring_cache.h"
#include "audio_stream_monitor.h"
#include "audio_stream_checker.h"
#include "player_dfx_writer.h"

namespace OHOS {
namespace AudioStandard {

struct RendererLatestInfoForWorkgroup {
    IStatus status;
    bool isInSilentState;
    bool silentModeAndMixWithOthers;
    bool lastWriteStandbyEnableStatus;
    float streamVolume;
    float systemVolume;
};

class StreamCallbacks : public IStatusCallback, public IWriteCallback {
public:
    explicit StreamCallbacks(uint32_t streamIndex);
    virtual ~StreamCallbacks();
    void OnStatusUpdate(IOperation operation) override;
    int32_t OnWriteData(size_t length) override;
    int32_t OnWriteData(int8_t *inputData, size_t requestDataLen) override;
    int32_t GetAvailableSize(size_t &length) override;
    std::unique_ptr<AudioRingCache>& GetDupRingBuffer();
private:
    uint32_t streamIndex_ = 0;
    int32_t recoveryAntiShakeBufferCount_ = 0;
    FILE *dumpDupOut_ = nullptr;
    std::string dumpDupOutFileName_ = "";
    std::unique_ptr<AudioRingCache> dupRingBuffer_ = nullptr;
};

class RendererInServer : public IStatusCallback, public IWriteCallback,
    public std::enable_shared_from_this<RendererInServer> {
public:
    RendererInServer(AudioProcessConfig processConfig, std::weak_ptr<IStreamListener> streamListener);
    virtual ~RendererInServer();
    void OnStatusUpdate(IOperation operation) override;
    void OnStatusUpdateExt(IOperation operation, std::shared_ptr<IStreamListener> stateListener);
    void HandleOperationFlushed();
    void HandleOperationStarted();
    int32_t OnWriteData(size_t length) override;
    int32_t OnWriteData(int8_t *inputData, size_t requestDataLen) override;
    int32_t GetAvailableSize(size_t &length) override;

    int32_t ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer);
    int32_t GetSessionId(uint32_t &sessionId);
    int32_t Start();
    int32_t Pause();
    int32_t Flush();
    int32_t Drain(bool stopFlag = false);
    int32_t Stop();
    int32_t Release(bool isSwitchStream = false);

    int32_t GetAudioTime(uint64_t &framePos, uint64_t &timestamp);
    int32_t GetAudioPosition(uint64_t &framePos, uint64_t &timestamp, uint64_t &latency, int32_t base);
    int32_t GetSpeedPosition(uint64_t &framePos, uint64_t &timestamp, uint64_t &latency, int32_t base);
    int32_t GetLatency(uint64_t &latency);
    int32_t SetRate(int32_t rate);
    int32_t SetLowPowerVolume(float volume);
    int32_t GetLowPowerVolume(float &volume);
    int32_t SetAudioEffectMode(int32_t effectMode);
    int32_t GetAudioEffectMode(int32_t &effectMode);
    int32_t SetPrivacyType(int32_t privacyType);
    int32_t GetPrivacyType(int32_t &privacyType);

    int32_t SetOffloadMode(int32_t state, bool isAppBack);
    int32_t UnsetOffloadMode();
    int32_t SetSpeed(float speed);
    int32_t SetOffloadDataCallbackState(int32_t state);
    int32_t GetOffloadApproximatelyCacheTime(uint64_t &timestamp, uint64_t &paWriteIndex,
        uint64_t &cacheTimeDsp, uint64_t &cacheTimePa);
    int32_t UpdateSpatializationState(bool spatializationEnabled, bool headTrackingEnabled);
    void CheckAndWriterRenderStreamStandbySysEvent(bool standbyEnable);

    int32_t GetStandbyStatus(bool &isStandby, int64_t &enterStandbyTime);

    int32_t Init();
    int32_t ConfigServerBuffer();
    int32_t InitBufferStatus();
    int32_t UpdateWriteIndex();
    BufferDesc DequeueBuffer(size_t length);
    void VolumeHandle(BufferDesc &desc);
    int32_t WriteData();
    void WriteEmptyData();
    int32_t DrainAudioBuffer();

    // for inner-cap
    int32_t EnableInnerCap(int32_t innerCapId);
    int32_t DisableInnerCap(int32_t innerCapId);
    int32_t InitDupStream(int32_t innerCapId);
    int32_t DisableAllInnerCap();
    std::unique_ptr<AudioRingCache>& GetDupRingBuffer();

    // for dual tone
    int32_t EnableDualTone();
    int32_t DisableDualTone();
    int32_t InitDualToneStream();

    void GetEAC3ControlParam();
    int32_t GetStreamManagerType() const noexcept;
    int32_t SetSilentModeAndMixWithOthers(bool on);
    int32_t SetClientVolume();
    int32_t SetLoudnessGain(float loudnessGain);
    int32_t SetMute(bool isMute);
    int32_t SetDuckFactor(float duckFactor);
    int32_t SetDefaultOutputDevice(const DeviceType defaultOutputDevice, bool skipForce = false);
    int32_t SetSourceDuration(int64_t duration);

    void OnDataLinkConnectionUpdate(IOperation operation);
    int32_t GetActualStreamManagerType() const noexcept;
    
    bool Dump(std::string &dumpString);
    void SetNonInterruptMute(const bool muteFlag);
    RestoreStatus RestoreSession(RestoreInfo restoreInfo);
    int32_t StopSession();
    void dualToneStreamInStart();
    bool CollectInfosForWorkgroup(float systemVolume);

    int32_t ResolveBufferBaseAndGetServerSpanSize(std::shared_ptr<OHAudioBufferBase> &buffer,
        uint32_t &spanSizeInFrame, uint64_t &engineTotalSizeInFrame);

    int32_t SetAudioHapticsSyncId(const int32_t &audioHapticsSyncId);
    void InitDupBuffer(int32_t innerCapId);

public:
    const AudioProcessConfig processConfig_;
private:
    void OnStatusUpdateSub(IOperation operation);
    bool IsHighResolution() const noexcept;
    void WriteMuteDataSysEvent(BufferDesc &bufferDesc);
    bool IsInvalidBuffer(uint8_t *buffer, size_t bufferSize);
    void ReportDataToResSched(std::unordered_map<std::string, std::string> payload, uint32_t type);
    void OtherStreamEnqueue(const BufferDesc &bufferDesc);
    void DoFadingOut(RingBufferWrapper& bufferDesc);
    int32_t SetStreamVolumeInfoForEnhanceChain();
    void StandByCheck();
    bool ShouldEnableStandBy();
    int32_t OffloadSetVolumeInner();
    void InnerCaptureOtherStream(const BufferDesc &bufferDesc, CaptureInfo &captureInfo, int32_t innerCapId);
    void InnerCaptureEnqueueBuffer(const BufferDesc &bufferDesc, CaptureInfo &captureInfo, int32_t innerCapId);
    int32_t StartInner();
    int32_t StopInner();
    int64_t GetLastAudioDuration();
    int32_t CreateDupBufferInner(int32_t innerCapId);
    int32_t WriteDupBufferInner(const BufferDesc &bufferDesc, int32_t innerCapId);
    void ReConfigDupStreamCallback();
    void HandleOperationStopped(RendererStage stage);
    int32_t StartInnerDuringStandby();
    void RecordStandbyTime(bool isStandby, bool isStart);
    int32_t FlushOhAudioBuffer();
    BufferDesc PrepareOutputBuffer(const RingBufferWrapper& ringBufferDesc);
    void CopyDataToInputBuffer(int8_t* inputData, size_t requestDataLen,
        const RingBufferWrapper& ringBufferDesc);
    void ProcessFadeOutIfNeeded(RingBufferWrapper& ringBufferDesc, uint64_t currentReadFrame,
        uint64_t currentWriteFrame, size_t requestDataInFrame);
    void UpdateLatestForWorkgroup(float systemVolume);
    int32_t DisableInnerCapHandle(int32_t innerCapId);
    int32_t InitDupStreamVolume(uint32_t dupStreamIndex);
    void ProcessManagerType();
private:
    std::mutex statusLock_;
    std::condition_variable statusCv_;
    std::shared_ptr<IRendererStream> stream_ = nullptr;
    uint32_t streamIndex_ = -1;
    std::string traceTag_;
    mutable int64_t volumeDataCount_ = 0;
    std::atomic<IStatus> status_ = I_STATUS_IDLE;
    bool offloadEnable_ = false;
    std::atomic<bool> standByEnable_ = false;
    std::atomic<bool> muteFlag_ = false;

    // for inner-cap
    std::mutex dupMutex_;
    size_t dupTotalSizeInFrame_ = 0;
    size_t dupSpanSizeInFrame_ = 0;
    size_t dupSpanSizeInByte_ = 0;
    size_t dupByteSizePerFrame_ = 0;
    FILE *dumpDupIn_ = nullptr;
    std::string dumpDupInFileName_ = "";
    std::map<int32_t, std::shared_ptr<StreamCallbacks>> innerCapIdToDupStreamCallbackMap_;
    std::unordered_map<int32_t, CaptureInfo> captureInfos_;
    std::unique_ptr<AudioRingCache> dupRingBuffer_ = nullptr;

    // for dual sink tone
    std::mutex dualToneMutex_;
    std::atomic<bool> isDualToneEnabled_ = false;
    uint32_t dualToneStreamIndex_ = 0;
    std::shared_ptr<IRendererStream> dualToneStream_ = nullptr;

    std::weak_ptr<IStreamListener> streamListener_;
    size_t engineTotalSizeInFrame_ = 0;
    size_t bufferTotalSizeInFrame_ = 0;
    size_t spanSizeInFrame_ = 0;
    size_t spanSizeInByte_ = 0;
    size_t byteSizePerFrame_ = 0;
    bool isBufferConfiged_  = false;
    std::atomic<bool> isInited_ = false;
    std::shared_ptr<OHAudioBufferBase> audioServerBuffer_ = nullptr;
    std::atomic<size_t> needForceWrite_ = 0;
    bool afterDrain = false;
    float lowPowerVolume_ = 1.0f;
    std::atomic<bool> isMuted_ = false;
    bool isNeedFade_ = false;
    float oldAppliedVolume_ = MAX_FLOAT_VOLUME;
    std::mutex updateIndexLock_;
    int64_t startedTime_ = 0;
    int64_t pausedTime_ = 0;
    int64_t stopedTime_ = 0;
    int64_t flushedTime_ = 0;
    int64_t drainedTime_ = 0;
    uint32_t underrunCount_ = 0;
    std::atomic<uint32_t> standByCounter_ = 0;
    int64_t enterStandbyTime_ = 0;
    int64_t lastWriteTime_ = 0;
    bool resetTime_ = false;
    uint64_t resetTimestamp_ = 0;
    std::mutex writeLock_;
    FILE *dumpC2S_ = nullptr; // client to server dump file
    std::string dumpFileName_ = "";
    ManagerType managerType_;
    std::mutex fadeoutLock_;
    int32_t fadeoutFlag_ = 0;
    std::time_t startMuteTime_ = 0;
    bool isInSilentState_ = false;
    std::atomic<bool> silentModeAndMixWithOthers_ = false;
    int32_t effectModeWhenDual_ = EFFECT_DEFAULT;
    int32_t renderEmptyCountForInnerCap_ = 0;

    // only read & write in CheckAndWriterRenderStreamStandbySysEvent
    bool lastWriteStandbyEnableStatus_ = false;
    std::set<int32_t> innerCapIds;

    int64_t lastStartTime_{};
    int64_t lastStopTime_{};
    int64_t lastWriteFrame_{};
    int64_t lastWriteMuteFrame_{};
    int64_t sourceDuration_ = -1;
    std::unique_ptr<PlayerDfxWriter> playerDfx_;
    std::shared_ptr<AudioStreamChecker> audioStreamChecker_ = nullptr;

    float loudnessGain_ = 0.0f;
    // Only use in Writedate(). Protect by writeMutex_.
    std::vector<uint8_t> rendererTmpBuffer_;
    // audio haptics play sync param id
    std::atomic<int32_t> audioHapticsSyncId_ = 0;

    bool latestForWorkgroupInited_ = false;
    struct RendererLatestInfoForWorkgroup latestForWorkgroup_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // RENDERER_IN_SERVER_H
