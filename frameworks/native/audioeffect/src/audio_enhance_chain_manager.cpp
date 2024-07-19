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
#define LOG_TAG "AudioEnhanceChainManager"

#include "audio_enhance_chain_manager.h"

#include "securec.h"
#include "audio_log.h"
#include "audio_errors.h"
#include "audio_effect.h"
#include "audio_enhance_chain.h"
#include "audio_enhance_chain_adapter.h"

using namespace OHOS::AudioStandard;

namespace OHOS {
namespace AudioStandard {

static int32_t FindEnhanceLib(const std::string &enhance,
    const std::vector<std::shared_ptr<AudioEffectLibEntry>> &enhanceLibraryList,
    std::shared_ptr<AudioEffectLibEntry> &libEntry, std::string &libName)
{
    for (const std::shared_ptr<AudioEffectLibEntry> &lib : enhanceLibraryList) {
        if (lib->libraryName == enhance) {
            libName = lib->libraryName;
            libEntry = lib;
            return SUCCESS;
        }
    }
    return ERROR;
}

static int32_t CheckValidEnhanceLibEntry(const std::shared_ptr<AudioEffectLibEntry> &libEntry,
    const std::string &enhance, const std::string &libName)
{
    CHECK_AND_RETURN_RET_LOG(libEntry, ERROR, "Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle, ERROR,
        "AudioEffectLibHandle of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle->createEffect, ERROR,
        "CreateEffect function of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    CHECK_AND_RETURN_RET_LOG(libEntry->audioEffectLibHandle->releaseEffect, ERROR,
        "ReleaseEffect function of Enhance [%{public}s] in lib [%{public}s] is nullptr",
        enhance.c_str(), libName.c_str());
    return SUCCESS;
}

AudioEnhanceChainManager::AudioEnhanceChainManager()
{
    sceneTypeToEnhanceChainMap_.clear();
    sceneTypeToEnhanceChainCountMap_.clear();
    sceneTypeAndModeToEnhanceChainNameMap_.clear();
    enhanceChainToEnhancesMap_.clear();
    enhanceToLibraryEntryMap_.clear();
    enhanceToLibraryNameMap_.clear();
    enhanceBuffer_ = nullptr;
    isInitialized_ = false;
}

AudioEnhanceChainManager::~AudioEnhanceChainManager()
{
    AUDIO_INFO_LOG("~AudioEnhanceChainManager destroy");
}

AudioEnhanceChainManager *AudioEnhanceChainManager::GetInstance()
{
    static AudioEnhanceChainManager audioEnhanceChainManager;
    return &audioEnhanceChainManager;
}

void AudioEnhanceChainManager::InitAudioEnhanceChainManager(std::vector<EffectChain> &enhanceChains,
    const EffectChainManagerParam &managerParam, std::vector<std::shared_ptr<AudioEffectLibEntry>> &enhanceLibraryList)
{
    const std::unordered_map<std::string, std::string> &enhanceChainNameMap = managerParam.sceneTypeToChainNameMap;
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    std::set<std::string> enhanceSet;
    for (EffectChain enhanceChain : enhanceChains) {
        for (std::string enhance : enhanceChain.apply) {
            enhanceSet.insert(enhance);
        }
    }
    // Construct enhanceToLibraryEntryMap_ that stores libEntry for each effect name
    std::shared_ptr<AudioEffectLibEntry> libEntry = nullptr;
    std::string libName;
    for (std::string enhance : enhanceSet) {
        int32_t ret = FindEnhanceLib(enhance, enhanceLibraryList, libEntry, libName);
        CHECK_AND_CONTINUE_LOG(ret != ERROR, "Couldn't find libEntry of effect %{public}s", enhance.c_str());
        ret = CheckValidEnhanceLibEntry(libEntry, enhance, libName);
        enhanceToLibraryEntryMap_[enhance] = libEntry;
        enhanceToLibraryNameMap_[enhance] = libName;
    }
    // Construct enhanceChainToEnhancesMap_ that stores all effect names of each effect chain
    for (EffectChain enhanceChain : enhanceChains) {
        std::string key = enhanceChain.name;
        std::vector<std::string> enhances;
        for (std::string enhanceName : enhanceChain.apply) {
            enhances.emplace_back(enhanceName);
        }
        enhanceChainToEnhancesMap_[key] = enhances;
    }
    // Construct sceneTypeAndModeToEnhanceChainNameMap_ that stores effectMode associated with the effectChainName
    for (auto item = enhanceChainNameMap.begin(); item != enhanceChainNameMap.end(); item++) {
        sceneTypeAndModeToEnhanceChainNameMap_[item->first] = item->second;
    }
    // Construct effectPropertyMap_ that stores effect's property
    effectPropertyMap_ = managerParam.effectDefaultProperty;

    AUDIO_INFO_LOG("enhanceToLibraryEntryMap_ size %{public}zu \
        enhanceToLibraryNameMap_ size %{public}zu \
        sceneTypeAndModeToEnhanceChainNameMap_ size %{public}zu",
        enhanceToLibraryEntryMap_.size(),
        enhanceChainToEnhancesMap_.size(),
        sceneTypeAndModeToEnhanceChainNameMap_.size());
    isInitialized_ = true;
}

int32_t AudioEnhanceChainManager::InitEnhanceBuffer()
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    uint32_t len = 0;
    uint32_t lenEc = 0;
    uint32_t tempLen = 0;
    uint32_t tempLenEc = 0;
    // get max buffer length of ecBuffer and micBufferIn
    for (auto &item : sceneTypeToEnhanceChainMap_) {
        tempLen = item.second->GetAlgoBufferSize();
        tempLenEc = item.second->GetAlgoBufferSizeEc();
        if (tempLen > len) {
            len = tempLen;
        }
        if (tempLenEc > lenEc) {
            lenEc = tempLenEc;
        }
    }
    if (enhanceBuffer_ == nullptr) {
        AUDIO_DEBUG_LOG("len:%{public}u lenEc:%{public}u", len, lenEc);
        enhanceBuffer_ = std::make_unique<EnhanceBuffer>();
        enhanceBuffer_->ecBuffer.resize(lenEc);
        enhanceBuffer_->micBufferIn.resize(len);
        enhanceBuffer_->micBufferOut.resize(len);
        enhanceBuffer_->length = len;
        enhanceBuffer_->lengthEc = lenEc;
        AUDIO_INFO_LOG("enhanceBuffer_ init SUCCESS");
        return SUCCESS;
    }
    if ((len > enhanceBuffer_->length)) {
        enhanceBuffer_->micBufferIn.resize(len);
        enhanceBuffer_->micBufferOut.resize(len);
    }
    if (lenEc > enhanceBuffer_->lengthEc) {
        enhanceBuffer_->ecBuffer.resize(lenEc);
    }
    AUDIO_INFO_LOG("enhanceBuffer_ update SUCCESS");
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::CreateAudioEnhanceChainDynamic(const std::string &scene, const std::string &mode,
    const std::string &up, const std::string &down)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    std::string sceneTypeAndDeviceKey = scene + "_&_" + up + "_&_" + down;
    std::shared_ptr<AudioEnhanceChain> audioEnhanceChain = nullptr;
    if (sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey)) {
        if ((!sceneTypeToEnhanceChainCountMap_.count(sceneTypeAndDeviceKey)) ||
            (sceneTypeToEnhanceChainCountMap_[sceneTypeAndDeviceKey] < 1)) {
            AUDIO_ERR_LOG("sceneTypeToEnhanceChainCountMap_ has wrong data with %{public}s",
                sceneTypeAndDeviceKey.c_str());
            sceneTypeToEnhanceChainCountMap_.erase(sceneTypeAndDeviceKey);
            sceneTypeToEnhanceChainMap_.erase(sceneTypeAndDeviceKey);
            return ERROR;
        }
        sceneTypeToEnhanceChainCountMap_[sceneTypeAndDeviceKey]++;
        audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneTypeAndDeviceKey];
        if (audioEnhanceChain->IsEmptyEnhanceHandles()) {
            return ERROR;
        }
        return SUCCESS;
    } else {
        audioEnhanceChain = std::make_shared<AudioEnhanceChain>(scene, mode);
        if (audioEnhanceChain == nullptr) {
            AUDIO_ERR_LOG("AudioEnhanceChain construct failed.");
            return ERROR;
        }
        sceneTypeToEnhanceChainMap_.insert(std::make_pair(sceneTypeAndDeviceKey, audioEnhanceChain));
        if (!sceneTypeToEnhanceChainCountMap_.count(sceneTypeAndDeviceKey)) {
            sceneTypeToEnhanceChainCountMap_.insert(std::make_pair(sceneTypeAndDeviceKey, 1));
        } else {
            AUDIO_ERR_LOG("sceneTypeToEnhanceChainCountMap_ has wrong data with %{public}s",
                sceneTypeAndDeviceKey.c_str());
            sceneTypeToEnhanceChainCountMap_[sceneTypeAndDeviceKey] = 1;
        }
    }
    if (SetAudioEnhanceChainDynamic(scene, mode, up, down) != SUCCESS) {
        AUDIO_ERR_LOG("Create AudioEnhanceChain failed.");
        return ERROR;
    }
    AUDIO_INFO_LOG("%{public}s create success", sceneTypeAndDeviceKey.c_str());
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetAudioEnhanceChainDynamic(const std::string &sceneType,
    const std::string &sceneMode, const std::string &upDevice, const std::string &downDevice)
{
    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upDevice + "_&_" + downDevice;
    CHECK_AND_RETURN_RET_LOG(sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey), ERROR,
        "SceneType [%{public}s] does not exist, fail to set.", sceneTypeAndDeviceKey.c_str());
    
    std::shared_ptr<AudioEnhanceChain> audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneTypeAndDeviceKey];

    std::string enhanceChain;
    std::string enhanceChainKey = sceneType + "_&_" + sceneMode;
    std::string enhanceNone = AUDIO_ENHANCE_SUPPORTED_SCENE_MODES.find(ENHANCE_NONE)->second;
    if (!sceneTypeAndModeToEnhanceChainNameMap_.count(enhanceChainKey)) {
        AUDIO_ERR_LOG("EnhanceChain key [%{public}s] does not exist, auto set to %{public}s",
            enhanceChainKey.c_str(), enhanceNone.c_str());
        enhanceChain = enhanceNone;
    } else {
        enhanceChain = sceneTypeAndModeToEnhanceChainNameMap_[enhanceChainKey];
    }

    if (enhanceChain != enhanceNone && !enhanceChainToEnhancesMap_.count(enhanceChain)) {
        AUDIO_ERR_LOG("EnhanceChain name [%{public}s] does not exist, auto set to %{public}s",
            enhanceChain.c_str(), enhanceNone.c_str());
            enhanceChain = enhanceNone;
    }

    for (std::string enhance : enhanceChainToEnhancesMap_[enhanceChain]) {
        AudioEffectHandle handle = nullptr;
        AudioEffectDescriptor descriptor;
        descriptor.libraryName = enhanceToLibraryNameMap_[enhance];
        descriptor.effectName = enhance;

        AUDIO_INFO_LOG("libraryName: %{public}s effectName:%{public}s",
            descriptor.libraryName.c_str(), descriptor.effectName.c_str());
        int32_t ret = enhanceToLibraryEntryMap_[enhance]->audioEffectLibHandle->createEffect(descriptor, &handle);
        CHECK_AND_CONTINUE_LOG(ret == 0, "EnhanceToLibraryEntryMap[%{public}s] createEffect fail",
            enhance.c_str());
        audioEnhanceChain->AddEnhanceHandle(handle, enhanceToLibraryEntryMap_[enhance]->audioEffectLibHandle);
    }

    if (audioEnhanceChain->IsEmptyEnhanceHandles()) {
        AUDIO_ERR_LOG("EnhanceChain is empty, copy bufIn to bufOut like EFFECT_NONE mode");
        return ERROR;
    }
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::FreeEnhanceBuffer()
{
    if (enhanceBuffer_ != nullptr) {
        std::vector<uint8_t>().swap(enhanceBuffer_->ecBuffer);
        std::vector<uint8_t>().swap(enhanceBuffer_->micBufferIn);
        std::vector<uint8_t>().swap(enhanceBuffer_->micBufferOut);
        enhanceBuffer_->length = 0;
        enhanceBuffer_->lengthEc = 0;
        enhanceBuffer_ = nullptr;
        AUDIO_INFO_LOG("release EnhanceBuffer success");
    }
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::ReleaseAudioEnhanceChainDynamic(const std::string &sceneType,
    const std::string &upDevice, const std::string &downDevice)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    CHECK_AND_RETURN_RET_LOG(isInitialized_, ERROR, "has not been initialized");
    CHECK_AND_RETURN_RET_LOG(sceneType != "", ERROR, "null sceneType");

    std::string sceneTypeAndDeviceKey = sceneType + "_&_" + upDevice + "_&_" + downDevice;
    if (!sceneTypeToEnhanceChainMap_.count(sceneTypeAndDeviceKey)) {
        sceneTypeToEnhanceChainCountMap_.erase(sceneTypeAndDeviceKey);
        return SUCCESS;
    } else if (sceneTypeToEnhanceChainCountMap_.count(sceneTypeAndDeviceKey) &&
        sceneTypeToEnhanceChainCountMap_[sceneTypeAndDeviceKey] > 1) {
        sceneTypeToEnhanceChainCountMap_[sceneTypeAndDeviceKey]--;
        return SUCCESS;
    }
    sceneTypeToEnhanceChainCountMap_.erase(sceneTypeAndDeviceKey);
    sceneTypeToEnhanceChainMap_.erase(sceneTypeAndDeviceKey);
    AUDIO_INFO_LOG("release %{public}s", sceneTypeAndDeviceKey.c_str());
    if (sceneTypeToEnhanceChainMap_.size() == 0) {
        FreeEnhanceBuffer();
    }
    return SUCCESS;
}

bool AudioEnhanceChainManager::ExistAudioEnhanceChain(const std::string &sceneKey)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    CHECK_AND_RETURN_RET_LOG(isInitialized_, false, "has not been initialized");
    CHECK_AND_RETURN_RET_LOG(sceneKey != "", false, "null sceneKey");

    if (!sceneTypeToEnhanceChainMap_.count(sceneKey)) {
        return false;
    }
    auto audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneKey];
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChain != nullptr, false, "null sceneTypeToEnhanceChainMap_[%{public}s]",
        sceneKey.c_str());
    return !audioEnhanceChain->IsEmptyEnhanceHandles();
}

int32_t AudioEnhanceChainManager::AudioEnhanceChainGetAlgoConfig(const std::string &sceneKey, AudioBufferConfig &config)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    CHECK_AND_RETURN_RET_LOG(isInitialized_, ERROR, "has not been initialized");
    if (!sceneTypeToEnhanceChainMap_.count(sceneKey)) {
        AUDIO_ERR_LOG("sceneTypeToEnhanceChainMap_ have not %{public}s", sceneKey.c_str());
        return ERROR;
    }
    auto audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneKey];
    CHECK_AND_RETURN_RET_LOG(audioEnhanceChain != nullptr, ERROR, "[%{public}s] get config faild",
        sceneKey.c_str());
    audioEnhanceChain->GetAlgoConfig(config);
    return SUCCESS;
}

bool AudioEnhanceChainManager::IsEmptyEnhanceChain()
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    CHECK_AND_RETURN_RET_LOG(isInitialized_, ERROR, "has not been initialized");
    if (sceneTypeToEnhanceChainMap_.size() == 0) {
        return true;
    }
    bool ret = true;
    for (auto &item : sceneTypeToEnhanceChainMap_) {
        if (!item.second->IsEmptyEnhanceHandles()) {
            ret = false;
        }
    }
    return ret;
}

int32_t AudioEnhanceChainManager::CopyToEnhanceBuffer(void *data, uint32_t length)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    if (enhanceBuffer_ == nullptr) {
        return ERROR;
    }
    AUDIO_DEBUG_LOG("length: %{public}u chunk length: %{public}u", length, enhanceBuffer_->length);
    CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer_->micBufferIn.data(), enhanceBuffer_->length, data, length) == 0,
        ERROR, "memcpy error in data to enhanceBuffer->micBufferIn");
    memset_s(enhanceBuffer_->ecBuffer.data(), enhanceBuffer_->lengthEc, 0, enhanceBuffer_->lengthEc);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::CopyFromEnhanceBuffer(void *data, uint32_t length)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    if (enhanceBuffer_ == nullptr) {
        return ERROR;
    }
    if (length > enhanceBuffer_->length) {
        return ERROR;
    }
    CHECK_AND_RETURN_RET_LOG(memcpy_s(data, length, enhanceBuffer_->micBufferOut.data(), length) == 0,
        ERROR, "memcpy error in micBufferOut to data");
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::ApplyAudioEnhanceChain(const std::string &sceneKey, uint32_t length)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    if (!sceneTypeToEnhanceChainMap_.count(sceneKey)) {
        CHECK_AND_RETURN_RET_LOG(memcpy_s(enhanceBuffer_->micBufferOut.data(), enhanceBuffer_->length,
            enhanceBuffer_->micBufferIn.data(), length) == 0, ERROR, "memcpy error in apply enhance");
        AUDIO_ERR_LOG("Can not find %{public}s in sceneTypeToEnhanceChainMap_", sceneKey.c_str());
        return ERROR;
    }
    auto audioEnhanceChain = sceneTypeToEnhanceChainMap_[sceneKey];
    if (audioEnhanceChain->ApplyEnhanceChain(enhanceBuffer_, length) != SUCCESS) {
        AUDIO_ERR_LOG("Apply %{public}s failed.", sceneKey.c_str());
        return ERROR;
    }
    AUDIO_DEBUG_LOG("Apply %{public}s success", sceneKey.c_str());
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetInputDevice(const uint32_t &captureId, const DeviceType &inputDevice)
{
    captureId_ = captureId;
    inputDevice_ = inputDevice;
    AUDIO_INFO_LOG("success, captureId: %{public}d, inputDevice: %{public}d", captureId_, inputDevice_);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetOutputDevice(const uint32_t &renderId, const DeviceType &outputDevice)
{
    renderId_ = renderId;
    outputDevice_ = outputDevice;
    AUDIO_INFO_LOG("success, renderId: %{public}d, outputDevice: %{public}d", renderId_, outputDevice_);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetVolumeInfo(const AudioVolumeType &volumeType, const float &systemVol)
{
    volumeType_ = volumeType;
    systemVol_ = systemVol;
    AUDIO_INFO_LOG("success, volumeType: %{public}d, systemVol: %{public}f", volumeType_, systemVol_);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetMicrophoneMuteInfo(const bool &isMute)
{
    isMute_ = isMute;
    AUDIO_INFO_LOG("success, isMute: %{public}d", isMute_);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetStreamVolumeInfo(const uint32_t &sessionId, const float &streamVol)
{
    sessionId_ = sessionId;
    streamVol_ = streamVol;
    AUDIO_INFO_LOG("success, sessionId: %{public}d, streamVol: %{public}f", sessionId_, streamVol_);
    return SUCCESS;
}

int32_t AudioEnhanceChainManager::SetAudioEnhanceProperty(const AudioEnhancePropertyArray &propertyArray)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    int32_t ret = 0;
    for (const auto &property : propertyArray.property) {
        auto item = effectPropertyMap_.find(property.enhanceClass);
        if (item == effectPropertyMap_.end()) {
            effectPropertyMap_[property.enhanceClass] = property.enhanceProp;
        } else {
            if (item->second == property.enhanceProp) {
                AUDIO_INFO_LOG("No need to update effect %{public}s with mode %{public}s",
                    property.enhanceClass.c_str(), property.enhanceProp.c_str());
                continue;
            }
            item->second = property.enhanceProp;
        }
        for (const auto &[sceneType, enhanceChain] : sceneTypeToEnhanceChainMap_) {
            if (enhanceChain) {
                ret = enhanceChain->SetEnhanceProperty(property.enhanceClass, property.enhanceProp);
                CHECK_AND_RETURN_RET_LOG(ret == 0, ERR_OPERATION_FAILED, "set property failed");
            }
        }
    }
    return 0;
}

int32_t AudioEnhanceChainManager::GetAudioEnhanceProperty(AudioEnhancePropertyArray &propertyArray)
{
    std::lock_guard<std::mutex> lock(chainManagerMutex_);
    propertyArray.property.clear();
    for (const auto &[effect, prop] : effectPropertyMap_) {
        if (!prop.empty()) {
            propertyArray.property.emplace_back(AudioEnhanceProperty{effect, prop});
            AUDIO_INFO_LOG("effect %{public}s is now %{public}s mode",
                effect.c_str(), prop.c_str());
        }
    }
    return AUDIO_OK;
}
} // namespace AudioStandard
} // namespace OHOS
