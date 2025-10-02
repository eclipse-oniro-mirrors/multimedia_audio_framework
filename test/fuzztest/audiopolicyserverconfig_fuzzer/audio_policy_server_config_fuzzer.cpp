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

#include <iostream>
#include <cstddef>
#include <cstdint>
#include "audio_policy_server.h"
#include "message_parcel.h"
#include "accesstoken_kit.h"
#include "nativetoken_kit.h"
#include "token_setproc.h"
#include "access_token.h"
#include "audio_info.h"
#include "audio_concurrency_parser.h"
using namespace std;

namespace OHOS {
namespace AudioStandard {
const int32_t LIMITSIZE = 4;

void AudioConcurrencyParserLoadConfigFuzzTest(const uint8_t* rawData, size_t size)
{
    if (rawData == nullptr || size < LIMITSIZE) {
        return;
    }
    std::map<std::pair<AudioPipeType, AudioPipeType>, ConcurrencyAction> concurrencyMap;
    concurrencyMap[std::make_pair(PIPE_TYPE_UNKNOWN, PIPE_TYPE_UNKNOWN)] = PLAY_BOTH;
    concurrencyMap[std::make_pair(PIPE_TYPE_SPATIALIZATION, PIPE_TYPE_SPATIALIZATION)] = CONCEDE_INCOMING;
    concurrencyMap[std::make_pair(PIPE_TYPE_NORMAL_OUT, PIPE_TYPE_NORMAL_OUT)] = CONCEDE_EXISTING;
    AudioConcurrencyParser audioConcurrencyParser;
    audioConcurrencyParser.LoadConfig(concurrencyMap);
}
} // namespace AudioStandard
} // namesapce OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioStandard::AudioConcurrencyParserLoadConfigFuzzTest(data, size);
    return 0;
}
