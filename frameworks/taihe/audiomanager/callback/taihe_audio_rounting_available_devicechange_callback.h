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

#ifndef TAIHE_AUDIO_ROUNTING_AVAILABLE_DEVICECHANGE_CALLBACK_H
#define TAIHE_AUDIO_ROUNTING_AVAILABLE_DEVICECHANGE_CALLBACK_H

#include "event_handler.h"
#include "taihe_work.h"
#include "audio_system_manager.h"

namespace ANI::Audio {
using namespace taihe;
using namespace ohos::multimedia::audio;
const std::string AVAILABLE_DEVICE_CHANGE_CALLBACK_NAME = "availableDeviceChange";

class TaiheAudioRountingAvailableDeviceChangeCallback :
    public OHOS::AudioStandard::AudioManagerAvailableDeviceChangeCallback,
    public std::enable_shared_from_this<TaiheAudioRountingAvailableDeviceChangeCallback> {
public:
    explicit TaiheAudioRountingAvailableDeviceChangeCallback();
    virtual ~TaiheAudioRountingAvailableDeviceChangeCallback();
    void SaveCallbackReference(const std::string &callbackName, std::shared_ptr<uintptr_t> callback);
    void OnAvailableDeviceChange(const OHOS::AudioStandard::AudioDeviceUsage usage,
        const OHOS::AudioStandard::DeviceChangeAction &deviceChangeAction) override;

    void SaveRoutingAvailbleDeviceChangeCbRef(OHOS::AudioStandard::AudioDeviceUsage usage,
        std::shared_ptr<uintptr_t> callback);
    void RemoveRoutingAvailbleDeviceChangeCbRef(std::shared_ptr<uintptr_t> callback);
    void RemoveAllRoutinAvailbleDeviceChangeCb();
    int32_t GetRoutingAvailbleDeviceChangeCbListSize();

private:
    struct AudioRountingJsCallback {
        std::shared_ptr<AutoRef> callback = nullptr;
        std::string callbackName = "unknown";
        OHOS::AudioStandard::DeviceChangeAction deviceChangeAction;
    };

    void OnJsCallbackAvailbleDeviceChange(std::unique_ptr<AudioRountingJsCallback> &jsCb);
    static void SafeJsCallbackAvailbleDeviceChangeWork(AudioRountingJsCallback *event);

    std::mutex mutex_;
    std::shared_ptr<AutoRef> deviceChangeCallback_ = nullptr;
    std::list<std::pair<std::shared_ptr<AutoRef>, OHOS::AudioStandard::AudioDeviceUsage>> availableDeviceChangeCbList_;
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> mainHandler_ = nullptr;
};
} // namespace ANI::Audio
#endif // TAIHE_AUDIO_ROUNTING_AVAILABLE_DEVICECHANGE_CALLBACK_H