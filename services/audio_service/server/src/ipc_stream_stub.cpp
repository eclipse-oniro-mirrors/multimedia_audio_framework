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

#include "ipc_stream_stub.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
bool IpcStreamStub::CheckInterfaceToken(MessageParcel &data)
{
    static auto localDescriptor = IpcStream::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    if (remoteDescriptor != localDescriptor) {
        AUDIO_ERR_LOG("CheckInterFfaceToken failed.");
        return false;
    }
    return true;
}

int IpcStreamStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply, MessageOption &option)
{
    if (!CheckInterfaceToken(data)) {
        return AUDIO_ERR;
    }
    if (code >= IpcStreamMsg::IPC_STREAM_MAX_MSG) {
        AUDIO_WARNING_LOG("OnRemoteRequest unsupported request code:%{public}d.", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return (this->*funcList_[code])(data, reply);
}

int32_t IpcStreamStub::HandleRegisterStreamListener(MessageParcel &data, MessageParcel &reply)
{
    sptr<IRemoteObject> object = data.ReadRemoteObject();
    if (object == nullptr) {
        AUDIO_ERR_LOG("IpcStreamStub: HandleRegisterProcessCb obj is null");
        reply.WriteInt32(AUDIO_INVALID_PARAM);
        return AUDIO_INVALID_PARAM;
    }
    reply.WriteInt32(RegisterStreamListener(object));
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleResolveBuffer(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::shared_ptr<OHAudioBuffer> buffer;
    int32_t ret = ResolveBuffer(buffer);
    reply.WriteInt32(ret);
    if (ret == AUDIO_OK && buffer != nullptr) {
        OHAudioBuffer::WriteToParcel(buffer, reply);
    } else {
        AUDIO_ERR_LOG("error: ResolveBuffer failed.");
        return AUDIO_INVALID_PARAM;
    }

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleUpdatePosition(MessageParcel &data, MessageParcel &reply)
{
    AUDIO_INFO_LOG("IpcStreamStub::HandleUpdatePosition");
    (void)data;
    reply.WriteInt32(UpdatePosition());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetAudioSessionID(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uint32_t sessionId = 0;
    reply.WriteInt32(GetAudioSessionID(sessionId));
    reply.WriteUint32(sessionId);

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleStart(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Start());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandlePause(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Pause());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleStop(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Stop());
    return AUDIO_OK;
}
int32_t IpcStreamStub::HandleRelease(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Release());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleFlush(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Flush());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleDrain(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(Drain());
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetAudioTime(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uint64_t framePos = 0;
    uint64_t timeStamp = 0;
    reply.WriteInt32(GetAudioTime(framePos, timeStamp));
    reply.WriteUint64(framePos);
    reply.WriteInt64(timeStamp);
    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetLatency(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    uint64_t latency = 0;
    reply.WriteInt32(GetLatency(latency));
    reply.WriteUint64(latency);

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleSetRate(MessageParcel &data, MessageParcel &reply)
{
    int32_t rate = data.ReadInt32();
    reply.WriteInt32(SetRate(rate));

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetRate(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t rate = -1;
    reply.WriteInt32(GetRate(rate));
    reply.WriteInt32(rate);

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleSetLowPowerVolume(MessageParcel &data, MessageParcel &reply)
{
    float lowPowerVolume = data.ReadFloat();
    reply.WriteInt32(SetLowPowerVolume(lowPowerVolume));

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetLowPowerVolume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    float lowPowerVolume = 0.0;
    reply.WriteInt32(GetLowPowerVolume(lowPowerVolume));
    reply.WriteFloat(lowPowerVolume);

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleSetAudioEffectMode(MessageParcel &data, MessageParcel &reply)
{
    int32_t effectMode = data.ReadInt32();
    reply.WriteInt32(SetAudioEffectMode(effectMode));

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetAudioEffectMode(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t effectMode = -1;
    reply.WriteInt32(GetAudioEffectMode(effectMode));
    reply.WriteInt32(effectMode);

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleSetPrivacyType(MessageParcel &data, MessageParcel &reply)
{
    int32_t privacyType = data.ReadInt32();
    reply.WriteInt32(SetPrivacyType(privacyType));

    return AUDIO_OK;
}

int32_t IpcStreamStub::HandleGetPrivacyType(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t privacyType = -1;
    reply.WriteInt32(GetPrivacyType(privacyType));
    reply.WriteInt32(privacyType);

    return AUDIO_OK;
}
} // namespace AudioStandard
} // namespace OHOS