/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include "multimedia_audio_volume_group_manager_callback.h"
#include "multimedia_audio_common.h"

namespace OHOS {
namespace AudioStandard {

void CjAudioRingerModeCallback::RegisterFunc(std::function<void(int32_t)> cjCallback)
{
    func_ = cjCallback;
}

void CjAudioRingerModeCallback::OnRingerModeUpdated(const AudioRingerMode &ringerMode)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (func_ == nullptr) {
        return;
    }
    int32_t cjRingerMode = ringerMode;
    func_(cjRingerMode);
}

void CjAudioManagerMicStateChangeCallback::RegisterFunc(std::function<void(CMicStateChangeEvent)> cjCallback)
{
    func_ = cjCallback;
}

void CjAudioManagerMicStateChangeCallback::OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent)
{
    std::lock_guard<std::mutex> lock(cbMutex_);
    if (func_ == nullptr) {
        return;
    }
    CMicStateChangeEvent cMic{};
    cMic.mute = micStateChangeEvent.mute;
    func_(cMic);
}
} // namespace AudioStandard
} // namespace OHOS
