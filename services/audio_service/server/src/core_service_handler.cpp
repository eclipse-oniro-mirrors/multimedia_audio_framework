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
#define LOG_TAG "CoreServiceHandler"
#endif

#include "core_service_handler.h"

#include "audio_errors.h"
#include "audio_common_log.h"

namespace OHOS {
namespace AudioStandard {
namespace {

}

CoreServiceHandler& CoreServiceHandler::GetInstance()
{
    static CoreServiceHandler CoreServiceHandler;
    return CoreServiceHandler;
}

CoreServiceHandler::CoreServiceHandler()
{
    AUDIO_INFO_LOG("Ctor");
}

CoreServiceHandler::~CoreServiceHandler()
{
    iCoreServiceProvider_ = nullptr;
    AUDIO_INFO_LOG("Dtor");
}

int32_t CoreServiceHandler::ConfigCoreServiceProvider(const sptr<ICoreServiceProviderIpc> coreServiceProvider)
{
    CHECK_AND_RETURN_RET_LOG(coreServiceProvider != nullptr, ERR_INVALID_PARAM, "Failed with null provider.");
    if (iCoreServiceProvider_ == nullptr) {
        iCoreServiceProvider_ = coreServiceProvider;
        return SUCCESS;
    }
    AUDIO_ERR_LOG("Provider is already configed!");
    return ERR_INVALID_OPERATION;
}

int32_t CoreServiceHandler::UpdateSessionOperation(uint32_t sessionId, SessionOperation operation,
    SessionOperationMsg opMsg)
{
    CHECK_AND_RETURN_RET_LOG(iCoreServiceProvider_ != nullptr, ERROR, "iCoreServiceProvider_ is nullptr");
    return iCoreServiceProvider_->UpdateSessionOperation(sessionId, operation, opMsg);
}

int32_t CoreServiceHandler::SetDefaultOutputDevice(const DeviceType defaultOutputDevice, const uint32_t sessionID,
    const StreamUsage streamUsage, bool isRunning)
{
    CHECK_AND_RETURN_RET_LOG(iCoreServiceProvider_ != nullptr, ERROR, "iCoreServiceProvider_ is nullptr");
    return iCoreServiceProvider_->SetDefaultOutputDevice(defaultOutputDevice, sessionID, streamUsage, isRunning);
}

std::string CoreServiceHandler::GetAdapterNameBySessionId(uint32_t sessionId)
{
    CHECK_AND_RETURN_RET_LOG(iCoreServiceProvider_ != nullptr, "", "iCoreServiceProvider_ is nullptr");
    return iCoreServiceProvider_->GetAdapterNameBySessionId(sessionId);
}

int32_t CoreServiceHandler::GetProcessDeviceInfoBySessionId(uint32_t sessionId, AudioDeviceDescriptor &deviceInfo)
{
    CHECK_AND_RETURN_RET_LOG(iCoreServiceProvider_ != nullptr, ERROR, "iCoreServiceProvider_ is nullptr");
    return iCoreServiceProvider_->GetProcessDeviceInfoBySessionId(sessionId, deviceInfo);
}

uint32_t CoreServiceHandler::GenerateSessionId()
{
    CHECK_AND_RETURN_RET_LOG(iCoreServiceProvider_ != nullptr, ERROR, "iCoreServiceProvider_ is nullptr");
    return iCoreServiceProvider_->GenerateSessionId();
}
} // namespace AudioStandard
} // namespace OHOS
