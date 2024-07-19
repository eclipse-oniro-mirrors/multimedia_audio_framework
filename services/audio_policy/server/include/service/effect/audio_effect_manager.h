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

#ifndef ST_AUDIO_EFFECT_MANAGER_H
#define ST_AUDIO_EFFECT_MANAGER_H

#include "audio_log.h"
#include "audio_effect.h"
#include "audio_effect_config_parser.h"

namespace OHOS {
namespace AudioStandard {
class AudioEffectManager {
public:
    explicit AudioEffectManager();
    ~AudioEffectManager();
    static AudioEffectManager& GetAudioEffectManager()
    {
        static AudioEffectManager audioEffectManager;
        return audioEffectManager;
    }
    void EffectManagerInit();
    void GetOriginalEffectConfig(OriginalEffectConfig &oriEffectConfig);
    void GetAvailableEffects(std::vector<Effect> &availableEffects);
    void UpdateAvailableEffects(std::vector<Effect> &newAvailableEffects);
    void GetSupportedEffectConfig(SupportedEffectConfig &supportedEffectConfig);
    void BuildAvailableAEConfig();
    void SetMasterSinkAvailable();
    void SetEffectChainManagerAvailable();
    bool CanLoadEffectSinks();
    void ConstructEffectChainManagerParam(EffectChainManagerParam &effectChainMgrParam);
    void ConstructEnhanceChainManagerParam(EffectChainManagerParam &enhanceChainMgrParam);
    int32_t QueryEffectManagerSceneMode(SupportedEffectConfig &supportedEffectConfig);
    int32_t GetSupportedAudioEffectProperty(const DeviceType& deviceType,
        std::set<std::pair<std::string, std::string>> &mergedSet);
    int32_t GetSupportedAudioEnhanceProperty(const DeviceType& deviceType,
        std::set<std::pair<std::string, std::string>> &mergedSet);

private:
    OriginalEffectConfig oriEffectConfig_;
    std::vector<Effect> availableEffects_;
    SupportedEffectConfig supportedEffectConfig_;
    int32_t existDefault_ = 0;
    bool isMasterSinkAvailable_ = false;
    bool isEffectChainManagerAvailable_ = false;
    std::vector<std::string> postSceneTypeSet_;
    std::unordered_map<std::string, std::set<std::pair<std::string, std::string>>> device2EffectPropertySet_;
    std::unordered_map<std::string, std::set<std::pair<std::string, std::string>>> device2EnhancePropertySet_;
    std::unordered_map<std::string, std::string> effectDefaultProperty_;
    std::unordered_map<std::string, std::string> enhanceDefaultProperty_;

    void UpdateAvailableAEConfig(OriginalEffectConfig &aeConfig);
    void UpdateEffectChains(std::vector<std::string> &availableLayout);
    void UpdateDuplicateBypassMode(ProcessNew &processNew);
    void UpdateDuplicateMode(ProcessNew &processNew);
    void UpdateDuplicateDefaultScene(ProcessNew &processNew);
    void UpdateDuplicateScene(ProcessNew &processNew);
    void UpdateDuplicateDevice(ProcessNew &processNew);
    int32_t UpdateUnavailableEffectChains(std::vector<std::string> &availableLayout, ProcessNew &processNew);
    bool VerifySceneMappingItem(const SceneMappingItem &item);
    void UpdateSupportedEffectProperty(const Device &device,
        std::unordered_map<std::string, std::set<std::pair<std::string, std::string>>> &device2PropertySet);
    void UpdateDuplicateProcessNew(std::vector<std::string> &availableLayout, ProcessNew &processNew);
    void ConstructDefaultEffectProperty(const std::string &chainName,
        std::unordered_map<std::string, std::string> &defaultProperty);
    int32_t GetSupportedPropertyInner(const DeviceType& deviceType,
        std::set<std::pair<std::string, std::string>> &mergedSet,
        const std::unordered_map<std::string, std::set<std::pair<std::string, std::string>>> &device2PropertySet);
};
} // namespce AudioStandard
} // namespace OHOS
#endif // ST_AUDIO_EFFECT_MANAGER_H