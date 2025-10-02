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
#ifndef AUDIO_SUITE_BASE_H
#define AUDIO_SUITE_BASE_H

#include <cstdint>
#include <memory>
#include "audio_stream_info.h"

namespace OHOS {
namespace AudioStandard {
namespace AudioSuite {

inline constexpr uint32_t INVALID_NODE_ID = 0;
inline constexpr uint32_t INVALID_PIPELINE_ID = 0;

typedef enum {
    NODE_TYPE_INPUT = 1,
    NODE_TYPE_OUTPUT = 2,
    NODE_TYPE_EQUALIZER = 3,
    NODE_TYPE_NOISE_REDUCTION = 4,
    NODE_TYPE_SOUND_FIELD = 5,
    NODE_TYPE_AUDIO_SEPARATION = 6,
    NODE_TYPE_TEMPO_PITCH = 7,
    NODE_TYPE_SPACE_RENDER = 8,
    NODE_TYPE_VOICE_BEAUTIFIER = 9,
    NODE_TYPE_ENVIRONMENT_EFFECT = 10,
    NODE_TYPE_AUDIO_MIXER = 11,
} AudioNodeType;

typedef enum {
    NODE_ENABLE = 1,
    NODE_DISABLE = 2,
} AudioNodeEnable;

typedef enum {
    ENCODING_TYPE_RAW = 0,
    ENCODING_TYPE_AUDIOVIVID = 1,
    ENCODING_TYPE_E_AC3 = 2,
} AudioStreamEncodingType;

struct AudioFormat {
    AudioChannelInfo audioChannelInfo;
    AudioSampleFormat format = INVALID_WIDTH;
    AudioSamplingRate rate = SAMPLE_RATE_48000;
    AudioStreamEncodingType encodingType = ENCODING_TYPE_RAW;
};

typedef enum {
    AUDIO_NODE_DEFAULT_OUTPORT_TYPE = 0,
    AUDIO_NODE_HUMAN_SOUND_OUTPORT_TYPE = 1,
    AUDIO_NODE_BACKGROUND_SOUND_OUTPORT_TYPE = 2,
} AudioNodePortType;

typedef struct {
    AudioNodeType nodeType;
    AudioFormat nodeFormat;
} AudioNodeBuilder;

typedef enum {
    DEFAULT_MODE = 1,
    BALLADS_MODE = 2,
    CHINESE_STYLE_MODE = 3,
    CLASSICAL_MODE = 4,
    DANCE_MUSIC_MODE = 5,
    JAZZ_MODE = 6,
    POP_MODE = 7,
    RB_MODE = 8,
    ROCK_MODE = 9,
} EqualizerMode;

#define EQUALIZER_BAND_NUM (10)

typedef struct AudioEqualizerFrequencyBandGains {
    int32_t gains[EQUALIZER_BAND_NUM];
} AudioEqualizerFrequencyBandGains;

typedef enum {
    AUDIO_SUITE_SOUND_FIELD_CLOSE = 1,
    AUDIO_SUITE_SOUND_FIELD_FRONT_FACING = 2,
    AUDIO_SUITE_SOUND_FIELD_GRAND = 3,
    AUDIO_SUITE_SOUND_FIELD_NEAR = 4,
    AUDIO_SUITE_SOUND_FIELD_WIDE = 5,
} SoundFieldType;

typedef enum {
    AUDIO_SUITE_ENVIRONMENT_TYPE_CLOSE = -1,
    AUDIO_SUITE_ENVIRONMENT_TYPE_BROADCAST = 0,
    AUDIO_SUITE_ENVIRONMENT_TYPE_EARPIECE = 1,
    AUDIO_SUITE_ENVIRONMENT_TYPE_UNDERWATER = 2,
    AUDIO_SUITE_ENVIRONMENT_TYPE_GRAMOPHONE = 3
} EnvironmentType;

typedef enum {
    AUDIO_SUITE_VOICE_BEAUTIFIER_TYPE_CLEAR,
    AUDIO_SUITE_VOICE_BEAUTIFIER_TYPE_THEATRE,
    AUDIO_SUITE_VOICE_BEAUTIFIER_TYPE_CD,
    AUDIO_SUITE_VOICE_BEAUTIFIER_TYPE_STUDIO,
    AUDIO_SUITE_VOICE_BEAUTIFIER_TYPE_NORMAL
} VoiceBeautifierType;

enum AudioSuitePipelineState {
    PIPELINE_STOPPED = 1,
    PIPELINE_RUNNING = 2,
};

enum PipelineWorkMode {
    PIPELINE_EDIT_MODE = 1,
    PIPELINE_REALTIME_MODE = 2,
};

}  // namespace AudioSuite
}  // namespace AudioStandard
}  // namespace OHOS
#endif  // AUDIO_SUITE_BASE_H