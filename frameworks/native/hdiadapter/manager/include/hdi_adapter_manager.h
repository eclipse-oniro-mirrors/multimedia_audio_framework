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

#ifndef HDI_ADAPTER_MANAGER_H
#define HDI_ADAPTER_MANAGER_H

#include <cinttypes>
#include <string>

#include "i_audio_capturer_source.h"

namespace OHOS {
namespace AudioStandard {

class HdiAdapterManager {
public:
    static HdiAdapterManager *GetInstance();

    IAudioCapturerSource *CreateCapture(CaptureAttr *attr);

    int32_t ReleaseCapture(IAudioCapturerSource *capture);

private:
    HdiAdapterManager();
    virtual ~HdiAdapterManager();

};

} // namespace AudioStandard
} // namespace OHOS
#endif // HDI_ADAPTER_MANAGER_H