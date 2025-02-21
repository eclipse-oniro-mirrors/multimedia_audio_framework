/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#ifndef BLUETOOTH_DEVICE_UTILS_H
#define BLUETOOTH_DEVICE_UTILS_H

#include "bluetooth_device.h"

namespace OHOS {
namespace Bluetooth {
enum BluetoothDeviceAction : int32_t {
    WEAR_ACTION = 0,
    UNWEAR_ACTION = 1,
    ENABLEFROMREMOTE_ACTION,
    DISABLEFROMREMOTE_ACTION,
    ENABLE_WEAR_DETECTION_ACTION,
    DISABLE_WEAR_DETECTION_ACTION,
    USER_SELECTION_ACTION,
    CONNECT_ACTION,
    DISCONNECT_ACTION,
};

enum DeviceStatus : int32_t {
    ADD = 0,
    REMOVE = 1,
};

enum EventType : int32_t {
    DEFAULT_SELECT = 0,
    USER_UNSELECT = 1,
    USER_SELECT = 2,
};

enum ScoCategory : int32_t {
    SCO_DEFAULT = -1,
    SCO_CALLULAR = 0,
    SCO_VIRTUAL = 1,
    SCO_RECOGNITION = 2,
};
} // namespace Bluetooth
} // namespace OHOS

#endif // BLUETOOTH_DEVICE_UTILS_H