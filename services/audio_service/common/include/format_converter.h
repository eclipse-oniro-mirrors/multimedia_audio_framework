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

#ifndef FORMAT_CONVERTER_H
#define FORMAT_CONVERTER_H

#include "stdint.h"
#include "audio_info.h"

namespace OHOS {
namespace AudioStandard {
class FormatConverter {
public:
    static void DataAccumulationFromVolume(const std::vector<AudioStreamData> &srcDataList,
        const AudioStreamData &dstData);
    static int32_t S16MonoToS16Stereo(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t S16StereoToS16Mono(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t S16StereoToF32Stereo(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t S16StereoToF32Mono(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t F32MonoToS16Stereo(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t F32StereoToS16Stereo(const BufferDesc &srcDesc, const BufferDesc &dstDesc);
    static int32_t S32MonoToS16Mono(std::vector<char> &audioBuffer, std::vector<char> &audioBufferConverted);
    static int32_t S32StereoToS16Stereo(std::vector<char> &audioBuffer, std::vector<char> &audioBufferConverted);
};
} // namespace AudioStandard
} // namespace OHOS
#endif // FORMAT_CONVERTER_H
