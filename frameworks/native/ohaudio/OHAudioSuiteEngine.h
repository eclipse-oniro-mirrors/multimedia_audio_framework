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

#ifndef OH_AUDIO_SUITE_ENGINE_H
#define OH_AUDIO_SUITE_ENGINE_H

#include <mutex>
#include <cstdint>
#include <unordered_map>
#include "native_audio_suite_engine.h"
#include "audio_suite_manager.h"
#include "OHAudioSuiteNodeBuilder.h"

namespace OHOS {
namespace AudioStandard {

class OHSuiteInputNodeWriteDataCallBack : public AudioSuite::SuiteInputNodeWriteDataCallBack {
public:
    explicit OHSuiteInputNodeWriteDataCallBack(
        OH_AudioNode *audioNode, OH_AudioNode_OnWriteDataCallBack callback, void *data)
        : audioNode_(audioNode), callback_(callback), userData_(data) {}
    ~OHSuiteInputNodeWriteDataCallBack() = default;

    int32_t OnWriteDataCallBack(void *audioData, int32_t audioDataSize, bool *finished) override;

private:
    OH_AudioNode *audioNode_ = nullptr;
    OH_AudioNode_OnWriteDataCallBack callback_ = nullptr;
    void *userData_ = nullptr;
};

class OHOnReadTapDataCallback : public AudioSuite::SuiteNodeReadTapDataCallback {
public:
    explicit OHOnReadTapDataCallback(OH_AudioNode_OnReadTapDataCallback callback,
        OH_AudioNode *ohAudioNode, void *userData)
        : ohAudioNode_(ohAudioNode), callback_(callback), userData_(userData)
    {
    }
    ~OHOnReadTapDataCallback() = default;

    void OnReadTapDataCallback(void *audioData, int32_t audioDataSize) override;

private:
    OH_AudioNode *ohAudioNode_ = nullptr;
    OH_AudioNode_OnReadTapDataCallback callback_ = nullptr;
    void *userData_ = nullptr;
};

class OHAudioNode {
public:
    explicit OHAudioNode(uint32_t id, AudioSuite::AudioNodeType type) : nodeId_(id), type_(type) {};
    ~OHAudioNode() = default;

    uint32_t GetNodeId() const
    {
        return nodeId_;
    }

    AudioSuite::AudioNodeType GetNodeType() const
    {
        return type_;
    }

private:
    uint32_t nodeId_ = AudioSuite::INVALID_NODE_ID;
    AudioSuite::AudioNodeType type_;
};

class OHAudioSuitePipeline {
public:
    explicit OHAudioSuitePipeline(uint32_t id) : pipelineId_(id) {};
    ~OHAudioSuitePipeline() = default;

    uint32_t GetPipelineId() const
    {
        return pipelineId_;
    }

private:
    uint32_t pipelineId_ = AudioSuite::INVALID_PIPELINE_ID;
};

class OHAudioSuiteEngine {
public:
    ~OHAudioSuiteEngine() {};

    static OHAudioSuiteEngine *GetInstance();

    // engine
    int32_t CreateEngine();
    int32_t DestroyEngine();

    // pipeline
    int32_t CreatePipeline(OH_AudioSuitePipeline **audioSuitePipeline);
    int32_t DestroyPipeline(OHAudioSuitePipeline *audioPipeline);
    int32_t StartPipeline(OHAudioSuitePipeline *audioPipeline);
    int32_t StopPipeline(OHAudioSuitePipeline *audioPipeline);
    int32_t GetPipelineState(OHAudioSuitePipeline *audioPipeline, OH_AudioSuite_PipelineState *state);
    int32_t RenderFrame(OHAudioSuitePipeline *audioPipeline,
        uint8_t *audioData, int32_t frameSize, int32_t *writeSize, bool *finishedFlag);

    // node
    int32_t CreateNode(
        OHAudioSuitePipeline *audioSuitePipeline, OHAudioSuiteNodeBuilder *builder, OH_AudioNode **audioNode);
    int32_t DestroyNode(OHAudioNode *node);
    int32_t GetNodeEnableStatus(OHAudioNode *node, OH_AudioNodeEnable *enable);
    int32_t EnableNode(OHAudioNode *node, OH_AudioNodeEnable enable);
    int32_t SetAudioFormat(OHAudioNode *node, OH_AudioFormat *audioFormat);
    int32_t ConnectNodes(OHAudioNode *srcNode, OHAudioNode *destNode,
        OH_AudioNode_Port_Type sourcePortType, OH_AudioNode_Port_Type destPortType);
    int32_t DisConnectNodes(OHAudioNode *srcNode, OHAudioNode *destNode);
    int32_t SetEqualizerMode(OHAudioNode *node, OH_EqualizerMode eqMode);
    int32_t SetEqualizerFrequencyBandGains(
        OHAudioNode *node, OH_EqualizerFrequencyBandGains frequencyBandGains);
    int32_t SetSoundFieldType(OHAudioNode *node, OH_SoundFieldType soundFieldType);
    int32_t SetEnvironmentType(OHAudioNode *node, OH_EnvironmentType environmentType);
    int32_t SetVoiceBeautifierType(
        OHAudioNode *node, OH_VoiceBeautifierType voiceBeautifierType);
    int32_t InstallTap(OHAudioNode *node,
        OH_AudioNode_Port_Type portType, OH_AudioNode_OnReadTapDataCallback callback, void *userData);
    int32_t RemoveTap(OHAudioNode *node, OH_AudioNode_Port_Type portType);

private:
    explicit OHAudioSuiteEngine() {};
    OHAudioSuiteEngine(const OHAudioSuiteEngine&) = delete;
    OHAudioSuiteEngine& operator=(const OHAudioSuiteEngine&) = delete;
};

} // namespace AudioStandard
} // namespace OHOS
#endif // OH_AUDIO_SUITE_ENGINE_H