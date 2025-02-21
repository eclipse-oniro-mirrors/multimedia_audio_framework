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

#include "policy_provider_stub.h"
#include "audio_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {
bool PolicyProviderStub::CheckInterfaceToken(MessageParcel &data)
{
    static auto localDescriptor = IPolicyProviderIpc::GetDescriptor();
    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(remoteDescriptor == localDescriptor, false, "CheckInterFfaceToken failed.");
    return true;
}

int PolicyProviderStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
    MessageOption &option)
{
    bool ret = CheckInterfaceToken(data);
    CHECK_AND_RETURN_RET(ret, AUDIO_ERR);
    if (code >= IPolicyProviderMsg::POLICY_PROVIDER_MAX_MSG) {
        AUDIO_WARNING_LOG("OnRemoteRequest unsupported request code:%{public}d.", code);
        return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    return (this->*funcList_[code])(data, reply);
}

int32_t PolicyProviderStub::HandleGetProcessDeviceInfo(MessageParcel &data, MessageParcel &reply)
{
    AudioProcessConfig config;
    int32_t ret = ProcessConfig::ReadConfigFromParcel(config, data);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "ReadConfigFromParcel failed %{public}d", ret);
    DeviceInfo deviceInfo;
    ret = GetProcessDeviceInfo(config, deviceInfo);
    CHECK_AND_RETURN_RET_LOG(ret == SUCCESS, ERR_OPERATION_FAILED, "GetProcessDeviceInfo failed %{public}d", ret);
    deviceInfo.Marshalling(reply);
    return AUDIO_OK;
}

int32_t PolicyProviderStub::HandleInitSharedVolume(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    std::shared_ptr<AudioSharedMemory> buffer = nullptr;
    int32_t ret = InitSharedVolume(buffer);
    if (ret == SUCCESS && buffer != nullptr) {
        ret = AudioSharedMemory::WriteToParcel(buffer, reply);
    } else {
        AUDIO_ERR_LOG("error: ResolveBuffer failed.");
        return AUDIO_INVALID_PARAM;
    }
    return ret;
}

int32_t PolicyProviderStub::HandleSetWakeupCapturer(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    int32_t ret = SetWakeUpAudioCapturerFromAudioServer();
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int32_t PolicyProviderStub::HandleSetCapturer(MessageParcel &data, MessageParcel &reply)
{
    AudioCapturerInfo capturerInfo;
    AudioStreamInfo streamInfo;
    uint32_t sessionId;
    capturerInfo.Unmarshalling(data);
    streamInfo.Unmarshalling(data);
    data.ReadUint32(sessionId);
    int32_t ret = NotifyCapturerAdded(capturerInfo, streamInfo, sessionId);
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

int32_t PolicyProviderStub::HandleWakeupCapturerRemoved(MessageParcel &data, MessageParcel &reply)
{
    int32_t ret = NotifyWakeUpCapturerRemoved();
    reply.WriteInt32(ret);
    return AUDIO_OK;
}

PolicyProviderWrapper::~PolicyProviderWrapper()
{
    policyWorker_ = nullptr;
}

PolicyProviderWrapper::PolicyProviderWrapper(IPolicyProvider *policyWorker) : policyWorker_(policyWorker)
{
}

int32_t PolicyProviderWrapper::GetProcessDeviceInfo(const AudioProcessConfig &config, DeviceInfo &deviceInfo)
{
    CHECK_AND_RETURN_RET_LOG(policyWorker_ != nullptr, AUDIO_INIT_FAIL, "policyWorker_ is null");
    return policyWorker_->GetProcessDeviceInfo(config, deviceInfo);
}

int32_t PolicyProviderWrapper::InitSharedVolume(std::shared_ptr<AudioSharedMemory> &buffer)
{
    CHECK_AND_RETURN_RET_LOG(policyWorker_ != nullptr, AUDIO_INIT_FAIL, "policyWorker_ is null");
    return policyWorker_->InitSharedVolume(buffer);
}

int32_t PolicyProviderWrapper::SetWakeUpAudioCapturerFromAudioServer()
{
    CHECK_AND_RETURN_RET_LOG(policyWorker_ != nullptr, AUDIO_INIT_FAIL, "policyWorker_ is null");
    return policyWorker_->SetWakeUpAudioCapturerFromAudioServer();
}

int32_t PolicyProviderWrapper::NotifyCapturerAdded(AudioCapturerInfo capturerInfo, AudioStreamInfo streamInfo,
    uint32_t sessionId)
{
    CHECK_AND_RETURN_RET_LOG(policyWorker_ != nullptr, AUDIO_INIT_FAIL, "policyWorker_ is null");
    return policyWorker_->NotifyCapturerAdded(capturerInfo, streamInfo, sessionId);
}

int32_t PolicyProviderWrapper::NotifyWakeUpCapturerRemoved()
{
    CHECK_AND_RETURN_RET_LOG(policyWorker_ != nullptr, AUDIO_INIT_FAIL, "policyWorker_ is null");
    return policyWorker_->NotifyWakeUpCapturerRemoved();
}
} // namespace AudioStandard
} // namespace OHOS
