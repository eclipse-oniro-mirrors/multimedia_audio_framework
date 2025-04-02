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

#ifndef LOG_TAG
#define LOG_TAG "SinkAdapterManager"
#endif

#include "sink_adapter_manager.h"

#include <cstdint>
#include <cstddef>
#include <cinttypes>
#include <atomic>
#include <mutex>
#include "safe_map.h"

#include "i_audio_renderer_sink_intf.h"
#include "audio_errors.h"
#include "audio_hdi_log.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::AudioStandard;

namespace {
std::atomic<uint64_t> g_idCount = 0;
SafeMap<void*, uint64_t> g_adapterIdMap = {};
}

struct RendererSinkAdapter* CreateSinkAdapter(void)
{
    struct RendererSinkAdapter *adapter = (struct RendererSinkAdapter *)calloc(1, sizeof(*adapter));
    CHECK_AND_RETURN_RET_LOG(adapter != nullptr, nullptr, "alloc sink adapter failed");

    uint64_t adapterId = g_idCount.fetch_add(1, memory_order_relaxed);
    AUDIO_INFO_LOG("id: %{public}" PRIu64 "", adapterId);
    g_adapterIdMap.Insert(adapter, adapterId);
    return adapter;
}

void DestorySinkAdapter(struct RendererSinkAdapter *adapter)
{
    CHECK_AND_RETURN_LOG(adapter != nullptr, "Invalid parameter");
    uint64_t adapterId;
    if (g_adapterIdMap.Find(adapter, adapterId) == true) {
        AUDIO_INFO_LOG("id: %{public}" PRIu64 "", adapterId);
        g_adapterIdMap.Erase(adapter);
    } else {
        AUDIO_ERR_LOG("ERR");
    }
    free(adapter);
}
