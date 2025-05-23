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

#ifndef HPAE_RENDER_EFFECT_NODE_H
#define HPAE_RENDER_EFFECT_NODE_H

#include "hpae_plugin_node.h"
#ifdef ENABLE_HOOK_PCM
#include "hpae_pcm_dumper.h"
#endif
namespace OHOS {
namespace AudioStandard {
namespace HPAE {

enum ModifyAudioEffectChainInfoReason {
    ADD_AUDIO_EFFECT_CHAIN_INFO = 0,
    REMOVE_AUDIO_EFFECT_CHAIN_INFO = 1,
};

class HpaeRenderEffectNode : public HpaePluginNode {
public:
    HpaeRenderEffectNode(HpaeNodeInfo &nodeInfo);
    int32_t AudioRendererCreate(HpaeNodeInfo &nodeInfo);
    int32_t AudioRendererStart(HpaeNodeInfo &nodeInfo);
    int32_t AudioRendererStop(HpaeNodeInfo &nodeInfo);
    int32_t AudioRendererRelease(HpaeNodeInfo &nodeInfo);
    int32_t GetExpectedInputChannelInfo(uint32_t &channels, uint64_t &channelLayout);
protected:
    HpaePcmBuffer* SignalProcess(const std::vector<HpaePcmBuffer*> &inputs) override;
private:
    void ReconfigOutputBuffer();
    int32_t CreateAudioEffectChain(HpaeNodeInfo &nodeInfo);
    int32_t ReleaseAudioEffectChain(HpaeNodeInfo &nodeInfo);
    void ModifyAudioEffectChainInfo(HpaeNodeInfo &nodeInfo, ModifyAudioEffectChainInfoReason reason);
    void UpdateAudioEffectChainInfo(HpaeNodeInfo &nodeInfo);
    PcmBufferInfo pcmBufferInfo_;
    HpaePcmBuffer effectOutput_;
    HpaeNodeInfo nodeInfo_;
    std::string sceneType_ = "EFFECT_NONE";
#ifdef ENABLE_HOOK_PCM
    std::unique_ptr<HpaePcmDumper> inputPcmDumper_;
    std::unique_ptr<HpaePcmDumper> outputPcmDumper_;
#endif
};

} // namespace HPAE
} // namespace AudioStandard
} // namespace OHOS

#endif // HPAE_RENDER_EFFECT_NODE_H