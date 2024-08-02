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

#include "audio_log.h"
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

int32_t EnhanceChainManagerCreateCb(const uint32_t sceneKeyCode, struct DeviceAttrAdapter adapter)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    AudioEnhanceDeviceAttr deviceAttr;
    deviceAttr.micRate = adapter.micRate;
    deviceAttr.micChannels = adapter.micChannels;
    deviceAttr.micFormat = adapter.micFormat;
    deviceAttr.needEc = adapter.needEc;
    deviceAttr.ecRate = adapter.ecRate;
    deviceAttr.ecChannels = adapter.ecChannels;
    deviceAttr.ecFormat = adapter.ecFormat;
    deviceAttr.needMicRef = adapter.needMicRef;
    deviceAttr.micRefRate = adapter.micRefRate;
    deviceAttr.micRefChannels = adapter.micRefChannels;
    deviceAttr.micRefFormat = adapter.micRefFormat;
    return audioEnhanceChainMananger->CreateAudioEnhanceChainDynamic(sceneKeyCode, deviceAttr);
}

int32_t EnhanceChainManagerReleaseCb(const uint32_t sceneKeyCode)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    if (audioEnhanceChainMananger->ReleaseAudioEnhanceChainDynamic(sceneKeyCode) != SUCCESS) {
        return ERROR;
    }
    return SUCCESS;
}

bool EnhanceChainManagerExist(const uint32_t sceneKeyCode)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    return audioEnhanceChainMananger->ExistAudioEnhanceChain(sceneKeyCode);
}

int32_t EnhanceChainManagerGetAlgoConfig(const uint32_t sceneKeyCode, pa_sample_spec *spec,
    bool *needEcFlag, bool *needMicRefFlag)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    AudioBufferConfig config = {};
    bool needEcFlagValue = false;
    bool needMicRefFlagValue = false;
    uint32_t ret = audioEnhanceChainMananger->AudioEnhanceChainGetAlgoConfig(sceneKeyCode, config,
        needEcFlagValue, needMicRefFlagValue);
    if (ret != 0 || config.samplingRate == 0) {
        return ERROR;
    }
    if (needEcFlag) {
        *needEcFlag = needEcFlagValue;
    }
    if (needMicRefFlag) {
        *needMicRefFlag = needMicRefFlagValue;
    }
    spec->rate = config.samplingRate;
    spec->channels = static_cast<uint8_t>(config.channels);

    auto item = FORMAT_CONVERT_MAP.find(config.format);
    if (item != FORMAT_CONVERT_MAP.end()) {
        spec->format = item->second;
    } else {
        spec->format = PA_SAMPLE_INVALID;
        return ERROR;
    }
    return SUCCESS;
}

bool EnhanceChainManagerIsEmptyEnhanceChain()
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        true, "null audioEnhanceChainManager");
    return audioEnhanceChainMananger->IsEmptyEnhanceChain();
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

int32_t CopyEcdataToEnhanceBufferAdapter(void *data, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    CHECK_AND_RETURN_RET_LOG(data != nullptr, ERROR, "data null");
    uint32_t ret = audioEnhanceChainMananger->CopyEcToEnhanceBuffer(data, length);
    if (ret != 0) {
        return ERROR;
    }
    return SUCCESS;
}

int32_t CopyMicRefdataToEnhanceBufferAdapter(void *data, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERROR, "null audioEnhanceChainManager");
    CHECK_AND_RETURN_RET_LOG(data != nullptr, ERROR, "data null");
    uint32_t ret = audioEnhanceChainMananger->CopyMicRefToEnhanceBuffer(data, length);
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

int32_t EnhanceChainManagerProcess(const uint32_t sceneKeyCode, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    if (audioEnhanceChainMananger->ApplyAudioEnhanceChain(sceneKeyCode, length) != SUCCESS) {
        AUDIO_ERR_LOG("%{public}u process failed", sceneKeyCode);
        return ERROR;
    }
    AUDIO_DEBUG_LOG("%{public}u process success", sceneKeyCode);
    return SUCCESS;
}

int32_t EnhanceChainManagerProcessDefault(const uint32_t captureId, uint32_t length)
{
    AudioEnhanceChainManager *audioEnhanceChainMananger = AudioEnhanceChainManager::GetInstance();
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChainMananger != nullptr,
        ERR_INVALID_HANDLE, "null audioEnhanceChainManager");
    if (audioEnhanceChainMananger->ApplyAudioEnhanceChainDefault(captureId, length) != SUCCESS) {
        AUDIO_ERR_LOG("%{public}u default process failed", captureId);
        return ERROR;
    }
    AUDIO_DEBUG_LOG("%{public}u default process success", captureId);
    return SUCCESS;
}

int32_t GetSceneTypeCode(const char *sceneType, uint32_t *sceneTypeCode)
{
    std::string sceneTypeString = "";
    if (sceneType) {
        sceneTypeString = sceneType;
    }
    for (auto &item : AUDIO_ENHANCE_SUPPORTED_SCENE_TYPES) {
        if (item.second == sceneTypeString) {
            *sceneTypeCode = static_cast<uint32_t>(item.first);
            return SUCCESS;
        }
    }
    return ERROR;
}