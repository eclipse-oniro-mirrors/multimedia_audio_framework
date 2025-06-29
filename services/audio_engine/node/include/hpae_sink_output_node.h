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
#ifndef HPAE_SINK_OUTPUT_NODE_H
#define HPAE_SINK_OUTPUT_NODE_H
#include <memory>
#include "hpae_node.h"
#include "hpae_pcm_buffer.h"
#include "audio_info.h"
#include "sink/i_audio_render_sink.h"
#include "common/hdi_adapter_info.h"
#include "manager/hdi_adapter_manager.h"
#include "high_resolution_timer.h"
#ifdef ENABLE_HOOK_PCM
#include "high_resolution_timer.h"
#include "hpae_pcm_dumper.h"
#endif
namespace OHOS {
namespace AudioStandard {
namespace HPAE {
typedef void (*AppCallbackFunc)(void *pHndl);

class HpaeSinkOutputNode : public InputNode<HpaePcmBuffer *> {
public:
    HpaeSinkOutputNode(HpaeNodeInfo &nodeInfo);
    virtual void DoProcess() override;
    virtual bool Reset() override;
    virtual bool ResetAll() override;
    void Connect(const std::shared_ptr<OutputNode<HpaePcmBuffer *>> &preNode) override;
    void DisConnect(const std::shared_ptr<OutputNode<HpaePcmBuffer *>> &preNode) override;
    int32_t GetRenderSinkInstance(std::string deviceClass, std::string deviceNetId);
    int32_t RenderSinkInit(IAudioSinkAttr &attr);
    int32_t RenderSinkDeInit();
    int32_t RenderSinkFlush(void);
    int32_t RenderSinkPause(void);
    int32_t RenderSinkReset(void);
    int32_t RenderSinkResume(void);
    int32_t RenderSinkStart(void);
    int32_t RenderSinkStop(void);
    int32_t RenderSinkSetPriPaPower(void);
    size_t GetPreOutNum();
    // for ut test
    const char *GetRenderFrameData(void);
    StreamManagerState GetSinkState(void);
    int32_t SetSinkState(StreamManagerState sinkState);
    int32_t UpdateAppsUid(const std::vector<int32_t> &appsUid);
    uint32_t GetLatency();
private:
    void HandleRemoteTiming();
    void HandlePaPower(HpaePcmBuffer *pcmBuffer);
    InputPort<HpaePcmBuffer *> inputStream_;
    std::vector<char> renderFrameData_;
    std::vector<float> interleveData_;
    std::shared_ptr<IAudioRenderSink> audioRendererSink_ = nullptr;
    uint32_t renderId_ = HDI_INVALID_ID;
    IAudioSinkAttr sinkOutAttr_;
    StreamManagerState state_ = STREAM_MANAGER_NEW;
    TimePoint remoteTimePoint_;
    std::chrono::milliseconds remoteSleepTime_ = std::chrono::milliseconds(0);
    int64_t silenceDataUs_ = 0;
    bool isOpenPaPower_ = true;
    bool isDisplayPaPowerState_ = false;
    uint32_t latency_ = 0;
#ifdef ENABLE_HOOK_PCM
    HighResolutionTimer intervalTimer_;
    std::unique_ptr<HpaePcmDumper> outputPcmDumper_ = nullptr;
#endif
};

}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS

#endif