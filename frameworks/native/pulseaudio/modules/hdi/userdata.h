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

#include "capturer_source_adapter.h"

#define DEFAULT_SCENE_BYPASS "scene.bypass"
#define MAX_SCENE_NAME_LEN 100

struct Userdata {
    pa_core *core;
    pa_module *module;
    pa_source *source;
    pa_thread *thread;
    pa_thread_mq thread_mq;
    pa_rtpoll *rtpoll;
    uint32_t buffer_size;
    uint32_t open_mic_speaker;
    pa_usec_t block_usec;
    pa_usec_t timestamp;
    SourceAttr attrs;
    bool IsCapturerStarted;
    struct CapturerSourceAdapter *sourceAdapter;
    pa_usec_t delayTime;
    pa_hashmap *sceneToCountMap;
};

#endif