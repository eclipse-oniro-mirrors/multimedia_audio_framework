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

#ifndef AUDIO_POLICY_CLIENT_STUB_H
#define AUDIO_POLICY_CLIENT_STUB_H

#include "event_handler.h"
#include "event_runner.h"
#include "audio_policy_client.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
class AudioPolicyClientStub : public IRemoteStub<IAudioPolicyClient> {
public:
    AudioPolicyClientStub();
    ~AudioPolicyClientStub();
    virtual int OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
        MessageOption &option) override;
private:
    void HandleVolumeKeyEvent(MessageParcel &data, MessageParcel &reply);
    void HandleAudioFocusInfoChange(MessageParcel &data, MessageParcel &reply);
    void HandleAudioFocusRequested(MessageParcel &data, MessageParcel &reply);
    void HandleAudioFocusAbandoned(MessageParcel &data, MessageParcel &reply);
    void HandleDeviceChange(MessageParcel &data, MessageParcel &reply);
    void HandleRingerModeUpdated(MessageParcel &data, MessageParcel &reply);
    void HandleMicStateChange(MessageParcel &data, MessageParcel &reply);
    void HandlePreferredOutputDeviceUpdated(MessageParcel &data, MessageParcel &reply);
    void HandlePreferredInputDeviceUpdated(MessageParcel &data, MessageParcel &reply);
    void HandleRendererStateChange(MessageParcel &data, MessageParcel &reply);
    void HandleCapturerStateChange(MessageParcel &data, MessageParcel &reply);
    void HandleRendererDeviceChange(MessageParcel &data, MessageParcel &reply);

    using HandlerFunc = void (AudioPolicyClientStub::*)(MessageParcel &data, MessageParcel &reply);
    static inline HandlerFunc handlers[]  = {
        &AudioPolicyClientStub::HandleVolumeKeyEvent,
        &AudioPolicyClientStub::HandleAudioFocusInfoChange,
        &AudioPolicyClientStub::HandleAudioFocusRequested,
        &AudioPolicyClientStub::HandleAudioFocusAbandoned,
        &AudioPolicyClientStub::HandleDeviceChange,
        &AudioPolicyClientStub::HandleRingerModeUpdated,
        &AudioPolicyClientStub::HandleMicStateChange,
        &AudioPolicyClientStub::HandlePreferredOutputDeviceUpdated,
        &AudioPolicyClientStub::HandlePreferredInputDeviceUpdated,
        &AudioPolicyClientStub::HandleRendererStateChange,
        &AudioPolicyClientStub::HandleCapturerStateChange,
        &AudioPolicyClientStub::HandleRendererDeviceChange,
    };
    static constexpr size_t handlersNums = sizeof(handlers) / sizeof(HandlerFunc);
    static_assert(handlersNums == (static_cast<size_t> (AudioPolicyClientCode::AUDIO_POLICY_CLIENT_CODE_MAX) + 1),
        "please check audio_policy_client.h");
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_POLICY_CLIENT_STUB_H