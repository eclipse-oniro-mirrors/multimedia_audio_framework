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

/**
 * @addtogroup OHAudio
 * @{
 *
 * @brief Provide the definition of the C interface for the audio module.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 *
 * @since 10
 * @version 1.0
 */

/**
 * @file native_audiostream_base.h
 *
 * @brief Declare the underlying data structure.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @since 10
 * @version 1.0
 */

#ifndef NATIVE_AUDIOSTREAM_BASE_H
#define NATIVE_AUDIOSTREAM_BASE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Define the result of the function execution.
 *
 * @since 10
 */
typedef enum {
    /**
     * The call was successful.
     *
     * @since 10
     */
    AUDIOSTREAM_SUCCESS = 0,

    /**
     * This means that the function was executed with an invalid input parameter.
     *
     * @since 10
     */
    AUDIOSTREAM_ERROR_INVALID_PARAM = 1,

    /**
     * Execution status exception.
     *
     * @since 10
     */
    AUDIOSTREAM_ERROR_ILLEGAL_STATE = 2,

    /**
     * An system error has occurred.
     *
     * @since 10
     */
    AUDIOSTREAM_ERROR_SYSTEM = 3
} OH_AudioStream_Result;

/**
 * Define the audio stream type.
 *
 * @since 10
 */
typedef enum {
    /**
     * The type for audio stream is renderer.
     *
     * @since 10
     */
    AUDIOSTREAM_TYPE_RENDERER = 1,

    /**
     * The type for audio stream is capturer.
     *
     * @since 10
     */
    AUDIOSTREAM_TYPE_CAPTURER = 2
} OH_AudioStream_Type;

/**
 * Define the audio stream sample format.
 *
 * @since 10
 */
typedef enum {
    /**
     * Unsigned 8 format.
     *
     * @since 10
     */
    AUDIOSTREAM_SAMPLE_U8 = 0,
    /**
     * Signed 16 bit integer, little endian.
     *
     * @since 10
     */
    AUDIOSTREAM_SAMPLE_S16LE = 1,
    /**
     * Signed 24 bit integer, little endian.
     *
     * @since 10
     */
    AUDIOSTREAM_SAMPLE_S24LE = 2,
    /**
     * Signed 32 bit integer, little endian.
     *
     * @since 10
     */
    AUDIOSTREAM_SAMPLE_S32LE = 3,
} OH_AudioStream_SampleFormat;

/**
 * Define the audio encoding type.
 *
 * @since 10
 */
typedef enum {
    /**
     * PCM encoding type.
     *
     * @since 10
     */
    AUDIOSTREAM_ENCODING_TYPE_RAW = 0,
} OH_AudioStream_EncodingType;

/**
 * Define the audio stream usage.
 * Audio stream usage is used to describe what work scenario
 * the current stream is used for.
 *
 * @since 10
 */
typedef enum {
    /**
     * Unknown usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_UNKNOWN = 0,
    /**
     * Music usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_MUSIC = 1,
    /**
     * Voice communication usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_VOICE_COMMUNICATION = 2,
    /**
     * Voice assistant usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_VOICE_ASSISTANT = 3,
    /**
     * Alarm usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_ALARM = 4,
    /**
     * Voice message usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_VOICE_MESSAGE = 5,
    /**
     * Ringtone usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_RINGTONE = 6,
    /**
     * Notification usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_NOTIFICATION = 7,
    /**
     * Accessibility usage, such as screen reader.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_ACCESSIBILITY = 8,
    /**
     * Movie or video usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_MOVIE = 10,
    /**
     * Game sound effect usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_GAME = 11,
    /**
     * Audiobook usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_AUDIOBOOK = 12,
    /**
     * Navigation usage.
     *
     * @since 10
     */
    AUDIOSTREAM_USAGE_NAVIGATION = 13,
} OH_AudioStream_Usage;

/**
 * Define the audio latency mode.
 *
 * @since 10
 */
typedef enum {
    /**
     * This is a normal audio scene.
     *
     * @since 10
     */
    AUDIOSTREAM_LATENCY_MODE_NORMAL = 0,
    /**
     * This is a low latency audio scene.
     *
     * @since 10
     */
    AUDIOSTREAM_LATENCY_MODE_FAST = 1
} OH_AudioStream_LatencyMode;

/**
 * Define the audio event.
 *
 * @since 10
 */
typedef enum {
    /**
     * The routing of the audio has changed.
     *
     * @since 10
     */
    AUDIOSTREAM_EVENT_ROUTING_CHANGED = 0
} OH_AudioStream_Event;

/**
 * The audio stream states
 *
 * @since 10
 */
typedef enum {
    /**
     * The invalid state.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_INVALID = -1,
    /**
     * Create new instance state.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_NEW = 0,
    /**
     * The prepared state.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_PREPARED = 1,
    /**
     * The stream is running.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_RUNNING = 2,
    /**
     * The stream is stopped.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_STOPPED = 3,
    /**
     * The stream is released.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_RELEASED = 4,
    /**
     * The stream is paused.
     *
     * @since 10
     */
    AUDIOSTREAM_STATE_PAUSED = 5,
} OH_AudioStream_State;

/**
 * Defines the audio interrupt type.
 *
 * @since 10
 */
typedef enum {
    /**
     * Force type, system change audio state.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_FORCE = 0,
    /**
     * Share type, application change audio state.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_SHARE = 1
} OH_AudioInterrupt_ForceType;

/**
 * Defines the audio interrupt hint type.
 *
 * @since 10
 */
typedef enum {
    /**
     * None.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_NONE = 0,
    /**
     * Resume the stream.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_RESUME = 1,
    /**
     * Pause the stream.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_PAUSE = 2,
    /**
     * Stop the stream.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_STOP = 3,
    /**
     * Ducked the stream.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_DUCK = 4,
    /**
     * Unducked the stream.
     *
     * @since 10
     */
    AUDIOSTREAM_INTERRUPT_HINT_UNDUCK = 5
} OH_AudioInterrupt_Hint;

/**
 * Defines the audio source type.
 *
 * @since 10
 */
typedef enum {
    /**
     * Invalid type.
     *
     * @since 10
     */
    AUDIOSTREAM_SOURCE_TYPE_INVALID = -1,
    /**
     * Mic source type.
     *
     * @since 10
     */
    AUDIOSTREAM_SOURCE_TYPE_MIC = 0,
    /**
     * Voice recognition source type.
     *
     * @since 10
     */
    AUDIOSTREAM_SOURCE_TYPE_VOICE_RECOGNITION = 1,
    /**
     * Playback capture source type.
     *
     * @since 10
     */
    AUDIOSTREAM_SOURCE_TYPE_PLAYBACK_CAPTURE = 2,
    /**
     * Voice call source type.
     *
     * @permission ohos.permission.RECORD_VOICE_CALL
     * @systemapi
     * @since 11
     */
    AUDIOSTREAM_SOURCE_TYPE_VOICE_CALL = 4,
    /**
     * Voice communication source type.
     *
     * @since 10
     */
    AUDIOSTREAM_SOURCE_TYPE_VOICE_COMMUNICATION = 7
} OH_AudioStream_SourceType;

/**
 * Declaring the audio stream builder.
 * The instance of builder is used for creating audio stream.
 *
 * @since 10
 */
typedef struct OH_AudioStreamBuilderStruct OH_AudioStreamBuilder;

/**
 * Declaring the audio renderer stream.
 * The instance of renderer stream is used for playing audio data.
 *
 * @since 10
 */
typedef struct OH_AudioRendererStruct OH_AudioRenderer;

/**
 * Declaring the audio capturer stream.
 * The instance of renderer stream is used for capturing audio data.
 *
 * @since 10
 */
typedef struct OH_AudioCapturerStruct OH_AudioCapturer;

/**
 * Declaring the callback struct for renderer stream.
 *
 * @since 10
 */
typedef struct OH_AudioRenderer_Callbacks_Struct {
    /**
     * This function pointer will point to the callback function that
     * is used to write audio data
     *
     * @since 10
     */
    int32_t (*OH_AudioRenderer_OnWriteData)(
            OH_AudioRenderer* renderer,
            void* userData,
            void* buffer,
            int32_t lenth);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio renderer stream events.
     *
     * @since 10
     */
    int32_t (*OH_AudioRenderer_OnStreamEvent)(
            OH_AudioRenderer* renderer,
            void* userData,
            OH_AudioStream_Event event);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio interrupt events.
     *
     * @since 10
     */
    int32_t (*OH_AudioRenderer_OnInterruptEvent)(
            OH_AudioRenderer* renderer,
            void* userData,
            OH_AudioInterrupt_ForceType type,
            OH_AudioInterrupt_Hint hint);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio error result.
     *
     * @since 10
     */
    int32_t (*OH_AudioRenderer_OnError)(
            OH_AudioRenderer* renderer,
            void* userData,
            OH_AudioStream_Result error);
} OH_AudioRenderer_Callbacks;

/**
 * Declaring the callback struct for capturer stream.
 *
 * @since 10
 */
typedef struct OH_AudioCapturer_Callbacks_Struct {
    /**
     * This function pointer will point to the callback function that
     * is used to read audio data.
     *
     * @since 10
     */
    int32_t (*OH_AudioCapturer_OnReadData)(
            OH_AudioCapturer* capturer,
            void* userData,
            void* buffer,
            int32_t lenth);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio capturer stream events.
     *
     * @since 10
     */
    int32_t (*OH_AudioCapturer_OnStreamEvent)(
            OH_AudioCapturer* capturer,
            void* userData,
            OH_AudioStream_Event event);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio interrupt events.
     *
     * @since 10
     */
    int32_t (*OH_AudioCapturer_OnInterruptEvent)(
            OH_AudioCapturer* capturer,
            void* userData,
            OH_AudioInterrupt_ForceType type,
            OH_AudioInterrupt_Hint hint);

    /**
     * This function pointer will point to the callback function that
     * is used to handle audio error result.
     *
     * @since 10
     */
    int32_t (*OH_AudioCapturer_OnError)(
            OH_AudioCapturer* capturer,
            void* userData,
            OH_AudioStream_Result error);
} OH_AudioCapturer_Callbacks;

/**
 * @brief Defines reason for device changes of one audio stream.
 *
 * @since 11
 */
typedef enum {
    /* Unknown. */
    REASON_UNKNOWN = 0,
    /* New Device available. */
    REASON_NEW_DEVICE_AVAILABLE = 1,
    /* Old Device unavailable. Applications should consider to pause the audio playback when this reason is
    reported. */
    REASON_OLD_DEVICE_UNAVAILABLE = 2,
    /* Device is overrode by user or system. */
    REASON_OVERRODE = 3,
} OH_AudioStream_DeviceChangeReason;

/**
 * @brief Callback when the output device of an audio renderer changed.
 *
 * @param renderer AudioRenderer where this event occurs.
 * @param userData User data which is passed by user.
 * @param reason Indicates that why does the output device changes.
 * @since 11
 */
typedef void (*OH_AudioRenderer_OutputDeviceChangeCallback)(OH_AudioRenderer* renderer, void* userData,
    OH_AudioStream_DeviceChangeReason reason);

#ifdef __cplusplus
}

#endif

#endif // NATIVE_AUDIOSTREAM_BASE_H