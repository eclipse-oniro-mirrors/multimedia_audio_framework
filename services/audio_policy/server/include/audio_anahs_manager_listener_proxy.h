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

#ifndef AUDIO_ANAHS_MANAGER_LISTENER_PROXY_H
#define AUDIO_ANAHS_MANAGER_LISTENER_PROXY_H

#include "audio_anahs_manager.h"
#include "i_standard_audio_anahs_manager_listener.h"

namespace OHOS {
namespace AudioStandard {
class AudioAnahsManagerListenerProxy : public IRemoteProxy<IStandardAudioAnahsManagerListener> {
public:
    explicit AudioAnahsManagerListenerProxy(const sptr<IRemoteObject> &impl);
    virtual ~AudioAnahsManagerListenerProxy();
    int32_t OnExtPnpDeviceStatusChanged(std::string anahsStatus, std::string anahsShowType) override;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_RINGERMODE_UPDATE_LISTENER_PROXY_H
