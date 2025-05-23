/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#ifndef NAPI_AUDIO_CAPTURER_STATE_CALLBACK_H
#define NAPI_AUDIO_CAPTURER_STATE_CALLBACK_H

#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_async_work.h"
#include "audio_policy_interface.h"
#include "audio_system_manager.h"
#include "audio_stream_manager.h"

namespace OHOS {
namespace AudioStandard {
class NapiAudioCapturerStateCallback : public AudioCapturerStateChangeCallback {
public:
    explicit NapiAudioCapturerStateCallback(napi_env env);
    virtual ~NapiAudioCapturerStateCallback();
    void SaveCallbackReference(napi_value args);
    bool IsSameCallback(const napi_value args);
    void RemoveCallbackReference(const napi_value args);
    void OnCapturerStateChange(
        const std::vector<std::shared_ptr<AudioCapturerChangeInfo>> &audioCapturerChangeInfos) override;
    void CreateCaptureStateTsfn(napi_env env);
    bool GetCaptureStateTsfnFlag();

private:
    struct AudioCapturerStateJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::vector<std::shared_ptr<AudioCapturerChangeInfo>> changeInfos;
    };

    void OnJsCallbackCapturerState(std::unique_ptr<AudioCapturerStateJsCallback> &jsCb);
    static void CapturerStateTsfnFinalize(napi_env env, void *data, void *hint);
    static void SafeJsCallbackCapturerStateWork(napi_env env, napi_value js_cb, void *context, void *data);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> capturerStateCallback_ = nullptr;
    bool regAmacStateTsfn_ = false;
    napi_threadsafe_function amacStateTsfn_ = nullptr;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif /* NAPI_AUDIO_CAPTURER_STATE_CALLBACK_H */
