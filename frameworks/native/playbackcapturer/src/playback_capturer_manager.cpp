/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
#define LOG_TAG "PlaybackCapturerManager"
#endif

#include "playback_capturer_manager.h"

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <vector>
#include <unordered_set>

#include "audio_common_log.h"
#include "audio_errors.h"
#include "playback_capturer_adapter.h"

using namespace OHOS::AudioStandard;

bool IsStreamSupportInnerCapturer(int32_t streamUsage)
{
    PlaybackCapturerManager *playbackCapturerMgr = PlaybackCapturerManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(playbackCapturerMgr != nullptr, false,
        "IsStreamSupportInnerCapturer return false for null manager.");

    return playbackCapturerMgr->IsStreamSupportInnerCapturer(streamUsage);
}

bool IsPrivacySupportInnerCapturer(int32_t privacyType)
{
    PlaybackCapturerManager *playbackCapturerMgr = PlaybackCapturerManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(playbackCapturerMgr != nullptr, false,
        "IsPrivacySupportInnerCapturer return false for null manager.");

    return playbackCapturerMgr->IsPrivacySupportInnerCapturer(privacyType);
}

bool IsCaptureSilently()
{
    PlaybackCapturerManager *playbackCapturerMgr = PlaybackCapturerManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(playbackCapturerMgr != nullptr, false,
        "IsCaptureSilently return false for null manager.");

    return playbackCapturerMgr->IsCaptureSilently();
}

extern "C" __attribute__((visibility("default"))) bool GetInnerCapturerState()
{
    PlaybackCapturerManager *playbackCapturerMgr = PlaybackCapturerManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(playbackCapturerMgr != nullptr, false,
        "IsCaptureSilently return false for null manager.");

    return playbackCapturerMgr->GetInnerCapturerState();
}

extern "C" __attribute__((visibility("default"))) void SetInnerCapturerState(bool state)
{
    PlaybackCapturerManager *playbackCapturerMgr = PlaybackCapturerManager::GetInstance();
    CHECK_AND_RETURN_LOG(playbackCapturerMgr != nullptr, "IsCaptureSilently return false for null manager.");

    playbackCapturerMgr->SetInnerCapturerState(state);
}

namespace OHOS {
namespace AudioStandard {

PlaybackCapturerManager::PlaybackCapturerManager() {}

PlaybackCapturerManager::~PlaybackCapturerManager() {}

PlaybackCapturerManager* PlaybackCapturerManager::GetInstance()
{
    static PlaybackCapturerManager playbackCapturerMgr;
    return &playbackCapturerMgr;
}

void PlaybackCapturerManager::SetSupportStreamUsage(std::vector<int32_t> usage)
{
    std::lock_guard<std::mutex> lock(setMutex_);
    supportStreamUsageSet_.clear();
    if (usage.empty()) {
        AUDIO_INFO_LOG("Clear support streamUsage");
        return;
    }
    for (size_t i = 0; i < usage.size(); i++) {
        supportStreamUsageSet_.emplace(usage[i]);
    }
}

bool PlaybackCapturerManager::IsStreamSupportInnerCapturer(int32_t streamUsage)
{
    std::lock_guard<std::mutex> lock(setMutex_);
    if (supportStreamUsageSet_.empty()) {
        return streamUsage == STREAM_USAGE_MEDIA || streamUsage == STREAM_USAGE_MUSIC ||
            streamUsage == STREAM_USAGE_MOVIE || streamUsage == STREAM_USAGE_GAME ||
            streamUsage == STREAM_USAGE_AUDIOBOOK;
    }
    return supportStreamUsageSet_.find(streamUsage) != supportStreamUsageSet_.end();
}

bool PlaybackCapturerManager::IsPrivacySupportInnerCapturer(int32_t privacyType)
{
    return privacyType == PRIVACY_TYPE_PUBLIC;
}

void PlaybackCapturerManager::SetCaptureSilentState(bool state)
{
    isCaptureSilently_ = state;
}

bool PlaybackCapturerManager::IsCaptureSilently()
{
    return isCaptureSilently_;
}

void PlaybackCapturerManager::SetInnerCapturerState(bool state)
{
    isInnerCapturerRunning_ = state;
}

bool PlaybackCapturerManager::GetInnerCapturerState()
{
    return isInnerCapturerRunning_;
}

std::vector<StreamUsage> PlaybackCapturerManager::GetDefaultUsages()
{
    return defaultUsages_;
}

bool PlaybackCapturerManager::RegisterCapturerFilterListener(ICapturerFilterListener *listener)
{
    if (listener == nullptr || listener_ != nullptr) {
        AUDIO_ERR_LOG("Register fail: listener is %{public}s", (listener == nullptr ? "null." : "already set."));
        return false;
    }
    AUDIO_INFO_LOG("Register success");
    listener_ = listener;
    return true;
}

int32_t PlaybackCapturerManager::SetPlaybackCapturerFilterInfo(uint32_t sessionId,
    const AudioPlaybackCaptureConfig &config, int32_t innerCapId)
{
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, ERR_ILLEGAL_STATE, "listener is null!");

    return listener_->OnCapturerFilterChange(sessionId, config, innerCapId);
}

int32_t PlaybackCapturerManager::RemovePlaybackCapturerFilterInfo(uint32_t sessionId, int32_t innerCapId)
{
    CHECK_AND_RETURN_RET_LOG(listener_ != nullptr, ERR_ILLEGAL_STATE, "listener is null!");
    return listener_->OnCapturerFilterRemove(sessionId, innerCapId);
}

int32_t PlaybackCapturerManager::CheckCaptureLimit(const AudioPlaybackCaptureConfig &config, int32_t &innerCapId)
{
    bool isSame = false;
    AudioPlaybackCaptureConfig newConfig = config;
    if (newConfig.filterOptions.usages.size() == 0) {
        std::vector<StreamUsage> defalutUsages = GetDefaultUsages();
        for (size_t i = 0; i < defalutUsages.size(); i++) {
            newConfig.filterOptions.usages.push_back(defalutUsages[i]);
        }
    }
    std::lock_guard<std::mutex> lock(filterMapMutex_);
    for (auto &filter : filters_) {
        if (filter.second.filterConfig == newConfig) {
            AUDIO_INFO_LOG("Capture num reuse innerId:%{public}d", filter.first);
            innerCapId = filter.first;
            filter.second.ref += 1;
            isSame = true;
            break;
        }
    }
    if (!isSame && filters_.size() >= innerCapLimit_) {
        AUDIO_ERR_LOG("Capture nume over limit");
        innerCapId = 0;
        return ERR_ADD_CAPTURE_OVER_LIMIT;
    }
    if (!isSame) {
        GetFilterIndex();
        innerCapId = static_cast<int32_t>(filterNowIndex_);
        AUDIO_INFO_LOG("Capture num add innerId:%{public}d", innerCapId);
        CaptureFilterRef captureFilter;
        captureFilter.ref = 1;
        captureFilter.filterConfig = newConfig;
        filters_[filterNowIndex_] = captureFilter;
    }
    return SUCCESS;
}

uint32_t PlaybackCapturerManager::GetFilterIndex()
{
    if (filterNowIndex_ >= innerCapLimit_) {
        filterNowIndex_ = 0;
    }
    return (++filterNowIndex_);
}

int32_t PlaybackCapturerManager::SetInnerCapLimit(uint32_t innerCapLimit)
{
    innerCapLimit_ = innerCapLimit;
    return SUCCESS;
}

bool PlaybackCapturerManager::CheckReleaseUnloadModernInnerCapSink(int32_t innerCapId)
{
    bool result = false;
    std::lock_guard<std::mutex> lock(filterMapMutex_);
    if (filters_.count(innerCapId)) {
        if (filters_[innerCapId].ref <= 1) {
            filters_.erase(innerCapId);
            result = true;
        } else {
            filters_[innerCapId].ref -= 1;
        }
    }
    return result;
}
} // namespace OHOS
} // namespace AudioStandard