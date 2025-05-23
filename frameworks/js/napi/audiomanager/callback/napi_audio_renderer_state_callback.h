/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef NAPI_AUDIO_RENDERER_STATE_CALLBACK_H
#define NAPI_AUDIO_RENDERER_STATE_CALLBACK_H

#include <uv.h>
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "napi_async_work.h"
#include "audio_system_manager.h"
#include "audio_stream_manager.h"

namespace OHOS {
namespace AudioStandard {
class NapiAudioRendererStateCallback : public AudioRendererStateChangeCallback {
public:
    explicit NapiAudioRendererStateCallback(napi_env env);
    virtual ~NapiAudioRendererStateCallback();
    void SaveCallbackReference(napi_value args);
    bool IsSameCallback(const napi_value args);
    void RemoveCallbackReference(const napi_value args);
    void OnRendererStateChange(
        const std::vector<std::shared_ptr<AudioRendererChangeInfo>> &audioRendererChangeInfos) override;
    void CreateRendererStateTsfn(napi_env env);
    bool GetRendererStateTsfnFlag();

private:
    struct AudioRendererStateJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::vector<std::shared_ptr<AudioRendererChangeInfo>> changeInfos;
    };

    void OnJsCallbackRendererState(std::unique_ptr<AudioRendererStateJsCallback> &jsCb);
    static void SafeJsCallbackRendererStateWork(napi_env env, napi_value js_cb, void *context, void *data);
    static void RendererStateTsfnFinalize(napi_env env, void *data, void *hint);

    std::mutex mutex_;
    napi_env env_ = nullptr;
    std::shared_ptr<AutoRef> rendererStateCallback_ = nullptr;
    bool regAmRendererSatTsfn_ = false;
    napi_threadsafe_function amRendererSatTsfn_ = nullptr;
};
}  // namespace AudioStandard
}  // namespace OHOS
#endif /* NAPI_AUDIO_RENDERER_STATE_CALLBACK_H */