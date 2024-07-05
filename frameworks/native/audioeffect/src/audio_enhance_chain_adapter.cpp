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

#undef LOG_TAG
#define LOG_TAG "AudioEnhanceChainAdapter"

#include "audio_enhance_chain_adapter.h"

#include <map>

#include "securec.h"
#include "audio_log.h"
#include "audio_effect.h"
#include "audio_errors.h"
#include "audio_enhance_chain_manager.h"

using namespace OHOS::AudioStandard;

constexpr int32_t SAMPLE_FORMAT_U8 = 8;
constexpr int32_t SAMPLE_FORMAT_S16LE = 16;
constexpr int32_t SAMPLE_FORMAT_S24LE = 24;
constexpr int32_t SAMPLE_FORMAT_S32LE = 32;

const std::map<int32_t, pa_sample_format_t> FORMAT_CONVERT_MAP {
    {SAMPLE_FORMAT_U8, PA_SAMPLE_U8},
    {SAMPLE_FORMAT_S16LE, PA_SAMPLE_S16LE},
    {SAMPLE_FORMAT_S24LE, PA_SAMPLE_S24LE},
    {SAMPLE_FORMAT_S32LE, PA_SAMPLE_S32LE},
};

int32_t EnhanceChainManagerCreateCb(const char *sceneType, const char *enhanceMode, const char *upDevice,
    const char *downDevice)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    std::string sceneModeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (enhanceMode) {
        sceneModeString = enhanceMode;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    if (audioEnhanceChainMananger->CreateAudioEnhanceChainDynamic(sceneTypeString, sceneModeString, upDeviceString,
        downDeviceString) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EnhanceChainManagerReleaseCb(const char *sceneType, const char *upDevice, const char *downDevice)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    if (audioEnhanceChainMananger->ReleaseAudioEnhanceChainDynamic(sceneTypeString, upDeviceString,
        downDeviceString) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

bool EnhanceChainManagerExist(const char *sceneKey)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneKeyString = "";
    if (sceneKey) {
        sceneKeyString = sceneKey;
    }
    return audioEnhanceChainMananger->ExistAudioEnhanceChain(sceneKeyString);
}

pa_sample_spec EnhanceChainManagerGetAlgoConfig(const char *sceneType, const char *upDevice, const char *downDevice)
{
    pa_sample_spec spec;
    pa_sample_spec_init(&spec);
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        spec, "null audioEnhanceChainManager");
    std::string sceneTypeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    AudioBufferConfig config = audioEnhanceChainMananger->AudioEnhanceChainGetAlgoConfig(sceneTypeString,
        upDeviceString, downDeviceString);
    if (config.samplingRate == 0) {
        return spec;
    }
    pa_sample_spec sampleSpec;
    sampleSpec.rate = config.samplingRate;
    sampleSpec.channels = static_cast<uint8_t>(config.channels);

    auto item = FORMAT_CONVERT_MAP.find(config.format);
    if (item != FORMAT_CONVERT_MAP.end()) {
        sampleSpec.format = item->second;
    } else {
        sampleSpec.format = PA_SAMPLE_INVALID;
    }
    return sampleSpec;
}

int32_t EnhanceChainManagerInitEnhanceBuffer()
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    if (audioEnhanceChainMananger->IsEmptyEnhanceChain()) {
        AUDIO_DEBUG_LOG("audioEnhanceChainMananger is empty EnhanceChain.");
        return ERROR;
    }
    return audioEnhanceChainMananger->InitEnhanceBuffer();
}

int32_t CopyToEnhanceBufferAdapter(void *data, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    CHECK_AND_RETURN_RET_LOG(data != nullptr, ERROR, "data null");
    uint32_t ret = audioEnhanceChainMananger->CopyToEnhanceBuffer(data, length);
    if (ret != 0) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t CopyFromEnhanceBufferAdapter(void *data, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    CHECK_AND_RETURN_RET_LOG(data != nullptr, ERROR, "data null");
    uint32_t ret = audioEnhanceChainMananger->CopyFromEnhanceBuffer(data, length);
    if (ret != 0) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t EnhanceChainManagerProcess(const char *sceneKey, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    std::string sceneKeyString = "";
    if (sceneKey) {
        sceneKeyString = sceneKey;
    }
    if (audioEnhanceChainMananger->ApplyAudioEnhanceChain(sceneKeyString, length) != SUCCESS) {
        AUDIO_ERR_LOG("%{public}s process failed", sceneKeyString.c_str());
        return ERROR;
    }
    AUDIO_DEBUG_LOG("%{public}s process success", sceneKeyString.c_str());
    return SUCCESS;
}

int32_t ConcatStr(const char *sceneType, const char *upDevice, const char *downDevice, char *sceneKey,
    uint32_t sceneKeyLen)
{
    std::string sceneTypeString = "";
    std::string upDeviceString = "";
    std::string downDeviceString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    if (upDevice) {
        upDeviceString = upDevice;
    }
    if (downDevice) {
        downDeviceString = downDevice;
    }
    std::string sceneKeyString = sceneTypeString + "_&_" + upDeviceString + "_&_" + downDeviceString + "\0";
    int32_t ret = memcpy_s(sceneKey, sceneKeyLen, sceneKeyString.c_str(), sceneKeyString.size());
    if (ret != 0) {
        AUDIO_ERR_LOG("memcpy from sceneKeyString to sceneKey failed");
        return ERROR;
    }
    return SUCCESS;
}