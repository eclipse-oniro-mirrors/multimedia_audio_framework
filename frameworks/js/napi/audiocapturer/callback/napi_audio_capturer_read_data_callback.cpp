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

#include "napi_audio_capturer_read_data_callback.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
NapiCapturerReadDataCallback::NapiCapturerReadDataCallback(napi_env env, NapiAudioCapturer *napiCapturer)
    : env_(env), napiCapturer_(napiCapturer)
{
    AUDIO_INFO_LOG("instance create");
}

NapiCapturerReadDataCallback::~NapiCapturerReadDataCallback()
{
    AUDIO_INFO_LOG("instance destroy");
}

void NapiCapturerReadDataCallback::AddCallbackReference(const std::string &callbackName, napi_value args)
{
    std::lock_guard<std::mutex> lock(mutex_);
    napi_ref callback = nullptr;
    const int32_t refCount = 1;
    napi_status status = napi_create_reference(env_, args, refCount, &callback);
    CHECK_AND_RETURN_LOG(status == napi_ok && callback != nullptr, "creating reference for callback failed");

    std::shared_ptr<AutoRef> cb = std::make_shared<AutoRef>(env_, callback);
    if (callbackName == READ_DATA_CALLBACK_NAME) {
        capturerReadDataCallback_ = cb;
        callback_ = callback;
    } else {
        AUDIO_ERR_LOG("Unknown callback type: %{public}s", callbackName.c_str());
    }
}

void NapiCapturerReadDataCallback::RemoveCallbackReference(napi_env env, napi_value callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    bool isEquals = false;
    napi_value copyValue = nullptr;

    if (callback == nullptr) {
        napi_status ret = napi_delete_reference(env, callback_);
        CHECK_AND_RETURN_LOG(napi_ok == ret, "delete callback reference failed");
        AUDIO_INFO_LOG("Remove Js Callback");
        return;
    }

    napi_get_reference_value(env, callback_, &copyValue);
    CHECK_AND_RETURN_LOG(copyValue != nullptr, "copyValue is nullptr");
    CHECK_AND_RETURN_LOG(napi_strict_equals(env, callback, copyValue, &isEquals) == napi_ok,
        "get napi_strict_equals failed");
    if (isEquals) {
        AUDIO_INFO_LOG("found JS Callback, delete it!");
        napi_status status = napi_delete_reference(env, capturerReadDataCallback_->cb_);
        CHECK_AND_RETURN_LOG(status == napi_ok, "deleting reference for callback failed");
    }
}

void NapiCapturerReadDataCallback::OnReadData(size_t length)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_DEBUG_LOG("OnReadData enqueue added");
    CHECK_AND_RETURN_LOG(capturerReadDataCallback_ != nullptr, "Cannot find the reference of readData callback");

    std::unique_ptr<CapturerReadDataJsCallback> cb = std::make_unique<CapturerReadDataJsCallback>();
    cb->callback = capturerReadDataCallback_;
    cb->callbackName = READ_DATA_CALLBACK_NAME;
    cb->bufDesc_.buffer = nullptr;
    cb->capturerNapiObj = napiCapturer_;

    CHECK_AND_RETURN_LOG(napiCapturer_ != nullptr, "Cannot find the reference to audio capturer napi");
    napiCapturer_->audioCapturer_->GetBufferDesc(cb->bufDesc_);
    if (cb->bufDesc_.buffer == nullptr) {
        return;
    }
    if (length > cb->bufDesc_.bufLength) {
        cb->bufDesc_.dataLength = cb->bufDesc_.bufLength;
    } else {
        cb->bufDesc_.dataLength = length;
    }

    return OnJsCapturerReadDataCallback(cb);
}

void NapiCapturerReadDataCallback::OnJsCapturerReadDataCallback(std::unique_ptr<CapturerReadDataJsCallback> &jsCb)
{
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(env_, &loop);
    CHECK_AND_RETURN_LOG(loop != nullptr, "loop is nullptr");

    uv_work_t *work = new(std::nothrow) uv_work_t;
    CHECK_AND_RETURN_LOG(work != nullptr, "readData Js Callback: No memory");

    if (jsCb.get() == nullptr) {
        AUDIO_ERR_LOG("readData Js Callback is null");
        delete work;
        return;
    }

    work->data = reinterpret_cast<void *>(jsCb.get());

    int ret = uv_queue_work_with_qos(loop, work, [] (uv_work_t *work) {},
        WorkCallbackCapturerReadData, uv_qos_default);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to execute uv work queue");
        delete work;
    } else {
        jsCb.release();
    }
}

void NapiCapturerReadDataCallback::WorkCallbackCapturerReadData(uv_work_t *work, int status)
{
    // Js Thread
    CHECK_AND_RETURN_LOG(work != nullptr, "capture read data work is nullptr");
    CapturerReadDataJsCallback *event = reinterpret_cast<CapturerReadDataJsCallback *>(work->data);
    CHECK_AND_RETURN_LOG(event != nullptr, "capture read data event is nullptr");
    std::string request = event->callbackName;
    CHECK_AND_RETURN_LOG(event->callback != nullptr, "event is nullptr");
    napi_env env = event->callback->env_;
    napi_ref callback = event->callback->cb_;

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);
    CHECK_AND_RETURN_LOG(scope != nullptr, "%{public}s scope is nullptr", request.c_str());
    AUDIO_DEBUG_LOG("JsCallback %{public}s, uv_queue_work_with_qos start", request.c_str());
    do {
        CHECK_AND_BREAK_LOG(status != UV_ECANCELED, "%{public}s canceled", request.c_str());

        napi_value jsCallback = nullptr;
        napi_status nstatus = napi_get_reference_value(env, callback, &jsCallback);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && jsCallback != nullptr, "%{public}s get reference value failed",
            request.c_str());
        napi_value args[ARGS_ONE] = { nullptr };
        nstatus = napi_create_external_arraybuffer(env, event->bufDesc_.buffer, event->bufDesc_.dataLength,
            [](napi_env env, void *data, void *hint) {}, nullptr, &args[PARAM0]);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok && args[PARAM0] != nullptr,
            "%{public}s callback fail to create buffer", request.c_str());
        const size_t argCount = 1;
        napi_value result = nullptr;
        nstatus = napi_call_function(env, nullptr, jsCallback, argCount, args, &result);
        CHECK_AND_BREAK_LOG(nstatus == napi_ok, "fail to call %{public}s callback", request.c_str());

        event->capturerNapiObj->audioCapturer_->Enqueue(event->bufDesc_);
    } while(0);
    napi_close_handle_scope(env, scope);
    delete event;
    delete work;
}
} // namespace AudioStandard
} // namespace OHOS
