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

#include "audio_process_proxy.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
AudioProcessProxy::AudioProcessProxy(const sptr<IRemoteObject> &impl) : IRemoteProxy<IAudioProcess>(impl)
{
    AUDIO_INFO_LOG("AudioProcessProxy()");
}

AudioProcessProxy::~AudioProcessProxy()
{
    AUDIO_INFO_LOG("~AudioProcessProxy()");
}

int32_t AudioProcessProxy::ResolveBuffer(std::shared_ptr<OHAudioBuffer> &buffer)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_RESOLVE_BUFFER, data, reply, option);
    CHECK_AND_RETURN_RET_LOG((ret == AUDIO_OK && reply.ReadInt32() == AUDIO_OK), ERR_OPERATION_FAILED,
        "ResolveBuffer failed, error: %{public}d", ret);
    buffer = OHAudioBuffer::ReadFromParcel(reply);
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, ERR_OPERATION_FAILED, "ReadFromParcel failed");
    return SUCCESS;
}

int32_t AudioProcessProxy::Start()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_START, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "Start failed, error: %{public}d", ret);

    return reply.ReadInt32();
}

int32_t AudioProcessProxy::Pause(bool isFlush)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");
    data.WriteBool(isFlush);
    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_PAUSE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "Pause failed, error: %{public}d", ret);

    return reply.ReadInt32();
}

int32_t AudioProcessProxy::Resume()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_RESUME, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "Resume failed, error: %{public}d", ret);

    return reply.ReadInt32();
}

int32_t AudioProcessProxy::Stop()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_STOP, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "Stop failed, error: %{public}d", ret);

    return reply.ReadInt32();
}

int32_t AudioProcessProxy::RequestHandleInfo()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC); // MessageOption::TF_ASYNC for no delay

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_REQUEST_HANDLE_INFO, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "RequestHandleInfo failed: %{public}d", ret);

    return reply.ReadInt32();
}

int32_t AudioProcessProxy::Release()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;

    CHECK_AND_RETURN_RET_LOG(data.WriteInterfaceToken(GetDescriptor()), ERROR, "Write descriptor failed!");

    int ret = Remote()->SendRequest(IAudioProcessMsg::ON_RELEASE, data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AUDIO_OK, ERR_OPERATION_FAILED, "Release failed, error: %{public}d", ret);

    return reply.ReadInt32();
}
} // namespace AudioStandard
} // namespace OHOS
