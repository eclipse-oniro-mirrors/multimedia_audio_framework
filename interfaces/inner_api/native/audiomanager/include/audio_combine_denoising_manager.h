/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ST_AUDIO_COMBINE_DENOISING_MANAGER_H
#define ST_AUDIO_COMBINE_DENOISING_MANAGER_H

#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
class AudioNnStateChangeCallback {
public:
    virtual ~AudioNnStateChangeCallback() = default;

    virtual void OnNnStateChange(const int32_t &nnState) = 0;
};

class AudioCombineDenoisingManager {
public:
    AudioCombineDenoisingManager() = default;
    virtual ~AudioCombineDenoisingManager() = default;

    static AudioCombineDenoisingManager *GetInstance();
    int32_t RegisterNnStateEventListener(const std::shared_ptr<AudioNnStateChangeCallback> &callback);
    int32_t UnregisterNnStateEventListener();
};
} // namespace AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_COMBINE_DENOISING_MANAGER_H