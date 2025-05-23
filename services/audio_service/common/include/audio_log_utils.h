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

#ifndef AUDIO_LOG_UTILS_H
#define AUDIO_LOG_UTILS_H
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
static const size_t CHANNEL_MAX = 16; // same with CHANNEL_16

struct ChannelVolumes {
    AudioChannel channel = STEREO;
    int32_t volStart[CHANNEL_MAX];
    int32_t volEnd[CHANNEL_MAX];
};
class AudioLogUtils {
public:
    static void ProcessVolumeData(const std::string &logTag, const ChannelVolumes &vols, int64_t &count);

private:
    static void IncSilentData(const std::string &logTag, const ChannelVolumes &vols, int64_t count);
    static void IncSoundData(const std::string &logTag, const ChannelVolumes &vols, int64_t count);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // AUDIO_LOG_UTILS_H
