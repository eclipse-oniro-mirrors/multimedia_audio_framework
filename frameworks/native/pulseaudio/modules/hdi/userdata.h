/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#ifndef USERDATA_H
#define USERDATA_H

#include <pulsecore/core.h>
#include <pulsecore/log.h>
#include <pulsecore/module.h>
#include <pulsecore/source.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/thread.h>
#include <pulsecore/hashmap.h>

#include "audio_hdiadapter_info.h"
#include "hdi_adapter_manager_api.h"
#include "capturer_source_adapter.h"

#define DEFAULT_SCENE_BYPASS "scene.bypass"
#define MAX_SCENE_NAME_LEN 100
#define SCENE_TYPE_OFFSET 16
#define CAPTURER_ID_OFFSET 8
#define BASE_TEN 10

struct Userdata {
    pa_core *core;
    pa_module *module;
    pa_source *source;
    pa_thread *thread;
    pa_thread_mq threadMq;
    pa_rtpoll *rtpoll;
    uint32_t bufferSize;
    uint32_t openMicSpeaker;
    pa_usec_t blockUsec;
    pa_usec_t timestamp;
    SourceAttr attrs;
    bool isCapturerStarted;
    EcType ecType;
    const char *ecAdapaterName;
    uint32_t ecSamplingRate;
    int32_t ecFormat;
    uint32_t ecChannels;
    MicRefSwitch micRef;
    uint32_t micRefRate;
    int32_t micRefFormat;
    uint32_t micRefChannels;
    struct CapturerSourceAdapter *sourceAdapter;
    pa_usec_t delayTime;
    pa_hashmap *sceneToCountMap;
    pa_hashmap *sceneToResamplerMap;
    HdiCaptureHandle *captureHandleEc;
    HdiCaptureHandle *captureHandleMicRef;
    uint64_t requestBytesEc;
    uint64_t requestBytesMicRef;
    void *bufferEc;
    void *bufferMicRef;
    uint32_t captureId;
    uint32_t renderId;
};

#endif // USERDATA_H