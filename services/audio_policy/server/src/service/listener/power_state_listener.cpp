/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "power_state_listener.h"

#include <chrono>
#include <thread>

#include "suspend/sync_sleep_callback_ipc_interface_code.h"

#include "audio_errors.h"
#include "audio_log.h"
#include "audio_policy_server.h"

namespace OHOS {
namespace AudioStandard {
using namespace std::chrono_literals;
const int32_t AUDIO_INTERRUPT_SESSION_ID = 10000;

int32_t PowerStateListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
    MessageParcel &reply, MessageOption &option)
{
    AUDIO_DEBUG_LOG("code = %{public}d, flag = %{public}d", code, option.GetFlags());
    std::u16string descriptor = PowerStateListenerStub::GetDescriptor();
    std::u16string remoteDescriptor = data.ReadInterfaceToken();
    if (descriptor != remoteDescriptor) {
        AUDIO_ERR_LOG("Descriptor not match");
        return -1;
    }

    int32_t ret = ERR_OK;
    switch (code) {
        case static_cast<int32_t>(PowerMgr::SyncSleepCallbackInterfaceCode::CMD_ON_SYNC_SLEEP):
            ret = OnSyncSleepCallbackStub(data);
            break;
        case static_cast<int32_t>(PowerMgr::SyncSleepCallbackInterfaceCode::CMD_ON_SYNC_WAKEUP):
            ret = OnSyncWakeupCallbackStub(data);
            break;
        default:
            ret = IPCObjectStub::OnRemoteRequest(code, data, reply, option);
            break;
    }

    return ret;
}

int32_t PowerStateListenerStub::OnSyncSleepCallbackStub(MessageParcel &data)
{
    bool forceSleep = data.ReadBool();
    OnSyncSleep(forceSleep);

    return ERR_OK;
}

int32_t PowerStateListenerStub::OnSyncWakeupCallbackStub(MessageParcel &data)
{
    bool forceSleep = data.ReadBool();
    OnSyncWakeup(forceSleep);

    return ERR_OK;
}

PowerStateListener::PowerStateListener(const sptr<AudioPolicyServer> audioPolicyServer)
    : audioPolicyServer_(audioPolicyServer) {}

void PowerStateListener::OnSyncSleep(bool OnForceSleep)
{
    if (!OnForceSleep) {
        AUDIO_ERR_LOG("OnSyncSleep not force sleep");
        return;
    }

    ControlAudioFocus(true);
}

void PowerStateListener::OnSyncWakeup(bool OnForceSleep)
{
    if (!OnForceSleep) {
        AUDIO_ERR_LOG("OnSyncWakeup not force sleep");
        return;
    }

    ControlAudioFocus(false);
}

void PowerStateListener::ControlAudioFocus(bool applyFocus)
{
    AudioInterrupt audioInterrupt;
    audioInterrupt.contentType = ContentType::CONTENT_TYPE_UNKNOWN;
    audioInterrupt.streamUsage = StreamUsage::STREAM_USAGE_UNKNOWN;
    audioInterrupt.audioFocusType.streamType = AudioStreamType::STREAM_INTERNAL_FORCE_STOP;
    audioInterrupt.audioFocusType.sourceType = SOURCE_TYPE_INVALID;
    audioInterrupt.audioFocusType.isPlay = true;
    audioInterrupt.sessionID = AUDIO_INTERRUPT_SESSION_ID;
    audioInterrupt.pid = getpid();

    int32_t result = -1;
    if (applyFocus) {
        result = audioPolicyServer_->ActivateAudioInterrupt(audioInterrupt);
        if (result != SUCCESS) {
            AUDIO_ERR_LOG("Activate audio interrupt failed, err = %{public}d", result);
        }
    } else {
        result = audioPolicyServer_->DeactivateAudioInterrupt(audioInterrupt);
        if (result != SUCCESS) {
            AUDIO_ERR_LOG("Deactivate audio interrupt failed, err = %{public}d", result);
        }
    }
}
} // namespace AudioStandard
} // namespace OHOS