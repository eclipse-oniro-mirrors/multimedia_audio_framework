/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MULTIMEDIA_AUDIO_SESSION_MANAGER_IMPL_H
#define MULTIMEDIA_AUDIO_SESSION_MANAGER_IMPL_H
#include "audio_session_manager.h"
#include "cj_common_ffi.h"
#include "multimedia_audio_ffi.h"
#include "multimedia_audio_session_manager_callback.h"
#include "native/ffi_remote_data.h"

namespace OHOS {
namespace AudioStandard {
class MMAAudioSessionManagerImpl : public OHOS::FFI::FFIData {
    DECL_TYPE(MMAAudioSessionManagerImpl, OHOS::FFI::FFIData)
public:
    MMAAudioSessionManagerImpl();
    ~MMAAudioSessionManagerImpl();

    void ActivateAudioSession(CAudioSessionStrategy& strategy, int32_t* errorCode);
    void DeactivateAudioSession(int32_t* errorCode);
    bool IsAudioSessionActivated();
    void On(std::string type, int64_t id, int32_t* errorCode);

private:
    AudioSessionManager* sessionMgr_;
    std::shared_ptr<CjAudioSessionCallback> sessionCallback_ = nullptr;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // MULTIMEDIA_AUDIO_SESSION_MANAGER_IMPL_H