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
#ifndef NAPI_AUDIO_MICSTATECHANGE_CALLBACK_H
#define NAPI_AUDIO_MICSTATECHANGE_CALLBACK_H

#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_async_work.h"
#include "audio_system_manager.h"

namespace OHOS {
namespace AudioStandard {
const std::string MIC_STATE_CHANGE_CALLBACK_NAME = "micStateChange";

class NapiAudioManagerMicStateChangeCallback : public AudioManagerMicStateChangeCallback {
public:
    explicit NapiAudioManagerMicStateChangeCallback(napi_env env);
    virtual ~NapiAudioManagerMicStateChangeCallback();
    void SaveCallbackReference(const std::string &callbackName, napi_value args);
    bool IsSameCallback(const napi_value args);
    void RemoveCallbackReference(const napi_value args);
    void OnMicStateUpdated(const MicStateChangeEvent &micStateChangeEvent) override;
    void CreateManagerMicStateChangeTsfn(napi_env env);
    bool GetManagerMicStateChangeTsfnFlag();

private:
    struct AudioManagerMicStateChangeJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        MicStateChangeEvent micStateChangeEvent;
    };

    void OnJsCallbackMicStateChange(std::unique_ptr<AudioManagerMicStateChangeJsCallback> &jsCb);
    static void MicStateChangeTsfnFinalize(napi_env env, void *data, void *hint);
    static void SafeJsCallbackMicStateChangeWork(napi_env env, napi_value js_cb, void *context, void *data);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> micStateChangeCallback_ = nullptr;
    bool regAmMicStateChgTsfn_ = false;
    napi_threadsafe_function amMicStateChgTsfn_ = nullptr;
};
} // namespace AudioStandard
} // namespace OHOS
#endif /* NAPI_AUDIO_MICSTATECHANGE_CALLBACK_H */