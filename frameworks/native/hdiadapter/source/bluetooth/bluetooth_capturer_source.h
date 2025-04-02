/*
 * Copyright (c) 2024-2025 Huawei Device Co., Ltd.
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

#ifndef BLUETOOTH_CAPTURER_SOURCE_H
#define BLUETOOTH_CAPTURER_SOURCE_H

#include <cstdio>
#include "i_audio_capturer_source.h"

namespace OHOS {
namespace AudioStandard {

class BluetoothCapturerSource : public IAudioCapturerSource {
public:
    static BluetoothCapturerSource *GetInstance();

    BluetoothCapturerSource() = default;
    virtual ~BluetoothCapturerSource() = default;
};
} // namespace AudioStandard
} // namespace OHOS
#endif // BLUETOOTH_CAPTURER_SOURCE_H