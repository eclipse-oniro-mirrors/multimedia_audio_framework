/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef AUDIO_MANAGER_INTERRUPT_CALLBACK_NAPI_H_
#define AUDIO_MANAGER_INTERRUPT_CALLBACK_NAPI_H_
#include "audio_common_napi.h"
#include "audio_manager_napi.h"
#include "audio_system_manager.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace OHOS {
namespace AudioStandard {
class AudioManagerInterruptCallbackNapi : public AudioManagerCallback {
public:
    explicit AudioManagerInterruptCallbackNapi(napi_env env);
    virtual ~AudioManagerInterruptCallbackNapi();
    void SaveCallbackReference(const std::string &callbackName, napi_value args);
    void RemoveCallbackReference(const std::string &callbackName, napi_value args);
    void RemoveAllCallbackReferences(const std::string &callbackName);
    int32_t GetInterruptCallbackListSize();
    void OnInterrupt(const InterruptAction &interruptAction) override;

private:
    struct AudioManagerInterruptJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        InterruptAction interruptAction;
    };

    void OnJsCallbackAudioManagerInterrupt(std::unique_ptr<AudioManagerInterruptJsCallback> &jsCb);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::list<std::shared_ptr<AutoRef>> audioManagerInterruptCallbackList_;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif // AUDIO_MANAGER_INTERRUPT_CALLBACK_NAPI_H_
