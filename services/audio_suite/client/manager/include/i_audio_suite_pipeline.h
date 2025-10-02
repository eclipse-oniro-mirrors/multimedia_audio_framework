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
#ifndef IAUDIO_SUITE_PIPELINE_H
#define IAUDIO_SUITE_PIPELINE_H

#include <string>
#include <memory>
#include <cstdint>
#include "audio_suite_node.h"

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {

class IAudioSuitePipeline {
public:
    virtual ~IAudioSuitePipeline() = default;

    virtual int32_t Init() = 0;
    virtual int32_t DeInit() = 0;
    virtual int32_t Start() = 0;
    virtual int32_t Stop() = 0;
    virtual int32_t GetPipelineState() = 0;

    virtual int32_t CreateNode(AudioNodeBuilder builder) = 0;
    virtual int32_t DestroyNode(uint32_t nodeId) = 0;
    virtual int32_t EnableNode(uint32_t nodeId, AudioNodeEnable audioNodeEnable) = 0;
    virtual int32_t GetNodeEnableStatus(uint32_t nodeId) = 0;
    virtual int32_t SetAudioFormat(uint32_t nodeId, AudioFormat audioFormat) = 0;
    virtual int32_t SetWriteDataCallback(uint32_t nodeId,
        std::shared_ptr<SuiteInputNodeWriteDataCallBack> callback) = 0;
    virtual int32_t ConnectNodes(uint32_t srcNodeId, uint32_t destNodeId,
        AudioNodePortType srcPortType, AudioNodePortType destPortType) = 0;
    virtual int32_t DisConnectNodes(uint32_t srcNodeId, uint32_t destNodeId) = 0;
    virtual int32_t InstallTap(uint32_t nodeId, AudioNodePortType portType,
        std::shared_ptr<SuiteNodeReadTapDataCallback> callback) = 0;
    virtual int32_t RemoveTap(uint32_t nodeId, AudioNodePortType portType) = 0;
    virtual int32_t RenderFrame(uint8_t *audioData, int32_t frameSize, int32_t *writeLen, bool *finishedFlag) = 0;
    virtual int32_t SetOptions(uint32_t nodeId, std::string name, std::string value) = 0;

    virtual uint32_t GetPipelineId() = 0;
};

}  // namespace AudioSuite
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // IAUDIO_SUITE_PIPELINE_H