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
#include <algorithm>
#include "audio_playback_engine.h"
#include "audio_errors.h"
#include "common/hdi_adapter_info.h"
namespace OHOS {
namespace AudioStandard {
AudioPlaybackEngine::AudioPlaybackEngine()
    : renderId_(HDI_INVALID_ID), playbackThread_(nullptr), streams_(0) {}

AudioPlaybackEngine::~AudioPlaybackEngine() {}

int32_t AudioPlaybackEngine::Init(const AudioDeviceDescriptor &type, bool isVoip)
{
    return SUCCESS;
}

int32_t AudioPlaybackEngine::AddRenderer(const std::shared_ptr<IRendererStream> &stream)
{
    auto it = std::find(streams_.begin(), streams_.end(), stream);
    if (it == streams_.end()) {
        streams_.emplace_back(stream);
    }
    return SUCCESS;
}

void AudioPlaybackEngine::RemoveRenderer(const std::shared_ptr<IRendererStream> &stream)
{
    auto it = std::find(streams_.begin(), streams_.end(), stream);
    if (it != streams_.end()) {
        streams_.erase(it);
    }
}

int32_t AudioPlaybackEngine::Start()
{
    return SUCCESS;
}

int32_t AudioPlaybackEngine::Stop()
{
    return SUCCESS;
}

int32_t AudioPlaybackEngine::Pause()
{
    return SUCCESS;
}

int32_t AudioPlaybackEngine::Flush()
{
    return SUCCESS;
}

bool AudioPlaybackEngine::IsPlaybackEngineRunning() const noexcept
{
    return false;
}

uint64_t AudioPlaybackEngine::GetLatency() noexcept
{
    return 0;
}
} // namespace AudioStandard
} // namespace OHOS
