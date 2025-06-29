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

#include "hpae_node_common.h"
#include "audio_errors.h"
#include "audio_engine_log.h"
#include "cinttypes"

namespace OHOS {
namespace AudioStandard {
namespace HPAE {
static constexpr uint64_t TIME_US_PER_S = 1000000;
static constexpr uint32_t DEFAULT_MULTICHANNEL_NUM = 6;
static constexpr uint32_t DEFAULT_MULTICHANNEL_CHANNELLAYOUT = 1551;
static constexpr float MAX_SINK_VOLUME_LEVEL = 1.0;
static constexpr uint32_t DEFAULT_MULTICHANNEL_FRAME_LEN_MS = 20;
static constexpr uint32_t MS_PER_SECOND = 1000;

static std::map<AudioStreamType, HpaeProcessorType> g_streamTypeToSceneTypeMap = {
    {STREAM_MUSIC, HPAE_SCENE_MUSIC},
    {STREAM_GAME, HPAE_SCENE_GAME},
    {STREAM_MOVIE, HPAE_SCENE_MOVIE},
    {STREAM_GAME, HPAE_SCENE_GAME},
    {STREAM_SPEECH, HPAE_SCENE_SPEECH},
    {STREAM_VOICE_RING, HPAE_SCENE_RING},
    {STREAM_VOICE_COMMUNICATION, HPAE_SCENE_VOIP_DOWN},
    {STREAM_MEDIA, HPAE_SCENE_OTHERS}
};

static std::map<AudioEffectScene, HpaeProcessorType> g_effectSceneToProcessorTypeMap = {
    {SCENE_OTHERS, HPAE_SCENE_OTHERS},
    {SCENE_MUSIC, HPAE_SCENE_MUSIC},
    {SCENE_MOVIE, HPAE_SCENE_MOVIE},
    {SCENE_GAME, HPAE_SCENE_GAME},
    {SCENE_SPEECH, HPAE_SCENE_SPEECH},
    {SCENE_RING, HPAE_SCENE_RING},
    {SCENE_VOIP_DOWN, HPAE_SCENE_VOIP_DOWN},
    {SCENE_COLLABORATIVE, HPAE_SCENE_COLLABORATIVE}
};

static std::unordered_map<SourceType, HpaeProcessorType> g_sourceTypeToSceneTypeMap = {
    {SOURCE_TYPE_MIC, HPAE_SCENE_RECORD},
    {SOURCE_TYPE_CAMCORDER, HPAE_SCENE_RECORD},
    {SOURCE_TYPE_VOICE_CALL, HPAE_SCENE_VOIP_UP},
    {SOURCE_TYPE_VOICE_COMMUNICATION, HPAE_SCENE_VOIP_UP},
    {SOURCE_TYPE_VOICE_TRANSCRIPTION, HPAE_SCENE_PRE_ENHANCE},
    {SOURCE_TYPE_VOICE_MESSAGE, HPAE_SCENE_VOICE_MESSAGE}
};


static std::unordered_set<HpaeProcessorType> g_processorTypeNeedEcSet = {
    HPAE_SCENE_VOIP_UP,
    HPAE_SCENE_PRE_ENHANCE,
};

static std::unordered_set<HpaeProcessorType> g_processorTypeNeedMicRefSet = {
    HPAE_SCENE_VOIP_UP,
    HPAE_SCENE_RECORD,
};

static std::unordered_map<HpaeProcessorType, AudioEnhanceScene> g_processorTypeToSceneTypeMap = {
    {HPAE_SCENE_RECORD, SCENE_RECORD},
    {HPAE_SCENE_VOIP_UP, SCENE_VOIP_UP},
    {HPAE_SCENE_PRE_ENHANCE, SCENE_PRE_ENHANCE},
    {HPAE_SCENE_VOICE_MESSAGE, SCENE_VOICE_MESSAGE}
};

static std::unordered_map<HpaeSessionState, std::string> g_sessionStateToStrMap = {
    {HPAE_SESSION_NEW, "NEW"},
    {HPAE_SESSION_PREPARED, "PREPARED"},
    {HPAE_SESSION_RUNNING, "RUNNING"},
    {HPAE_SESSION_PAUSING, "PAUSING"},
    {HPAE_SESSION_PAUSED, "PAUSED"},
    {HPAE_SESSION_STOPPING, "STOPPING"},
    {HPAE_SESSION_STOPPED, "STOPPED"},
    {HPAE_SESSION_RELEASED, "RELEASED"}
};

static std::unordered_map<StreamManagerState, std::string> g_streamMgrStateToStrMap = {
    {STREAM_MANAGER_NEW, "NEW"},
    {STREAM_MANAGER_IDLE, "IDLE"},
    {STREAM_MANAGER_RUNNING, "RUNNING"},
    {STREAM_MANAGER_SUSPENDED, "SUSPENDED"},
    {STREAM_MANAGER_RELEASED, "RELEASED"}
};

static std::map<std::string, uint32_t> g_formatFromParserStrToEnum = {
    {"s16", SAMPLE_S16LE},
    {"s16le", SAMPLE_S16LE},
    {"s24", SAMPLE_S24LE},
    {"s24le", SAMPLE_S24LE},
    {"s32", SAMPLE_S32LE},
    {"s32le", SAMPLE_S32LE},
    {"f32", SAMPLE_F32LE},
    {"f32le", SAMPLE_F32LE},
};

static std::map<uint32_t, std::string> g_formatFromParserEnumToStr = {
    {SAMPLE_S16LE, "s16le"},
    {SAMPLE_S24LE, "s24le"},
    {SAMPLE_S32LE, "s32le"},
    {SAMPLE_F32LE, "f32le"},
};

std::string ConvertSessionState2Str(HpaeSessionState state)
{
    if (g_sessionStateToStrMap.find(state) == g_sessionStateToStrMap.end()) {
        return "UNKNOWN";
    }
    return g_sessionStateToStrMap[state];
}

std::string ConvertStreamManagerState2Str(StreamManagerState state)
{
    if (g_streamMgrStateToStrMap.find(state) == g_streamMgrStateToStrMap.end()) {
        return "UNKNOWN";
    }
    return g_streamMgrStateToStrMap[state];
}

HpaeProcessorType TransStreamTypeToSceneType(AudioStreamType streamType)
{
    if (g_streamTypeToSceneTypeMap.find(streamType) == g_streamTypeToSceneTypeMap.end()) {
        return HPAE_SCENE_EFFECT_NONE;
    } else {
        return g_streamTypeToSceneTypeMap[streamType];
    }
}

HpaeProcessorType TransEffectSceneToSceneType(AudioEffectScene effectScene)
{
    if (g_effectSceneToProcessorTypeMap.find(effectScene) == g_effectSceneToProcessorTypeMap.end()) {
        return HPAE_SCENE_EFFECT_NONE;
    } else {
        return g_effectSceneToProcessorTypeMap[effectScene];
    }
}

void TransNodeInfoForCollaboration(HpaeNodeInfo &nodeInfo, bool isCollaborationEnabled)
{
    if (isCollaborationEnabled) {
        if (nodeInfo.effectInfo.effectScene == SCENE_MUSIC || nodeInfo.effectInfo.effectScene == SCENE_MOVIE) {
            nodeInfo.effectInfo.lastEffectScene = nodeInfo.effectInfo.effectScene;
            nodeInfo.effectInfo.effectScene = SCENE_COLLABORATIVE;
            nodeInfo.sceneType = HPAE_SCENE_COLLABORATIVE;
            AUDIO_INFO_LOG("collaboration enabled, effectScene from %{public}d, sceneType changed to %{public}d",
                nodeInfo.effectInfo.lastEffectScene, nodeInfo.sceneType);
        }
    } else {
        RecoverNodeInfoForCollaboration(nodeInfo);
    }
}

HpaeProcessorType TransSourceTypeToSceneType(SourceType sourceType)
{
    if (g_sourceTypeToSceneTypeMap.find(sourceType) == g_sourceTypeToSceneTypeMap.end()) {
        return HPAE_SCENE_EFFECT_NONE;
    } else {
        return g_sourceTypeToSceneTypeMap[sourceType];
    }
}

bool CheckSceneTypeNeedEc(HpaeProcessorType processorType)
{
    return g_processorTypeNeedEcSet.find(processorType) != g_processorTypeNeedEcSet.end();
}

bool CheckSceneTypeNeedMicRef(HpaeProcessorType processorType)
{
    return g_processorTypeNeedMicRefSet.find(processorType) != g_processorTypeNeedMicRefSet.end();
}

static std::unordered_map<HpaeProcessorType, std::string> g_processorTypeToEffectSceneTypeMap = {
    {HPAE_SCENE_DEFAULT, "HPAE_SCENE_DEFAULT"},
    {HPAE_SCENE_OTHERS, "SCENE_OTHERS"},
    {HPAE_SCENE_MUSIC, "SCENE_MUSIC"},
    {HPAE_SCENE_GAME, "SCENE_GAME"},
    {HPAE_SCENE_MOVIE, "SCENE_MOVIE"},
    {HPAE_SCENE_SPEECH, "SCENE_SPEECH"},
    {HPAE_SCENE_RING, "SCENE_RING"},
    {HPAE_SCENE_VOIP_DOWN, "SCENE_VOIP_DOWN"},
    {HPAE_SCENE_COLLABORATIVE, "SCENE_COLLABORATIVE"}};

std::string TransProcessorTypeToSceneType(HpaeProcessorType processorType)
{
    if (g_processorTypeToEffectSceneTypeMap.find(processorType) == g_processorTypeToEffectSceneTypeMap.end()) {
        return "SCENE_EXTRA";
    } else {
        return g_processorTypeToEffectSceneTypeMap[processorType];
    }
}

bool CheckHpaeNodeInfoIsSame(HpaeNodeInfo &preNodeInfo, HpaeNodeInfo &curNodeInfo)
{
    return preNodeInfo.channels == curNodeInfo.channels &&  //&& preNodeInfo.format == curNodeInfo.format todo
           preNodeInfo.samplingRate == curNodeInfo.samplingRate &&
           preNodeInfo.channelLayout == curNodeInfo.channelLayout;
}

std::string TransNodeInfoToStringKey(HpaeNodeInfo& nodeInfo)
{
    std::string nodeKey = std::to_string(nodeInfo.sourceBufferType) + "_" +
                          std::to_string(nodeInfo.samplingRate) + "_" +
                          std::to_string(nodeInfo.channels) + "_" +
                          std::to_string(nodeInfo.format);
    return nodeKey;
}

AudioEnhanceScene TransProcessType2EnhanceScene(const HpaeProcessorType &processorType)
{
    if (g_processorTypeToSceneTypeMap.find(processorType) == g_processorTypeToSceneTypeMap.end()) {
        return SCENE_NONE;
    } else {
        return g_processorTypeToSceneTypeMap[processorType];
    }
}

size_t ConvertUsToFrameCount(uint64_t usTime, const HpaeNodeInfo &nodeInfo)
{
    return usTime * nodeInfo.samplingRate / TIME_US_PER_S /
        (nodeInfo.frameLen * nodeInfo.channels * static_cast<uint32_t>(GetSizeFromFormat(nodeInfo.format)));
}

uint64_t ConvertDatalenToUs(size_t bufferSize, const HpaeNodeInfo &nodeInfo)
{
    if (nodeInfo.channels == 0 || GetSizeFromFormat(nodeInfo.format) == 0 || nodeInfo.samplingRate == 0) {
        AUDIO_ERR_LOG("invalid nodeInfo");
        return 0;
    }

    double samples = static_cast<double>(bufferSize) /
                     (nodeInfo.channels * GetSizeFromFormat(nodeInfo.format));
    double seconds = samples / static_cast<int32_t>(nodeInfo.samplingRate);
    double microseconds = seconds * TIME_US_PER_S;

    return static_cast<uint64_t>(microseconds);
}

AudioSampleFormat TransFormatFromStringToEnum(std::string format)
{
    return static_cast<AudioSampleFormat>(g_formatFromParserStrToEnum[format]);
}

void AdjustMchSinkInfo(const AudioModuleInfo &audioModuleInfo, HpaeSinkInfo &sinkInfo)
{
    if (sinkInfo.deviceName != "MCH_Speaker") {
        return;
    }
    sinkInfo.channels = static_cast<AudioChannel>(DEFAULT_MULTICHANNEL_NUM);
    sinkInfo.channelLayout = DEFAULT_MULTICHANNEL_CHANNELLAYOUT;
    sinkInfo.frameLen = DEFAULT_MULTICHANNEL_FRAME_LEN_MS * sinkInfo.samplingRate / MS_PER_SECOND;
    sinkInfo.volume = MAX_SINK_VOLUME_LEVEL;
    AUDIO_INFO_LOG("adjust MCH SINK info ch: %{public}u, channelLayout: %{public}" PRIu64
                   " frameLen: %{public}zu volume %{public}f",
        sinkInfo.channels,
        sinkInfo.channelLayout,
        sinkInfo.frameLen,
        sinkInfo.volume);
}

int32_t TransModuleInfoToHpaeSinkInfo(const AudioModuleInfo &audioModuleInfo, HpaeSinkInfo &sinkInfo)
{
    if (g_formatFromParserStrToEnum.find(audioModuleInfo.format) == g_formatFromParserStrToEnum.end()) {
        AUDIO_ERR_LOG("openaudioport failed,format:%{public}s not supported", audioModuleInfo.format.c_str());
        return ERROR;
    }
    sinkInfo.deviceNetId = audioModuleInfo.networkId;
    sinkInfo.deviceClass = audioModuleInfo.className;
    AUDIO_INFO_LOG("HpaeManager::deviceNetId: %{public}s, deviceClass: %{public}s",
        sinkInfo.deviceNetId.c_str(),
        sinkInfo.deviceClass.c_str());
    sinkInfo.adapterName = audioModuleInfo.adapterName;
    sinkInfo.lib = audioModuleInfo.lib;
    sinkInfo.splitMode = audioModuleInfo.extra;
    sinkInfo.filePath = audioModuleInfo.fileName;

    sinkInfo.samplingRate = static_cast<AudioSamplingRate>(std::atol(audioModuleInfo.rate.c_str()));
    sinkInfo.format = static_cast<AudioSampleFormat>(TransFormatFromStringToEnum(audioModuleInfo.format));
    sinkInfo.channels = static_cast<AudioChannel>(std::atol(audioModuleInfo.channels.c_str()));
    int32_t bufferSize = static_cast<int32_t>(std::atol(audioModuleInfo.bufferSize.c_str()));
    sinkInfo.frameLen = static_cast<size_t>(bufferSize) / (sinkInfo.channels *
                                static_cast<size_t>(GetSizeFromFormat(sinkInfo.format)));
    sinkInfo.channelLayout = 0ULL;
    sinkInfo.deviceType = static_cast<int32_t>(std::atol(audioModuleInfo.deviceType.c_str()));
    sinkInfo.volume = static_cast<uint32_t>(std::atol(audioModuleInfo.deviceType.c_str()));
    sinkInfo.openMicSpeaker = static_cast<uint32_t>(std::atol(audioModuleInfo.OpenMicSpeaker.c_str()));
    sinkInfo.renderInIdleState = static_cast<uint32_t>(std::atol(audioModuleInfo.renderInIdleState.c_str()));
    sinkInfo.offloadEnable = static_cast<uint32_t>(std::atol(audioModuleInfo.offloadEnable.c_str()));
    sinkInfo.sinkLatency = static_cast<uint32_t>(std::atol(audioModuleInfo.sinkLatency.c_str()));
    sinkInfo.fixedLatency = static_cast<uint32_t>(std::atol(audioModuleInfo.fixedLatency.c_str()));
    sinkInfo.deviceName = audioModuleInfo.name;
    AdjustMchSinkInfo(audioModuleInfo, sinkInfo);
    return SUCCESS;
}

int32_t TransModuleInfoToHpaeSourceInfo(const AudioModuleInfo &audioModuleInfo, HpaeSourceInfo &sourceInfo)
{
    if (g_formatFromParserStrToEnum.find(audioModuleInfo.format) == g_formatFromParserStrToEnum.end()) {
        AUDIO_ERR_LOG("openaudioport failed,format:%{public}s not supported", audioModuleInfo.format.c_str());
        return ERROR;
    }
    sourceInfo.deviceNetId = audioModuleInfo.networkId;
    sourceInfo.deviceClass = audioModuleInfo.className;
    sourceInfo.adapterName = audioModuleInfo.adapterName;
    sourceInfo.sourceName = audioModuleInfo.name;  // built_in_mic
    sourceInfo.deviceName = audioModuleInfo.name;
    sourceInfo.sourceType = static_cast<SourceType>(std::atol(audioModuleInfo.sourceType.c_str()));
    sourceInfo.filePath = audioModuleInfo.fileName;
    int32_t bufferSize = static_cast<int32_t>(std::atol(audioModuleInfo.bufferSize.c_str()));
    sourceInfo.channels = static_cast<AudioChannel>(std::atol(audioModuleInfo.channels.c_str()));
    sourceInfo.format = TransFormatFromStringToEnum(audioModuleInfo.format);
    sourceInfo.frameLen = static_cast<size_t>(bufferSize) / (sourceInfo.channels *
                                static_cast<size_t>(GetSizeFromFormat(sourceInfo.format)));
    sourceInfo.samplingRate = static_cast<AudioSamplingRate>(std::atol(audioModuleInfo.rate.c_str()));
    sourceInfo.channelLayout = 0ULL;
    sourceInfo.deviceType = static_cast<int32_t>(std::atol(audioModuleInfo.deviceType.c_str()));
    sourceInfo.volume = static_cast<uint32_t>(std::atol(audioModuleInfo.deviceType.c_str()));  // 1.0f;

    sourceInfo.ecType = static_cast<HpaeEcType>(std::atol(audioModuleInfo.ecType.c_str()));
    sourceInfo.ecAdapterName = audioModuleInfo.ecAdapter;
    sourceInfo.ecSamplingRate = static_cast<AudioSamplingRate>(std::atol(audioModuleInfo.ecSamplingRate.c_str()));
    sourceInfo.ecFormat = TransFormatFromStringToEnum(audioModuleInfo.ecFormat);
    sourceInfo.ecChannels = static_cast<AudioChannel>(std::atol(audioModuleInfo.ecChannels.c_str()));
    sourceInfo.ecFrameLen = DEFAULT_MULTICHANNEL_FRAME_LEN_MS * (sourceInfo.ecSamplingRate / MS_PER_SECOND);

    sourceInfo.micRef = static_cast<HpaeMicRefSwitch>(std::atol(audioModuleInfo.openMicRef.c_str()));
    sourceInfo.micRefSamplingRate = static_cast<AudioSamplingRate>(std::atol(audioModuleInfo.micRefRate.c_str()));
    sourceInfo.micRefFormat = TransFormatFromStringToEnum(audioModuleInfo.micRefFormat);
    sourceInfo.micRefChannels = static_cast<AudioChannel>(std::atol(audioModuleInfo.micRefChannels.c_str()));
    sourceInfo.openMicSpeaker = static_cast<uint32_t>(std::atol(audioModuleInfo.OpenMicSpeaker.c_str()));
    sourceInfo.micRefFrameLen = DEFAULT_MULTICHANNEL_FRAME_LEN_MS * (sourceInfo.micRefSamplingRate / MS_PER_SECOND);
    return SUCCESS;
}

bool CheckSourceInfoIsDifferent(const HpaeSourceInfo &info, const HpaeSourceInfo &oldInfo)
{
    auto getKey = [](const HpaeSourceInfo &sourceInfo) {
        return std::tie(
            sourceInfo.deviceNetId,
            sourceInfo.deviceClass,
            sourceInfo.adapterName,
            sourceInfo.sourceName,
            sourceInfo.sourceType,
            sourceInfo.filePath,
            sourceInfo.deviceName,
            sourceInfo.frameLen,
            sourceInfo.samplingRate,
            sourceInfo.format,
            sourceInfo.channels,
            sourceInfo.channelLayout,
            sourceInfo.deviceType,
            sourceInfo.volume,
            sourceInfo.openMicSpeaker,
            sourceInfo.ecType,
            sourceInfo.ecFrameLen,
            sourceInfo.ecSamplingRate,
            sourceInfo.ecFormat,
            sourceInfo.ecChannels,
            sourceInfo.micRef,
            sourceInfo.micRefFrameLen,
            sourceInfo.micRefSamplingRate,
            sourceInfo.micRefFormat,
            sourceInfo.micRefChannels);
    };
    return getKey(info) != getKey(oldInfo);
}

std::string TransFormatFromEnumToString(AudioSampleFormat format)
{
    CHECK_AND_RETURN_RET_LOG(g_formatFromParserEnumToStr.find(format) != g_formatFromParserEnumToStr.end(),
        "", "error param format");
    return g_formatFromParserEnumToStr[format];
}

void RecoverNodeInfoForCollaboration(HpaeNodeInfo &nodeInfo)
{
    if (nodeInfo.effectInfo.effectScene == SCENE_COLLABORATIVE) {
        nodeInfo.effectInfo.effectScene = nodeInfo.effectInfo.lastEffectScene;
        nodeInfo.sceneType = TransEffectSceneToSceneType(nodeInfo.effectInfo.effectScene);
        AUDIO_INFO_LOG("collaboration disabled, effectScene changed to %{public}d, sceneType changed to %{public}d",
            nodeInfo.effectInfo.effectScene, nodeInfo.sceneType);
    }
}
}  // namespace HPAE
}  // namespace AudioStandard
}  // namespace OHOS