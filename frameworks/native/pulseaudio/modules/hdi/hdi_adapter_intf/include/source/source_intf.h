/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef SOURCE_INTF_H
#define SOURCE_INTF_H

#include <stdio.h>
#include <stdint.h>
#include "intf_def.h"
#include "source/source_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

struct SourceAdapter *GetSourceAdapter(const char *deviceClass, const int32_t sourceType, const char *info);
void ReleaseSourceAdapter(struct SourceAdapter *sourceAdapter);
const char *GetSourceDeviceClass(uint32_t classType);

#ifdef __cplusplus
}
#endif
#endif // SOURCE_INTF_H
