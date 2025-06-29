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
#ifndef LOG_TAG
#define LOG_TAG "AudioDefinitionPolicyUtils"
#endif

#include "audio_definition_policy_utils.h"
#include <ability_manager_client.h>
#include "iservice_registry.h"
#include "parameter.h"
#include "parameters.h"
#include "audio_utils.h"
#include "audio_log.h"

namespace OHOS {
namespace AudioStandard {
std::unordered_map<std::string, DeviceRole> AudioDefinitionPolicyUtils::deviceRoleStrToEnum = {
    {"input", INPUT_DEVICE},
    {"output", OUTPUT_DEVICE},
};

std::unordered_map<std::string, AudioPipeRole> AudioDefinitionPolicyUtils::pipeRoleStrToEnum = {
    {"input", PIPE_ROLE_INPUT},
    {"output", PIPE_ROLE_OUTPUT},
};

std::unordered_map<std::string, DeviceType> AudioDefinitionPolicyUtils::deviceTypeStrToEnum = {
    {"DEVICE_TYPE_EARPIECE", DEVICE_TYPE_EARPIECE},
    {"DEVICE_TYPE_SPEAKER", DEVICE_TYPE_SPEAKER},
    {"DEVICE_TYPE_WIRED_HEADSET", DEVICE_TYPE_WIRED_HEADSET},
    {"DEVICE_TYPE_WIRED_HEADPHONES", DEVICE_TYPE_WIRED_HEADPHONES},
    {"DEVICE_TYPE_BLUETOOTH_SCO", DEVICE_TYPE_BLUETOOTH_SCO},
    {"DEVICE_TYPE_BLUETOOTH_A2DP", DEVICE_TYPE_BLUETOOTH_A2DP},
    {"DEVICE_TYPE_BLUETOOTH_A2DP_IN", DEVICE_TYPE_BLUETOOTH_A2DP_IN},
    {"DEVICE_TYPE_MIC", DEVICE_TYPE_MIC},
    {"DEVICE_TYPE_WAKEUP", DEVICE_TYPE_WAKEUP},
    {"DEVICE_TYPE_USB_HEADSET", DEVICE_TYPE_USB_HEADSET},
    {"DEVICE_TYPE_DP", DEVICE_TYPE_DP},
    {"DEVICE_TYPE_REMOTE_CAST", DEVICE_TYPE_REMOTE_CAST},
    {"DEVICE_TYPE_HDMI", DEVICE_TYPE_HDMI},
    {"DEVICE_TYPE_LINE_DIGITAL", DEVICE_TYPE_LINE_DIGITAL},
    {"DEVICE_TYPE_FILE_SINK", DEVICE_TYPE_FILE_SINK},
    {"DEVICE_TYPE_FILE_SOURCE", DEVICE_TYPE_FILE_SOURCE},
    {"DEVICE_TYPE_EXTERN_CABLE", DEVICE_TYPE_EXTERN_CABLE},
    {"DEVICE_TYPE_DEFAULT", DEVICE_TYPE_DEFAULT},
    {"DEVICE_TYPE_USB_ARM_HEADSET", DEVICE_TYPE_USB_ARM_HEADSET},
    {"DEVICE_TYPE_ACCESSORY", DEVICE_TYPE_ACCESSORY},
    {"DEVICE_TYPE_NEARLINK", DEVICE_TYPE_NEARLINK},
    {"DEVICE_TYPE_NEARLINK_IN", DEVICE_TYPE_NEARLINK_IN},
};

std::unordered_map<std::string, AudioPin> AudioDefinitionPolicyUtils::pinStrToEnum = {
    {"PIN_OUT_SPEAKER", AUDIO_PIN_OUT_SPEAKER},
    {"PIN_OUT_HEADSET", AUDIO_PIN_OUT_HEADSET},
    {"PIN_OUT_LINEOUT", AUDIO_PIN_OUT_LINEOUT},
    {"PIN_OUT_HDMI", AUDIO_PIN_OUT_HDMI},
    {"PIN_OUT_USB", AUDIO_PIN_OUT_USB},
    {"PIN_OUT_USB_EXT", AUDIO_PIN_OUT_USB_EXT},
    {"PIN_OUT_EARPIECE", AUDIO_PIN_OUT_EARPIECE},
    {"PIN_OUT_BLUETOOTH_SCO", AUDIO_PIN_OUT_BLUETOOTH_SCO},
    {"PIN_OUT_DAUDIO_DEFAULT", AUDIO_PIN_OUT_DAUDIO_DEFAULT},
    {"PIN_OUT_HEADPHONE", AUDIO_PIN_OUT_HEADPHONE},
    {"PIN_OUT_USB_HEADSET", AUDIO_PIN_OUT_USB_HEADSET},
    {"PIN_OUT_BLUETOOTH_A2DP", AUDIO_PIN_OUT_BLUETOOTH_A2DP},
    {"PIN_OUT_DP", AUDIO_PIN_OUT_DP},
    {"PIN_OUT_NEARLINK", AUDIO_PIN_OUT_NEARLINK},
    {"PIN_IN_MIC", AUDIO_PIN_IN_MIC},
    {"PIN_IN_HS_MIC", AUDIO_PIN_IN_HS_MIC},
    {"PIN_IN_LINEIN", AUDIO_PIN_IN_LINEIN},
    {"PIN_IN_USB_EXT", AUDIO_PIN_IN_USB_EXT},
    {"PIN_IN_BLUETOOTH_SCO_HEADSET", AUDIO_PIN_IN_BLUETOOTH_SCO_HEADSET},
    {"PIN_IN_DAUDIO_DEFAULT", AUDIO_PIN_IN_DAUDIO_DEFAULT},
    {"PIN_IN_PENCIL", AUDIO_PIN_IN_PENCIL},
    {"PIN_IN_UWB", AUDIO_PIN_IN_UWB},
    {"PIN_IN_NEARLINK", AUDIO_PIN_IN_NEARLINK},
};

std::unordered_map<std::string, AudioSampleFormat> AudioDefinitionPolicyUtils::formatStrToEnum = {
    {"s16le", SAMPLE_S16LE},
    {"s24le", SAMPLE_S24LE},
    {"s32le", SAMPLE_S32LE},
    {"eac3", INVALID_WIDTH},
};

// for moduleInfo
std::unordered_map<AudioSampleFormat, std::string> AudioDefinitionPolicyUtils::enumToFormatStr = {
    {SAMPLE_S16LE, "s16le"},
    {SAMPLE_S24LE, "s24le"},
    {SAMPLE_S32LE, "s32le"},
};

std::unordered_map<std::string, AudioChannelLayout> AudioDefinitionPolicyUtils::layoutStrToEnum = {
    {"CH_LAYOUT_MONO", CH_LAYOUT_MONO},
    {"CH_LAYOUT_STEREO", CH_LAYOUT_STEREO},
    {"CH_LAYOUT_STEREO_DOWNMIX", CH_LAYOUT_STEREO_DOWNMIX},
    {"CH_LAYOUT_2POINT1", CH_LAYOUT_2POINT1},
    {"CH_LAYOUT_3POINT0", CH_LAYOUT_3POINT0},
    {"CH_LAYOUT_SURROUND", CH_LAYOUT_SURROUND},
    {"CH_LAYOUT_3POINT1", CH_LAYOUT_3POINT1},
    {"CH_LAYOUT_4POINT0", CH_LAYOUT_4POINT0},
    {"CH_LAYOUT_QUAD_SIDE", CH_LAYOUT_QUAD_SIDE},
    {"CH_LAYOUT_QUAD", CH_LAYOUT_QUAD},
    {"CH_LAYOUT_2POINT0POINT2", CH_LAYOUT_2POINT0POINT2},
    {"CH_LAYOUT_4POINT1", CH_LAYOUT_4POINT1},
    {"CH_LAYOUT_5POINT0", CH_LAYOUT_5POINT0},
    {"CH_LAYOUT_5POINT0_BACK", CH_LAYOUT_5POINT0_BACK},
    {"CH_LAYOUT_2POINT1POINT2", CH_LAYOUT_2POINT1POINT2},
    {"CH_LAYOUT_3POINT0POINT2", CH_LAYOUT_3POINT0POINT2},
    {"CH_LAYOUT_5POINT1", CH_LAYOUT_5POINT1},
    {"CH_LAYOUT_5POINT1_BACK", CH_LAYOUT_5POINT1_BACK},
    {"CH_LAYOUT_6POINT0", CH_LAYOUT_6POINT0},
    {"CH_LAYOUT_HEXAGONAL", CH_LAYOUT_HEXAGONAL},
    {"CH_LAYOUT_3POINT1POINT2", CH_LAYOUT_3POINT1POINT2},
    {"CH_LAYOUT_6POINT0_FRONT", CH_LAYOUT_6POINT0_FRONT},
    {"CH_LAYOUT_6POINT1", CH_LAYOUT_6POINT1},
    {"CH_LAYOUT_6POINT1_BACK", CH_LAYOUT_6POINT1_BACK},
    {"CH_LAYOUT_6POINT1_FRONT", CH_LAYOUT_6POINT1_FRONT},
    {"CH_LAYOUT_7POINT0", CH_LAYOUT_7POINT0},
    {"CH_LAYOUT_7POINT0_FRONT", CH_LAYOUT_7POINT0_FRONT},
    {"CH_LAYOUT_7POINT1", CH_LAYOUT_7POINT1},
    {"CH_LAYOUT_OCTAGONAL", CH_LAYOUT_OCTAGONAL},
    {"CH_LAYOUT_5POINT1POINT2", CH_LAYOUT_5POINT1POINT2},
    {"CH_LAYOUT_7POINT1_WIDE", CH_LAYOUT_7POINT1_WIDE},
    {"CH_LAYOUT_7POINT1_WIDE_BACK", CH_LAYOUT_7POINT1_WIDE_BACK},
    {"CH_LAYOUT_5POINT1POINT4", CH_LAYOUT_5POINT1POINT4},
    {"CH_LAYOUT_7POINT1POINT2", CH_LAYOUT_7POINT1POINT2},
    {"CH_LAYOUT_7POINT1POINT4", CH_LAYOUT_7POINT1POINT4},
    {"CH_LAYOUT_10POINT2", CH_LAYOUT_10POINT2},
    {"CH_LAYOUT_9POINT1POINT4", CH_LAYOUT_9POINT1POINT4},
    {"CH_LAYOUT_9POINT1POINT6", CH_LAYOUT_9POINT1POINT6},
    {"CH_LAYOUT_HEXADECAGONAL", CH_LAYOUT_HEXADECAGONAL},
    {"CH_LAYOUT_HOA_ORDER1_ACN_N3D", CH_LAYOUT_HOA_ORDER1_ACN_N3D},
    {"CH_LAYOUT_HOA_ORDER1_ACN_SN3D", CH_LAYOUT_HOA_ORDER1_ACN_SN3D},
    {"CH_LAYOUT_HOA_ORDER1_FUMA", CH_LAYOUT_HOA_ORDER1_FUMA},
    {"CH_LAYOUT_HOA_ORDER2_ACN_N3D", CH_LAYOUT_HOA_ORDER2_ACN_N3D},
    {"CH_LAYOUT_HOA_ORDER2_ACN_SN3D", CH_LAYOUT_HOA_ORDER2_ACN_SN3D},
    {"CH_LAYOUT_HOA_ORDER2_FUMA", CH_LAYOUT_HOA_ORDER2_FUMA},
    {"CH_LAYOUT_HOA_ORDER3_ACN_N3D", CH_LAYOUT_HOA_ORDER3_ACN_N3D},
    {"CH_LAYOUT_HOA_ORDER3_ACN_SN3D", CH_LAYOUT_HOA_ORDER3_ACN_SN3D},
    {"CH_LAYOUT_HOA_ORDER3_FUMA", CH_LAYOUT_HOA_ORDER3_FUMA},
};

std::unordered_map<std::string, AudioFlag> AudioDefinitionPolicyUtils::flagStrToEnum = {
    {"AUDIO_OUTPUT_FLAG_NORMAL", AUDIO_OUTPUT_FLAG_NORMAL},
    {"AUDIO_OUTPUT_FLAG_DIRECT", AUDIO_OUTPUT_FLAG_DIRECT},
    {"AUDIO_OUTPUT_FLAG_HD", AUDIO_OUTPUT_FLAG_HD},
    {"AUDIO_OUTPUT_FLAG_MULTICHANNEL", AUDIO_OUTPUT_FLAG_MULTICHANNEL},
    {"AUDIO_OUTPUT_FLAG_LOWPOWER", AUDIO_OUTPUT_FLAG_LOWPOWER},
    {"AUDIO_OUTPUT_FLAG_FAST", AUDIO_OUTPUT_FLAG_FAST},
    {"AUDIO_OUTPUT_FLAG_VOIP", AUDIO_OUTPUT_FLAG_VOIP},
    {"AUDIO_OUTPUT_FLAG_VOIP_FAST", AUDIO_OUTPUT_FLAG_VOIP_FAST},
    {"AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD", AUDIO_OUTPUT_FLAG_COMPRESS_OFFLOAD},
    {"AUDIO_INPUT_FLAG_NORMAL", AUDIO_INPUT_FLAG_NORMAL},
    {"AUDIO_INPUT_FLAG_FAST", AUDIO_INPUT_FLAG_FAST},
    {"AUDIO_INPUT_FLAG_VOIP", AUDIO_INPUT_FLAG_VOIP},
    {"AUDIO_INPUT_FLAG_VOIP_FAST", AUDIO_INPUT_FLAG_VOIP_FAST},
    {"AUDIO_INPUT_FLAG_WAKEUP", AUDIO_INPUT_FLAG_WAKEUP},
};

std::unordered_map<std::string, AudioPreloadType> AudioDefinitionPolicyUtils::preloadStrToEnum = {
    {"false", PRELOAD_TYPE_NOTSUPPORT},
    {"true", PRELOAD_TYPE_SUPPORT},
};

std::unordered_map<std::string, uint32_t> AudioDefinitionPolicyUtils::usageStrToEnum = {
    {"AUDIO_USAGE_NORMAL", AUDIO_USAGE_NORMAL},
    {"AUDIO_USAGE_VOIP", AUDIO_USAGE_VOIP},
};

uint32_t AudioDefinitionPolicyUtils::PcmFormatToBytes(AudioSampleFormat format)
{
    // AudioSampleFormat / PCM_8_BIT
    switch (format) {
        case SAMPLE_U8:
            return 1; // 1 byte
        case SAMPLE_S16LE:
            return 2; // 2 byte
        case SAMPLE_S24LE:
            return 3; // 3 byte
        case SAMPLE_S32LE:
            return 4; // 4 byte
        case SAMPLE_F32LE:
            return 4; // 4 byte
        default:
            return 2; // 2 byte
    }
}

AudioChannel AudioDefinitionPolicyUtils::ConvertLayoutToAudioChannel(AudioChannelLayout layout)
{
    AudioChannel channel = AudioChannel::CHANNEL_UNKNOW;
    switch (layout) {
        case AudioChannelLayout::CH_LAYOUT_MONO:
            channel = AudioChannel::MONO;
            break;
        case AudioChannelLayout::CH_LAYOUT_STEREO:
            channel = AudioChannel::STEREO;
            break;
        case AudioChannelLayout::CH_LAYOUT_2POINT1:
        case AudioChannelLayout::CH_LAYOUT_3POINT0:
            channel = AudioChannel::CHANNEL_3;
            break;
        case AudioChannelLayout::CH_LAYOUT_3POINT1:
        case AudioChannelLayout::CH_LAYOUT_4POINT0:
        case AudioChannelLayout::CH_LAYOUT_QUAD:
            channel = AudioChannel::CHANNEL_4;
            break;
        case AudioChannelLayout::CH_LAYOUT_5POINT0:
        case AudioChannelLayout::CH_LAYOUT_2POINT1POINT2:
            channel = AudioChannel::CHANNEL_5;
            break;
        case AudioChannelLayout::CH_LAYOUT_5POINT1:
        case AudioChannelLayout::CH_LAYOUT_HEXAGONAL:
        case AudioChannelLayout::CH_LAYOUT_3POINT1POINT2:
            channel = AudioChannel::CHANNEL_6;
            break;
        case AudioChannelLayout::CH_LAYOUT_7POINT0:
            channel = AudioChannel::CHANNEL_7;
            break;
        case AudioChannelLayout::CH_LAYOUT_7POINT1:
            channel = AudioChannel::CHANNEL_8;
            break;
        case AudioChannelLayout::CH_LAYOUT_7POINT1POINT2:
            channel = AudioChannel::CHANNEL_10;
            break;
        case AudioChannelLayout::CH_LAYOUT_7POINT1POINT4:
            channel = AudioChannel::CHANNEL_12;
            break;
        default:
            channel = AudioChannel::CHANNEL_UNKNOW;
            break;
    }
    return channel;
}
}
}
