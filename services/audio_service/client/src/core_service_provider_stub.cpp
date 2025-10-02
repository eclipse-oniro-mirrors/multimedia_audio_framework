/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "CoreServiceProviderStub"
#endif

#include "core_service_provider_stub.h"
#include "audio_service_log.h"
#include "audio_errors.h"

namespace OHOS {
namespace AudioStandard {

CoreServiceProviderWrapper::~CoreServiceProviderWrapper()
{
    coreServiceWorker_ = nullptr;
}

CoreServiceProviderWrapper::CoreServiceProviderWrapper(ICoreServiceProvider *coreServiceWorker)
    : coreServiceWorker_(coreServiceWorker)
{
}

int32_t CoreServiceProviderWrapper::UpdateSessionOperation(uint32_t sessionId, uint32_t operation, uint32_t opMsg)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    return coreServiceWorker_->UpdateSessionOperation(sessionId, static_cast<SessionOperation>(operation),
        static_cast<SessionOperationMsg>(opMsg));
}

int32_t CoreServiceProviderWrapper::ReloadCaptureSession(uint32_t sessionId, uint32_t operation)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    return coreServiceWorker_->ReloadCaptureSession(sessionId, static_cast<SessionOperation>(operation));
}

int32_t CoreServiceProviderWrapper::SetDefaultOutputDevice(int32_t defaultOutputDevice,
    uint32_t sessionID, int32_t streamUsage, bool isRunning, bool skipForce)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    return coreServiceWorker_->SetDefaultOutputDevice(static_cast<DeviceType>(defaultOutputDevice), sessionID,
        static_cast<StreamUsage>(streamUsage), isRunning, skipForce);
}

int32_t CoreServiceProviderWrapper::GetAdapterNameBySessionId(uint32_t sessionID, std::string& name)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    name = coreServiceWorker_->GetAdapterNameBySessionId(sessionID);
    return SUCCESS;
}

int32_t CoreServiceProviderWrapper::GetProcessDeviceInfoBySessionId(uint32_t sessionId,
    AudioDeviceDescriptor &deviceInfo, AudioStreamInfo &streamInfo, bool isReloadProcess)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    return coreServiceWorker_->GetProcessDeviceInfoBySessionId(sessionId, deviceInfo, streamInfo, isReloadProcess);
}

int32_t CoreServiceProviderWrapper::GenerateSessionId(uint32_t &sessionId)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, 0, "coreServiceWorker_ is null");
    sessionId = coreServiceWorker_->GenerateSessionId();
    return SUCCESS;
}

int32_t CoreServiceProviderWrapper::SetWakeUpAudioCapturerFromAudioServer(
    const AudioProcessConfig &config, int32_t &ret)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceWorker_ != nullptr, AUDIO_INIT_FAIL, "coreServiceWorker_ is null");
    ret = coreServiceWorker_->SetWakeUpAudioCapturerFromAudioServer(config);
    return SUCCESS;
}
} // namespace AudioStandard
} // namespace OHOS
