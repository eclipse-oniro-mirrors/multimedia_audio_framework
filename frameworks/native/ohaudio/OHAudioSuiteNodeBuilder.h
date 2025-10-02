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

#ifndef OH_SUITE_BUILDER_H
#define OH_SUITE_BUILDER_H

#include <mutex>
#include <cstdint>
#include <unordered_map>
#include "native_audio_suite_engine.h"
#include "audio_suite_manager.h"

namespace OHOS {
namespace AudioStandard {

using OHOS::AudioStandard::AudioSuite::AudioFormat;
using OHOS::AudioStandard::AudioSuite::AudioNodeType;


class OHAudioSuiteNodeBuilder {
public:
    explicit OHAudioSuiteNodeBuilder(const OH_AudioNode_Type type);
    ~OHAudioSuiteNodeBuilder();

    OH_AudioSuite_Result SetFormat(OH_AudioFormat audioFormat);
    OH_AudioSuite_Result SetOnWriteDataCallback(OH_AudioNode_OnWriteDataCallBack callback, void *userData);

    AudioNodeType GetNodeType() const;
    bool IsSetFormat() const;
    bool IsSetWriteDataCallBack() const;
    AudioFormat GetNodeFormat() const;
    OH_AudioNode_OnWriteDataCallBack GetOnWriteDataCallBack() const;
    void *GetOnWriteUserData() const;

private:
    bool CheckSamplingRateVaild(int32_t samplingRate) const;
    bool CheckChannelCountVaild(int32_t channelCount) const;

    AudioNodeType nodeType_;
    AudioFormat nodeFormat_;
    bool setNodeFormat_ = false;

    OH_AudioNode_OnWriteDataCallBack onWriteDataCallBack_ = nullptr;
    void *onWriteDataUserData_ = nullptr;
};

} // namespace AudioStandard
} // namespace OHOS
#endif // OH_SUITE_BUILDER_H