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

#ifndef HDI_ADAPTER_MANAGER_API_H
#define HDI_ADAPTER_MANAGER_API_H

#include <inttypes.h>
#include "audio_hdiadapter_info.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HdiCaptureHandle {
    void *capture;
    int32_t (*Init)(void *capture);
    int32_t (*Deinit)(void *capture);
    int32_t (*Start)(void *capture);
    int32_t (*Stop)(void *capture);
    int32_t (*CaptureFrame)(void *capture,
        char *frame, uint64_t requestBytes, uint64_t *replyBytes);
    int32_t (*CaptureFrameWithEc)(void *capture,
        FrameDesc *fdesc, uint64_t *replyBytes,
        FrameDesc *fdescEc, uint64_t *replyBytesEc);
} HdiCaptureHandle;

int32_t CreateCaptureHandle(HdiCaptureHandle **handle, CaptureAttr *attr);

int32_t ReleaseCaptureHandle(HdiCaptureHandle *handle);

#ifdef __cplusplus
}
#endif
#endif // HDI_ADAPTER_MANAGER_API_H