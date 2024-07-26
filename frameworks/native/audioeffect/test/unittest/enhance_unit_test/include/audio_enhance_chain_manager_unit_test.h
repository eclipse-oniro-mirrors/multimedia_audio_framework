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

#ifndef AUDIO_ENHANCE_CHAIN_MANAGER_UNIT_TEST_H
#define AUDIO_ENHANCE_CHAIN_MANAGER_UNIT_TEST_H

#include <chrono>
#include <dlfcn.h>
#include <thread>
#include <fstream>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "audio_effect.h"
#include "audio_utils.h"
#include "audio_log.h"
#include "audio_enhance_chain_manager.h"
#include "audio_errors.h"

#define VALID_SCENEKEY_CODE 68864
#define INVALID_SCENEKEY_CODE 23333
#define VALID_BUFFER_SIZE 1000
#define INVALID_BUFFER_SIZE 10000
#define MAX_EXTRA_NUM 3

namespace OHOS {
namespace AudioStandard {
class AudioEnhanceChainManagerUnitTest : public testing::Test {
public:
    static AudioEnhanceChainManager* manager_;
    static std::vector<EffectChain> enhanceChains_;
    static EffectChainManagerParam managerParam_;
    static std::vector<std::shared_ptr<AudioEffectLibEntry>> enhanceLibraryList_;
    
    // SetUpTestSuite: Called before all test cases
    static void SetUpTestSuite(void);
    // TearDownTestSuite: Called after all test cases
    static void TearDownTestSuite(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};
}
}

#endif // AUDIO_ENHANCE_CHAIN_MANAGER_UNIT_TEST_H