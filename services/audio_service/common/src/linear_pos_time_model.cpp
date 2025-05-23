/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#define LOG_TAG "LinearPosTimeModel"
#endif

#include "linear_pos_time_model.h"

#include <cinttypes>

#include "audio_errors.h"
#include "audio_service_log.h"
#include "audio_utils.h"

namespace OHOS {
namespace AudioStandard {
namespace {
    static constexpr int64_t NANO_COUNT_PER_SECOND = 1000000000;
    static constexpr int32_t MAX_SUPPORT_SAMPLE_RETE = 384000;
    static constexpr int32_t MAX_STATISTICS_COUNT = 5;
    static constexpr int64_t REASONABLE_DELTA_BOUND_IN_NANO = 3000000; // 3ms
    static constexpr int64_t REASONABLE_BOUND_IN_NANO = 10000000; // 10ms
}
LinearPosTimeModel::LinearPosTimeModel()
{
    AUDIO_INFO_LOG("New LinearPosTimeModel");
}

bool LinearPosTimeModel::ConfigSampleRate(int32_t sampleRate)
{
    AUDIO_INFO_LOG("ConfigSampleRate:%{public}d", sampleRate);
    CHECK_AND_RETURN_RET_LOG(!isConfiged, false, "SampleRate already set:%{public}d", sampleRate_);
    sampleRate_ = sampleRate;
    if (sampleRate_ <= 0 || sampleRate_ > MAX_SUPPORT_SAMPLE_RETE) {
        AUDIO_ERR_LOG("Invalid sample rate!");
        return false;
    } else {
        nanoTimePerFrame_ = NANO_COUNT_PER_SECOND / sampleRate;
    }
    isConfiged = true;
    return true;
}

void LinearPosTimeModel::ResetFrameStamp(uint64_t frame, int64_t nanoTime)
{
    AUDIO_INFO_LOG("Reset frame:%{public}" PRIu64" with time:%{public}" PRId64".", frame, nanoTime);
    posTimeVec_.clear();
    stampFrame_ = frame;
    stampNanoTime_ = nanoTime;
    return;
}

bool LinearPosTimeModel::IsReasonable(uint64_t frame, int64_t nanoTime)
{
    if (frame == stampFrame_ && nanoTime == stampNanoTime_) {
        return true;
    }
    int64_t deltaFrame = 0;
    int64_t reasonableDeltaTime = 0;
    if (frame > stampFrame_) {
        deltaFrame = static_cast<int64_t>(frame - stampFrame_);
    } else {
        deltaFrame = -static_cast<int64_t>(stampFrame_ - frame);
    }
    reasonableDeltaTime = stampNanoTime_ + deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;

    // note: compare it with current time?
    if (nanoTime <= (reasonableDeltaTime + REASONABLE_BOUND_IN_NANO) &&
        nanoTime >= (reasonableDeltaTime - REASONABLE_BOUND_IN_NANO)) {
        return true;
    }
    return false;
}

CheckPosTimeRes LinearPosTimeModel::CheckReasonable(uint64_t frame, int64_t nanoTime)
{
    posTimeVec_.push_back(std::make_pair(frame, nanoTime));
    if (posTimeVec_.size() < MAX_STATISTICS_COUNT) {
        return CHECK_FAILED;
    }

    for (size_t i = 0; i < posTimeVec_.size() - 1; i++) {
        if (!CheckPosTimeReasonable(posTimeVec_[i], posTimeVec_[i + 1])) {
            Trace trace("LinearPosTimeModel::CheckReasonable Fail");
            AUDIO_WARNING_LOG("Unreasonable data pos: %{public}zu", i);
            posTimeVec_.clear();
            return CHECK_FAILED;
        }
    }

    // todo: add DFX
    Trace trace("LinearPosTimeModel::CheckReasonable Success");
    AUDIO_ERR_LOG("Updata new frame:%{public}" PRIu64" with time:%{public}" PRId64".", frame, nanoTime);
    return NEED_MODIFY;
}

bool LinearPosTimeModel::CheckPosTimeReasonable(std::pair<uint64_t, int64_t> &pre, std::pair<uint64_t, int64_t> &next)
{
    if (pre.first >= next.first) {
        return false;
    }
    int64_t deltaFrame = static_cast<int64_t>(next.first - pre.first);
    int64_t deltaFrameTime = deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;
    int64_t deltaTime = next.second - pre.second - deltaFrameTime;

    return std::abs(deltaTime) < REASONABLE_DELTA_BOUND_IN_NANO;
}

CheckPosTimeRes LinearPosTimeModel::UpdataFrameStamp(uint64_t frame, int64_t nanoTime)
{
    if (IsReasonable(frame, nanoTime)) {
        AUDIO_DEBUG_LOG("Updata frame:%{public}" PRIu64" with time:%{public}" PRId64".", frame, nanoTime);
        stampFrame_ = frame;
        stampNanoTime_ = nanoTime;
        posTimeVec_.clear();
        return CHECK_SUCCESS;
    }

    AUDIO_WARNING_LOG("Unreasonable pos-time[ %{public}" PRIu64" %{public}" PRId64"] "
        " stamp pos-time[ %{public}" PRIu64" %{public}" PRId64"].", frame, nanoTime, stampFrame_, stampNanoTime_);
    
    return CheckReasonable(frame, nanoTime);
}

bool LinearPosTimeModel::GetFrameStamp(uint64_t &frame, int64_t &nanoTime)
{
    CHECK_AND_RETURN_RET_LOG(isConfiged, false, "GetFrameStamp is not configed!");
    frame = stampFrame_;
    nanoTime = stampNanoTime_;
    return true;
}

void LinearPosTimeModel::SetSpanCount(uint64_t spanCountInFrame)
{
    AUDIO_INFO_LOG("New spanCountInFrame:%{public}" PRIu64".", spanCountInFrame);
    spanCountInFrame_ = spanCountInFrame;
    return;
}

int64_t LinearPosTimeModel::GetTimeOfPos(uint64_t posInFrame)
{
    int64_t deltaFrame = 0;
    int64_t invalidTime = -1;
    CHECK_AND_RETURN_RET_LOG(isConfiged, invalidTime, "SampleRate is not configed!");
    if (posInFrame >= stampFrame_) {
        if (posInFrame - stampFrame_ >= (uint64_t)sampleRate_) {
            AUDIO_WARNING_LOG("posInFrame %{public}" PRIu64" is too"
                " large, stampFrame: %{public}" PRIu64"", posInFrame, stampFrame_);
        }
        deltaFrame = static_cast<int64_t>(posInFrame - stampFrame_);
        return stampNanoTime_ + deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;
    } else {
        if (stampFrame_ - posInFrame >= (uint64_t)sampleRate_) {
            AUDIO_WARNING_LOG("posInFrame %{public}" PRIu64" is too"
                " small, stampFrame: %{public}" PRIu64"", posInFrame, stampFrame_);
        }
        deltaFrame = static_cast<int64_t>(stampFrame_ - posInFrame);
        return stampNanoTime_ - deltaFrame * NANO_COUNT_PER_SECOND / (int64_t)sampleRate_;
    }
    return invalidTime;
}
} // namespace AudioStandard
} // namespace OHOS

