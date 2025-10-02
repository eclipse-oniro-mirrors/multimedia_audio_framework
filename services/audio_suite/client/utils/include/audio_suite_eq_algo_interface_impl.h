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

#ifndef AUDIO_SUITE_EQ_ALGO_INTERFACE_IMPL_H
#define AUDIO_SUITE_EQ_ALGO_INTERFACE_IMPL_H
#define EQUALIZER_BANDS_NUM (10)
#define ALGO_CHANNEL_NUM (2)    // 算法声道数规格
#define ALGO_BYTE_NUM (2)       // 算法单帧字节数规格
#define ALGO_SAMPLE_WIDTH (16)  // 算法单帧位深规格
#define MASTERVOLUME (15)       // 算法音量设置
#define ONE_BYTE_WIDTH (8)
#define TWO_BYTES_WIDTH (16)

#include "audio_suite_algo_interface.h"
#include "imedia_api.h"
#include "audio_suite_log.h"
#include <utility>
#include <dlfcn.h>

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {

using FuniMedia_Eq_GetSize = int (*)(iMedia_SWS_MEM_SIZE *);
using FuniMedia_Eq_Init = int (*)(void *, void *, IMEDIA_INT32 iScratchBufLen, const iMedia_Eq_PARA *);
using FuniMedia_Eq_Apply = int (*)(void *, void *, IMEDIA_INT32 iScratchBufLen, iMedia_SWS_DATA *);
using FuniMedia_Eq_SetParams = int (*)(void *, void *, IMEDIA_INT32 iScratchBufLen, const iMedia_Eq_PARA *);
using FuniMedia_Eq_GetParams = int (*)(void *, iMedia_Eq_PARA *);

struct EqAlgoApi {
    FuniMedia_Eq_GetSize getSize{nullptr};
    FuniMedia_Eq_Init initAlgo{nullptr};
    FuniMedia_Eq_Apply applyAlgo{nullptr};
    FuniMedia_Eq_SetParams setPara{nullptr};
    FuniMedia_Eq_GetParams getPara{nullptr};
};

class AudioSuiteEqAlgoInterfaceImpl : public AudioSuiteAlgoInterface {
public:
    AudioSuiteEqAlgoInterfaceImpl();
    ~AudioSuiteEqAlgoInterfaceImpl();

    int32_t Init() override;
    int32_t Deinit() override;
    int32_t IsEqAlgoInit();
    int32_t SetParameter(const std::string &paramType, const std::string &paramValue) override;
    int32_t GetParameter(const std::string &paramType, std::string &paramValue) override;
    int32_t Apply(std::vector<uint8_t *> &v1, std::vector<uint8_t *> &v2) override;
    iMedia_Eq_PARA para = {0};
    iMedia_SWS_MEM_SIZE stSize;
    size_t frameLen;
    size_t frameBytes;

private:
    bool isEqAlgoInit_ = false;
    short changeFormat(int high, int low);
    std::vector<char> runBuf_;
    std::vector<char> scratchBuf_;

    void *libHandle_{nullptr};
    EqAlgoApi algoApi_{0};
    iMedia_SWS_DATA stData;
    std::vector<IMEDIA_INT32> dataIn;
    std::vector<IMEDIA_INT32> dataOut;
};

}  // namespace AudioSuite
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // AUDIO_SUITE_EQ_ALGO_INTERFACE_IMPL_H