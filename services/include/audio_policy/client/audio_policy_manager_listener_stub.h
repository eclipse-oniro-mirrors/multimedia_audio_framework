/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#ifndef AUDIO_POLICY_MANAGER_LISTENER_STUB_H
#define AUDIO_POLICY_MANAGER_LISTENER_STUB_H

#include <thread>

#include "audio_interrupt_callback.h"
#include "i_standard_audio_policy_manager_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioPolicyManagerListenerStub : public IRemoteStub<IStandardAudioPolicyManagerListener> {
public:
    AudioPolicyManagerListenerStub();
    virtual ~AudioPolicyManagerListenerStub();

    // IStandardAudioManagerListener override
    int OnRemoteRequest(uint32_t code, MessageParcel &data,
                                MessageParcel &reply, MessageOption &option) override;
    void OnInterrupt(const InterruptEvent &interruptEvent) override;
    // AudioManagerListenerStub
    void SetInterruptCallback(const std::weak_ptr<AudioInterruptCallback> &callback);
private:
    std::weak_ptr<AudioInterruptCallback> callback_;
    void ReadInterruptEventParams(MessageParcel &data, InterruptEvent &interruptEvent);
    std::vector<std::unique_ptr<std::thread>> interruptThreads_;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_POLICY_MANAGER_LISTENER_STUB_H
