/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
#define LOG_TAG "AudioClientTrackerCallbackStub"
#endif

#include "audio_client_tracker_callback_stub.h"
#include "audio_policy_log.h"

namespace OHOS {
namespace AudioStandard {
AudioClientTrackerCallbackStub::AudioClientTrackerCallbackStub()
{
}

AudioClientTrackerCallbackStub::~AudioClientTrackerCallbackStub()
{
}

void AudioClientTrackerCallbackStub::SelectCodeCase(uint32_t code,
    StreamSetStateEventInternal &streamSetStateEventInternal)
{
    switch (code) {
        case PAUSEDSTREAM:
            return PausedStreamImpl(streamSetStateEventInternal);
        case RESUMESTREAM:
            return ResumeStreamImpl(streamSetStateEventInternal);
        case MUTESTREAM:
            return MuteStreamImpl(streamSetStateEventInternal);
        case UNMUTESTREAM:
            return UnmuteStreamImpl(streamSetStateEventInternal);
        default:
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return;
    }
    return;
}

int AudioClientTrackerCallbackStub::OnRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    CHECK_AND_RETURN_RET_LOG(data.ReadInterfaceToken() == GetDescriptor(), -1,
        "AudioClientTrackerCallbackStub: ReadInterfaceToken failed");

    switch (code) {
        case PAUSEDSTREAM:
        case RESUMESTREAM:
        case MUTESTREAM:
        case UNMUTESTREAM: {
            StreamSetStateEventInternal sreamSetStateEventInternal = {};
            sreamSetStateEventInternal.streamSetState= static_cast<StreamSetState>(data.ReadInt32());
            sreamSetStateEventInternal.streamUsage = static_cast<StreamUsage>(data.ReadInt32());
            SelectCodeCase(code, sreamSetStateEventInternal);
            return AUDIO_OK;
        }
        case SETLOWPOWERVOL: {
            float volume = data.ReadFloat();
            SetLowPowerVolumeImpl(volume);
            return AUDIO_OK;
        }
        case GETLOWPOWERVOL: {
            float volume;
            GetLowPowerVolumeImpl(volume);
            reply.WriteFloat(volume);
            return AUDIO_OK;
        }
        case GETSINGLESTREAMVOL: {
            float volume;
            GetSingleStreamVolumeImpl(volume);
            reply.WriteFloat(volume);
            return AUDIO_OK;
        }
        case SETOFFLOADMODE:
        case UNSETOFFLOADMODE: {
            return OffloadRemoteRequest(code, data, reply, option);
        }
        default: {
            AUDIO_ERR_LOG("default case, need check AudioListenerStub");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
        }
    }

    return 0;
}

int AudioClientTrackerCallbackStub::OffloadRemoteRequest(
    uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (code == SETOFFLOADMODE) {
        int32_t state = data.ReadInt32();
        bool isAppBack = data.ReadBool();
        SetOffloadModeImpl(state, isAppBack);
        return AUDIO_OK;
    }

    if (code == UNSETOFFLOADMODE) {
        UnsetOffloadModeImpl();
        return AUDIO_OK;
    }
    return 0;
}

void AudioClientTrackerCallbackStub::SetClientTrackerCallback(
    const std::weak_ptr<AudioClientTracker> &callback)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    callback_ = callback;
}

void AudioClientTrackerCallbackStub::UnsetClientTrackerCallback()
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    callback_.reset();
}

void AudioClientTrackerCallbackStub::MuteStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->MuteStreamImpl(streamSetStateEventInternal);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: MuteStreamImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::UnmuteStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->UnmuteStreamImpl(streamSetStateEventInternal);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: UnmuteStreamImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::PausedStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->PausedStreamImpl(streamSetStateEventInternal);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: PausedStreamImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::SetLowPowerVolumeImpl(float volume)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->SetLowPowerVolumeImpl(volume);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: SetLowPowerVolumeImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::ResumeStreamImpl(
    const StreamSetStateEventInternal &streamSetStateEventInternal)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->ResumeStreamImpl(streamSetStateEventInternal);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: ResumeStreamImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::SetOffloadModeImpl(int32_t state, bool isAppBack)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->SetOffloadModeImpl(state, isAppBack);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: SetOffloadModeImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::UnsetOffloadModeImpl()
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->UnsetOffloadModeImpl();
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: UnsetOffloadModeImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::GetLowPowerVolumeImpl(float &volume)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->GetLowPowerVolumeImpl(volume);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: GetLowPowerVolumeImpl callback_ is nullptr");
    }
}

void AudioClientTrackerCallbackStub::GetSingleStreamVolumeImpl(float &volume)
{
    std::unique_lock<std::mutex> lock(clientTrackerMutex_);
    std::shared_ptr<AudioClientTracker> cb = callback_.lock();
    lock.unlock();
    if (cb != nullptr) {
        cb->GetSingleStreamVolumeImpl(volume);
    } else {
        AUDIO_WARNING_LOG("AudioClientTrackerCallbackStub: GetSingleStreamVolumeImpl callback_ is nullptr");
    }
}
} // namespace AudioStandard
} // namespace OHOS
