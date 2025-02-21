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

#ifndef AUDIO_FRAMEWORK_AUDIO_EFFECT_H
#define AUDIO_FRAMEWORK_AUDIO_EFFECT_H

#include <cassert>
#include <cstdint>
#include <stddef.h>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include "audio_info.h"

#define AUDIO_EFFECT_LIBRARY_INFO_SYM_AS_STR "AELI"
#define EFFECT_STRING_LEN_MAX 64

namespace OHOS {
namespace AudioStandard {
// audio effect manager info
constexpr int32_t AUDIO_EFFECT_COUNT_UPPER_LIMIT = 20;
constexpr int32_t AUDIO_EFFECT_COUNT_FIRST_NODE_UPPER_LIMIT = 1;
constexpr int32_t AUDIO_EFFECT_CHAIN_CONFIG_UPPER_LIMIT = 64; // max conf for sceneType + effectMode + deviceType
constexpr int32_t AUDIO_EFFECT_CHAIN_COUNT_UPPER_LIMIT = 32; // max num of effectChain
constexpr int32_t AUDIO_EFFECT_COUNT_PER_CHAIN_UPPER_LIMIT = 16; // max num of effect per effectChain

constexpr int32_t HDI_SET_PATAM = 6;
constexpr int32_t HDI_INIT = 0;
constexpr int32_t HDI_BYPASS = 1;
constexpr int32_t HDI_HEAD_MODE = 2;
constexpr int32_t HDI_ROOM_MODE = 3;
constexpr int32_t HDI_BLUETOOTH_MODE = 4;
constexpr int32_t HDI_DESTROY = 5;

enum AudioSpatialDeviceType {
    EARPHONE_TYPE_NONE = 0,
    EARPHONE_TYPE_INEAR,
    EARPHONE_TYPE_HALF_INEAR,
    EARPHONE_TYPE_HEADPHONE,
    EARPHONE_TYPE_GLASSES,
    EARPHONE_TYPE_OTHERS,
};

struct AudioSpatialDeviceState {
    std::string address;
    bool isSpatializationSupported;
    bool isHeadTrackingSupported;
    AudioSpatialDeviceType spatialDeviceType;
};

struct Library {
    std::string name;
    std::string path;
};

struct Effect {
    std::string name;
    std::string libraryName;
};

struct EffectChain {
    std::string name;
    std::vector<std::string> apply;
    std::string label = "";
};

struct Device {
    std::string type;
    std::string chain;
};

struct Preprocess {
    std::string stream;
    std::vector<std::string> mode;
    std::vector<std::vector<Device>> device;
};

struct Postprocess {
    std::string stream;
    std::vector<std::string> mode;
    std::vector<std::vector<Device>> device;
};

struct OriginalEffectConfig {
    float version;
    std::vector<Library> libraries;
    std::vector<Effect> effects;
    std::vector<EffectChain> effectChains;
    std::vector<Preprocess> preProcess;
    std::vector<Postprocess> postProcess;
};

struct StreamEffectMode {
    std::string mode;
    std::vector<Device> devicePort;
};

struct Stream {
    std::string scene;
    std::vector<StreamEffectMode> streamEffectMode;
};

struct ProcessNew {
    std::vector<Stream> stream;
};

struct SupportedEffectConfig {
    std::vector<EffectChain> effectChains;
    ProcessNew preProcessNew;
    ProcessNew postProcessNew;
};


/**
* Enumerates the audio scene effect type.
*/
enum AudioEffectScene {
    SCENE_OTHERS = 0,
    SCENE_MUSIC = 1,
    SCENE_MOVIE = 2,
    SCENE_GAME = 3,
    SCENE_SPEECH = 4,
    SCENE_RING = 5
};

/**
* Enumerates the audio scene effct mode.
*/
enum AudioEffectMode {
    EFFECT_NONE = 0,
    EFFECT_DEFAULT = 1
};

struct AudioSceneEffectInfo {
    std::vector<AudioEffectMode> mode;
};

const std::unordered_map<AudioEffectScene, std::string> AUDIO_SUPPORTED_SCENE_TYPES {
    {SCENE_OTHERS, "SCENE_OTHERS"},
    {SCENE_MUSIC, "SCENE_MUSIC"},
    {SCENE_MOVIE, "SCENE_MOVIE"},
    {SCENE_GAME, "SCENE_GAME"},
    {SCENE_SPEECH, "SCENE_SPEECH"},
    {SCENE_RING, "SCENE_RING"}
};

const std::unordered_map<AudioEffectMode, std::string> AUDIO_SUPPORTED_SCENE_MODES {
    {EFFECT_NONE, "EFFECT_NONE"},
    {EFFECT_DEFAULT, "EFFECT_DEFAULT"},
};

const std::unordered_map<DeviceType, std::string> SUPPORTED_DEVICE_TYPE {
    {DEVICE_TYPE_NONE, "DEVICE_TYPE_NONE"},
    {DEVICE_TYPE_INVALID, "DEVICE_TYPE_INVALID"},
    {DEVICE_TYPE_EARPIECE, "DEVICE_TYPE_EARPIECE"},
    {DEVICE_TYPE_SPEAKER, "DEVICE_TYPE_SPEAKER"},
    {DEVICE_TYPE_WIRED_HEADSET, "DEVICE_TYPE_WIRED_HEADSET"},
    {DEVICE_TYPE_WIRED_HEADPHONES, "DEVICE_TYPE_WIRED_HEADPHONES"},
    {DEVICE_TYPE_BLUETOOTH_SCO, "DEVICE_TYPE_BLUETOOTH_SCO"},
    {DEVICE_TYPE_BLUETOOTH_A2DP, "DEVICE_TYPE_BLUETOOTH_A2DP"},
    {DEVICE_TYPE_MIC, "DEVICE_TYPE_MIC"},
    {DEVICE_TYPE_WAKEUP, "DEVICE_TYPE_WAKEUP"},
    {DEVICE_TYPE_USB_HEADSET, "DEVICE_TYPE_USB_HEADSET"},
    {DEVICE_TYPE_FILE_SINK, "DEVICE_TYPE_FILE_SINK"},
    {DEVICE_TYPE_FILE_SOURCE, "DEVICE_TYPE_FILE_SOURCE"},
    {DEVICE_TYPE_EXTERN_CABLE, "DEVICE_TYPE_EXTERN_CABLE"},
    {DEVICE_TYPE_DEFAULT, "DEVICE_TYPE_DEFAULT"},
};

enum AudioEffectCommandCode {
    EFFECT_CMD_INIT = 0,
    EFFECT_CMD_SET_CONFIG = 1,
    EFFECT_CMD_ENABLE = 2,
    EFFECT_CMD_DISABLE = 3,
    EFFECT_CMD_SET_PARAM = 4,
    EFFECT_CMD_GET_PARAM = 5,
    EFFECT_CMD_GET_CONFIG = 6,
    EFFECT_CMD_SET_IMU = 7
};

enum AudioEffectParamSetCode {
    EFFECT_SET_BYPASS = 1,
    EFFECT_SET_PARAM = 2,
};

enum AudioDataFormat {
    DATA_FORMAT_S16 = SAMPLE_S16LE,
    DATA_FORMAT_S24 = SAMPLE_S24LE,
    DATA_FORMAT_S32 = SAMPLE_S32LE,
    DATA_FORMAT_F32 = SAMPLE_F32LE,
};

struct AudioEffectParam {
    int32_t status;
    uint32_t paramSize;
    uint32_t valueSize;
    int32_t data[];
};

struct AudioBuffer {
    size_t frameLength;
    union {
        void*     raw;
        float*    f32;
        int32_t*  s32;
        int16_t*  s16;
        uint8_t*  u8;
    };
    void *metaData;
};

struct AudioBufferConfig {
    uint32_t samplingRate;
    uint32_t channels;
    uint8_t format;
    uint64_t channelLayout;
    AudioEncodingType encoding;
};

struct AudioEffectConfig {
    AudioBufferConfig inputCfg;
    AudioBufferConfig outputCfg;
};
 
struct AudioEffectTransInfo {
    uint32_t size;
    void *data;
};

struct AudioEffectDescriptor {
    std::string libraryName;
    std::string effectName;
};

typedef struct AudioEffectInterface **AudioEffectHandle;

struct AudioEffectInterface {
    int32_t (*process) (AudioEffectHandle self, AudioBuffer *inBuffer, AudioBuffer *outBuffer);
    int32_t (*command) (AudioEffectHandle self, uint32_t cmdCode,
        AudioEffectTransInfo *cmdInfo, AudioEffectTransInfo *replyInfo);
};

struct AudioEffectLibrary {
    uint32_t version;
    const char *name;
    const char *implementor;
    bool (*checkEffect) (const AudioEffectDescriptor descriptor);
    int32_t (*createEffect) (const AudioEffectDescriptor descriptor, AudioEffectHandle *handle);
    int32_t (*releaseEffect) (AudioEffectHandle handle);
};

struct AudioEffectLibEntry {
    AudioEffectLibrary *audioEffectLibHandle;
    std::string libraryName;
    std::vector<std::string> effectName;
};

struct AudioSpatializationState {
    bool spatializationEnabled;
    bool headTrackingEnabled;
};

} // namespace AudioStandard
} // namespace OHOS

#endif //AUDIO_FRAMEWORK_AUDIO_EFFECT_H