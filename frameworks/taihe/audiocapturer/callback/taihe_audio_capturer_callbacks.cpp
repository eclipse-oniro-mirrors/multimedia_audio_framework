/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "TaiheAudioCapturerCallback"
#endif

#include "taihe_audio_capturer_callbacks.h"
#include <mutex>
#include <thread>
#include "audio_errors.h"
#include "audio_capturer_log.h"
#include "taihe_audio_enum.h"
#include "taihe_param_utils.h"

namespace ANI::Audio {
TaiheAudioCapturerCallback::TaiheAudioCapturerCallback()
{
    AUDIO_DEBUG_LOG("TaiheAudioCapturerCallback: instance create");
}

TaiheAudioCapturerCallback::~TaiheAudioCapturerCallback()
{
    AUDIO_DEBUG_LOG("TaiheAudioCapturerCallback: instance destroy");
}

void TaiheAudioCapturerCallback::SaveCallbackReference(const std::string &callbackName,
    std::shared_ptr<uintptr_t> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // create function that will operate while save callback reference success.
    std::function<void(std::shared_ptr<AutoRef> generatedCallback)> successed =
        [this, callbackName](std::shared_ptr<AutoRef> generatedCallback) {
        if (callbackName == INTERRUPT_CALLBACK_NAME || callbackName == AUDIO_INTERRUPT_CALLBACK_NAME) {
            interruptCallback_ = generatedCallback;
            return;
        }
        if (callbackName == STATE_CHANGE_CALLBACK_NAME) {
            stateChangeCallback_ = generatedCallback;
            return;
        }
    };
    TaiheAudioCapturerCallbackInner::SaveCallbackReferenceInner(callbackName, callback, successed);
    if (!mainHandler_) {
        std::shared_ptr<OHOS::AppExecFwk::EventRunner> runner = OHOS::AppExecFwk::EventRunner::GetMainEventRunner();
        CHECK_AND_RETURN_LOG(runner != nullptr, "runner is null");
        mainHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    } else {
        AUDIO_DEBUG_LOG("mainHandler_ is not nullptr");
    }
}

std::shared_ptr<AutoRef> TaiheAudioCapturerCallback::GetCallback(const std::string &callbackName)
{
    std::shared_ptr<AutoRef> cb = nullptr;

    if (callbackName == AUDIO_INTERRUPT_CALLBACK_NAME) {
        return interruptCallback_;
    }
    if (callbackName == STATE_CHANGE_CALLBACK_NAME) {
        return stateChangeCallback_;
    }
    AUDIO_ERR_LOG("TaiheAudioCapturerCallback->GetCallback Unknown callback type: %{public}s", callbackName.c_str());
    return cb;
}

void TaiheAudioCapturerCallback::RemoveCallbackReference(const std::string &callbackName,
    std::shared_ptr<uintptr_t> &callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    // create function that will operate while save callback reference success.
    std::function<void()> successed =
        [this, callbackName]() {
            if (callbackName == AUDIO_INTERRUPT_CALLBACK_NAME) {
                interruptCallback_ = nullptr;
                return;
            }
            if (callbackName == STATE_CHANGE_CALLBACK_NAME) {
                stateChangeCallback_ = nullptr;
                return;
            }
        };
    RemoveCallbackReferenceInner(callbackName, callback, successed);
}

bool TaiheAudioCapturerCallback::CheckIfTargetCallbackName(const std::string &callbackName)
{
    if (callbackName == INTERRUPT_CALLBACK_NAME || callbackName == AUDIO_INTERRUPT_CALLBACK_NAME ||
        callbackName == STATE_CHANGE_CALLBACK_NAME) {
        return true;
    }
    return false;
}

void TaiheAudioCapturerCallback::OnInterrupt(const OHOS::AudioStandard::InterruptEvent &interruptEvent)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_DEBUG_LOG("TaiheAudioCapturerCallback: OnInterrupt is called, hintType: %{public}d", interruptEvent.hintType);
    CHECK_AND_RETURN_LOG(interruptCallback_ != nullptr, "Cannot find the reference of interrupt callback");

    std::unique_ptr<AudioCapturerJsCallback> cb = std::make_unique<AudioCapturerJsCallback>();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = interruptCallback_;
    cb->callbackName = INTERRUPT_CALLBACK_NAME;
    cb->interruptEvent = interruptEvent;
    return OnJsCallbackInterrupt(cb);
}

void TaiheAudioCapturerCallback::OnJsCallbackInterrupt(std::unique_ptr<AudioCapturerJsCallback> &jsCb)
{
    if (jsCb.get() == nullptr) {
        AUDIO_ERR_LOG("OnJsCallbackInterrupt: jsCb.get() is null");
        return;
    }
    CHECK_AND_RETURN_LOG(mainHandler_ != nullptr, "mainHandler_ is nullptr");
    AudioCapturerJsCallback *event = jsCb.release();
    CHECK_AND_RETURN_LOG((event != nullptr) && (event->callback != nullptr), "event is nullptr.");
    auto sharePtr = shared_from_this();
    auto task = [event, sharePtr]() {
        if (sharePtr != nullptr) {
            sharePtr->SafeJsCallbackInterruptWork(event);
        }
    };
    mainHandler_->PostTask(task, "OnAudioInterrupt", 0, OHOS::AppExecFwk::EventQueue::Priority::IMMEDIATE, {});
}

void TaiheAudioCapturerCallback::SafeJsCallbackInterruptWork(AudioCapturerJsCallback *event)
{
    CHECK_AND_RETURN_LOG((event != nullptr) && (event->callback != nullptr),
        "SafeJsCallbackInterruptWork: no memory");
    std::shared_ptr<AudioCapturerJsCallback> safeContext(
        static_cast<AudioCapturerJsCallback*>(event),
        [](AudioCapturerJsCallback *ptr) {
            delete ptr;
    });
    std::string request = event->callbackName;
    InterruptEvent interruptEvent = {
        .eventType = TaiheAudioEnum::ToTaiheInterruptType(event->interruptEvent.eventType),
        .forceType = TaiheAudioEnum::ToTaiheInterruptForceType(event->interruptEvent.forceType),
        .hintType = TaiheAudioEnum::ToTaiheInterruptHint(event->interruptEvent.hintType),
    };
    do {
        std::shared_ptr<taihe::callback<void(InterruptEvent const&)>> cacheCallback =
            std::reinterpret_pointer_cast<taihe::callback<void(InterruptEvent const&)>>(event->callback->cb_);
        CHECK_AND_BREAK_LOG(cacheCallback != nullptr, "%{public}s get reference value fail", request.c_str());
        (*cacheCallback)(interruptEvent);
    } while (0);
}

void TaiheAudioCapturerCallback::OnStateChange(const OHOS::AudioStandard::CapturerState state)
{
    std::lock_guard<std::mutex> lock(mutex_);
    AUDIO_DEBUG_LOG("TaiheAudioCapturerOnStateChange is called,Callback: state: %{public}d", state);
    CHECK_AND_RETURN_LOG(stateChangeCallback_ != nullptr, "Cannot find the reference of stateChange callback");

    std::unique_ptr<AudioCapturerJsCallback> cb = std::make_unique<AudioCapturerJsCallback>();
    CHECK_AND_RETURN_LOG(cb != nullptr, "No memory");
    cb->callback = stateChangeCallback_;
    cb->callbackName = STATE_CHANGE_CALLBACK_NAME;
    cb->state = state;
    return OnJsCallbackStateChange(cb);
}

void TaiheAudioCapturerCallback::OnJsCallbackStateChange(std::unique_ptr<AudioCapturerJsCallback> &jsCb)
{
    if (jsCb.get() == nullptr) {
        AUDIO_ERR_LOG("OnJsCallbackStateChange: OnJsCallbackRingerMode: jsCb.get() is null");
        return;
    }
    CHECK_AND_RETURN_LOG(mainHandler_ != nullptr, "mainHandler_ is nullptr");
    AudioCapturerJsCallback *event = jsCb.release();
    CHECK_AND_RETURN_LOG((event != nullptr) && (event->callback != nullptr),
        "OnJsCallbackStateChange: event is nullptr.");
    auto sharePtr = shared_from_this();
    auto task = [event, sharePtr]() {
        if (sharePtr != nullptr) {
            sharePtr->SafeJsCallbackStateChangeWork(event);
        }
    };
    mainHandler_->PostTask(task, "OnStateChange", 0, OHOS::AppExecFwk::EventQueue::Priority::IMMEDIATE, {});
}

void TaiheAudioCapturerCallback::SafeJsCallbackStateChangeWork(AudioCapturerJsCallback *event)
{
    CHECK_AND_RETURN_LOG((event != nullptr) && (event->callback != nullptr),
        "SafeJsCallbackStateChangeWork: no memory");
    std::shared_ptr<AudioCapturerJsCallback> safeContext(
        static_cast<AudioCapturerJsCallback*>(event),
        [](AudioCapturerJsCallback *ptr) {
            delete ptr;
    });
    std::string request = event->callbackName;

    do {
        std::shared_ptr<taihe::callback<void(AudioState)>> cacheCallback =
            std::reinterpret_pointer_cast<taihe::callback<void(AudioState)>>(event->callback->cb_);
        CHECK_AND_BREAK_LOG(cacheCallback != nullptr, "%{public}s get reference value fail", request.c_str());
        (*cacheCallback)(TaiheAudioEnum::ToTaiheAudioState(event->state));
    } while (0);
}
} // namespace ANI::Audio
