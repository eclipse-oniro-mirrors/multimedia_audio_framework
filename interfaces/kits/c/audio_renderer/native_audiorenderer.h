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
 * @file native_audiorenderer.h
 *
 * @brief Declare audio stream related interfaces for output type.
 *
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @since 10
 * @version 1.0
 */

#ifndef NATIVE_AUDIORENDERER_H
#define NATIVE_AUDIORENDERER_H

#include <time.h>
#include "native_audiostream_base.h"
#include "native_audio_device_base.h"
#include "multimedia/native_audio_channel_layout.h"
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Request to release the renderer stream.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_Release(OH_AudioRenderer* renderer);

/*
 * Request to start the renderer stream.
 *
 * @since 10
 *
 * @param renderer reference created by OH_AudioStreamBuilder
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_Start(OH_AudioRenderer* renderer);

/*
 * Request to pause the renderer stream.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_Pause(OH_AudioRenderer* renderer);

/*
 * Request to stop renderer stream.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_Stop(OH_AudioRenderer* renderer);

/*
 * Request to flush the renderer stream.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_Flush(OH_AudioRenderer* renderer);

/*
 * Query the current state of the renderer client.
 *
 * This function will return the renderer state without updating the state.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param state Pointer to a variable that will be set for the state value.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetCurrentState(OH_AudioRenderer* renderer,
    OH_AudioStream_State* state);

/*
 * Query the sample rate value of the renderer client
 *
 * This function will return the renderer sample rate value without updating the state.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param rate The state value to be updated
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetSamplingRate(OH_AudioRenderer* renderer, int32_t* rate);

/*
 * Query the stream id of the renderer client.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param stramId Pointer to a variable that will be set for the stream id.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetStreamId(OH_AudioRenderer* renderer, uint32_t* streamId);

/*
 * Query the channel count of the renderer client.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param channelCount Pointer to a variable that will be set for the channel count.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetChannelCount(OH_AudioRenderer* renderer, int32_t* channelCount);

/*
 * Query the sample format of the renderer client.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param sampleFormat Pointer to a variable that will be set for the sample format.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetSampleFormat(OH_AudioRenderer* renderer,
    OH_AudioStream_SampleFormat* sampleFormat);

/*
 * Query the latency mode of the renderer client.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param latencyMode Pointer to a variable that will be set for the latency mode.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetLatencyMode(OH_AudioRenderer* renderer,
    OH_AudioStream_LatencyMode* latencyMode);
/*
 * Query the renderer info of the renderer client.
 *
 * The rendere info includes {@link OH_AudioStream_Usage} value.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param usage Pointer to a variable that will be set for the stream usage.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetRendererInfo(OH_AudioRenderer* renderer,
    OH_AudioStream_Usage* usage);

/*
 * Query the encoding type of the renderer client.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param encodingType Pointer to a variable that will be set for the encoding type.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetEncodingType(OH_AudioRenderer* renderer,
    OH_AudioStream_EncodingType* encodingType);

/*
 * Query the the number of frames that have been written since the stream was created.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param frames Pointer to a variable that will be set for the frame count number.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetFramesWritten(OH_AudioRenderer* renderer, int64_t* frames);

/*
 * Query the the time at which a particular frame was presented.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param clockId {@link #CLOCK_MONOTONIC}
 * @param framePosition Pointer to a variable to receive the position
 * @param timestamp Pointer to a variable to receive the timestamp
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetTimestamp(OH_AudioRenderer* renderer,
    clockid_t clockId, int64_t* framePosition, int64_t* timestamp);

/*
 * Query the timestamp at which a particular frame was presented in clock monotonic timebase, the frame at
 * the returned position was just committed to hardware. This is often used in video synchronization and
 * recording stream alignment.
 *
 * Position is 0 and timestamp is fixed until stream really runs and frame is committed. Position will
 * also be reset while flush function is called. When a audio route change happens, like in device or output
 * type change situations, the position may also be reset but timestamp remains monotonically increasing.
 * So it is better to use the values until they becomes regularly after the change.
 * This interface also adapts to playback speed change. For example, the increseing speed for position
 * will be double for 2x speed playback.
 *
 * @since 15
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param framePosition Pointer to a variable to receive the position
 * @param timestamp Pointer to a variable to receive the timestamp
 * @return Function result code:
 *         {@link AUDIOSTREAM_SUCCESS} If the execution is successful.
 *         {@link AUDIOSTREAM_ERROR_INVALID_PARAM}:
 *                                         1.The param of renderer is nullptr;
 *                                         2.The param of framePosition or timestamp is nullptr;
 *         {@link AUDIOSTREAM_ERROR_ILLEGAL_STATE}:
 *                                         1.Stopped state is illegal for getting audio timestamp.
 *         {@link AUDIOSTREAM_ERROR_SYSTEM}:
 *                                         1.Crash or blocking occurs in system process.
 *                                         2.Other unexpected error from internal system.
 */
OH_AudioStream_Result OH_AudioRenderer_GetAudioTimestampInfo(OH_AudioRenderer* renderer,
    int64_t* framePosition, int64_t* timestamp);

/*
 * Query the frame size in callback, it is a fixed length that the stream want to be filled for each callback.
 *
 * @since 10
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param frameSize Pointer to a variable that will be set for the frame size.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetFrameSizeInCallback(OH_AudioRenderer* renderer, int32_t* frameSize);

/*
* Query the playback speed of the stream client
*
* @since 11
*
* @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
* @param speed Pointer to a variable to receive the playback speed.
* @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
*/
OH_AudioStream_Result OH_AudioRenderer_GetSpeed(OH_AudioRenderer* renderer, float* speed);

/*
* Set the playback speed of the stream client
*
* @since 11
*
* @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
* @param speed The playback speed, form 0.25 to 4.0.
* @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
*/
OH_AudioStream_Result OH_AudioRenderer_SetSpeed(OH_AudioRenderer* renderer, float speed);

/**
 * @brief Set mark position on current renderer. Calling this function will overwrite the mark postion which has already
 * set.
 *
 * @since 12
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param samplePos Mark position in samples.
 * @param callback Callback used when the samplePos has reached.
 * @param userData User data which is passed by user.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_SetMarkPosition(OH_AudioRenderer* renderer, uint32_t samplePos,
    OH_AudioRenderer_OnMarkReachedCallback callback, void* userData);

/**
 * @brief Cancel mark which has set by {@link #OH_AudioRenderer_SetMarkPosition}.
 *
 * @since 12
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_CancelMark(OH_AudioRenderer* renderer);

/**
 * Set volume of current renderer.
 *
 * @since 12
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param volume Volume to set which changes from 0.0 to 1.0.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_SetVolume(OH_AudioRenderer* renderer, float volume);

/**
 * Changes the volume with ramp for a duration.
 *
 * @since 12
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param volume Volume to set which changes from 0.0 to 1.0.
 * @param durationMs Duration for volume ramp, in millisecond.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_SetVolumeWithRamp(OH_AudioRenderer* renderer, float volume, int32_t durationMs);

/**
 * @brief Sets the loudness gain of current renderer.
 * The default loudness gain is 0.0dB. The stream usage of the audio renderer must be
 * {@link OH_AudioStream_Usage#AUDIOSTREAM_USAGE_MUSIC}, {@link OH_AudioStream_Usage#AUDIOSTREAM_USAGE_MOVIE}
 * or {@link OH_AudioStream_Usage#AUDIOSTREAM_USAGE_AUDIOBOOK}.
 * The latency mode of the audio renderer must be {@link OH_AudioStream_LatencyMode#AUDIOSTREAM_LATENCY_MODE_NORMAL}.
 * If AudioRenderer is played through the high-resolution pipe, this operation is not supported.
 *
 * @param renderer AudioRender created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param loudnessGain Loudness gain to set which changes from -90.0 to 24.0, expressing in dB.
 * @return Function result code:
 *         {@link AUDIOSTREAM_SUCCESS} If the execution is successful.
 *         {@link AUDIOSTREAM_ERROR_INVALID_PARAM}:
 *                                                 1.The param of renderer is nullptr or not supported to set gain;
 *                                                 2.The param of loudnessGain is invalid.
 * @since 20
 */
OH_AudioStream_Result OH_AudioRenderer_SetLoudnessGain(OH_AudioRenderer* renderer, float loudnessGain);

/**
 * @brief Get the loudness gain of current renderer.
 *
 * @param renderer AudioRender created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param loudnessGain Pointer to a variable to receive the loudness gain.
 * @return Function result code:
 *         {@link AUDIOSTREAM_SUCCESS} If the execution is successful.
 *         {@link AUDIOSTREAM_ERROR_INVALID_PARAM}:
 *                                                 1.The param of renderer is nullptr;
 *                                                 2.The param of loudnessGain is nullptr.
 * @since 20
 */
OH_AudioStream_Result OH_AudioRenderer_GetLoudnessGain(OH_AudioRenderer* renderer, float* loudnessGain);

/**
 * Get Volume of current renderer.
 *
 * @since 12
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param volume Pointer to a variable to receive the volume.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetVolume(OH_AudioRenderer* renderer, float* volume);

/**
 * @brief Gets the underflow count on this stream.
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param count Pointer to a variable to receive the underflow count number.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioStream_Result OH_AudioRenderer_GetUnderflowCount(OH_AudioRenderer* renderer, uint32_t* count);

/**
 * @brief Query the channel layout of the renderer client.
 *
 * @since 12
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param channelLayout Pointer to a variable to receive the channel layout
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetChannelLayout(OH_AudioRenderer* renderer,
    OH_AudioChannelLayout* channelLayout);

/**
 * @brief Query current audio effect mode.
 *
 * @since 12
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param effectMode Pointer to a variable to receive current audio effect mode
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetEffectMode(OH_AudioRenderer* renderer,
    OH_AudioStream_AudioEffectMode* effectMode);

/**
 * @brief Set current audio effect mode.
 *
 * @since 12
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer()
 * @param effectMode Audio effect mode that will be set for the stream
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_SetEffectMode(OH_AudioRenderer* renderer,
    OH_AudioStream_AudioEffectMode effectMode);

/**
 * @brief Query the playback capture privacy of the renderer client.
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param privacy The playback capture privacy of the renderer client.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 * @since 12
 */
OH_AudioStream_Result OH_AudioRenderer_GetRendererPrivacy(OH_AudioRenderer* renderer,
    OH_AudioStream_PrivacyType* privacy);

/*
 * Set silent and mix with other stream for this stream.
 *
 * @since 12
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param on The silent and mix with other stream status.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_SetSilentModeAndMixWithOthers(
    OH_AudioRenderer* renderer, bool on);

/*
 * Query silent and mix with other stream status for this stream.
 *
 * @since 12
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param on Pointer to the silent and mix with other stream status.
 * @return {@link #AUDIOSTREAM_SUCCESS} or an undesired error.
 */
OH_AudioStream_Result OH_AudioRenderer_GetSilentModeAndMixWithOthers(
    OH_AudioRenderer* renderer, bool* on);

/**
 * @brief Temporarily changes the current audio device
 *        This function applys on audiorenderers whose StreamUsage are
 *        STREAM_USAGE_VOICE_COMMUNICATIN/STREAM_USAGE_VIDEO_COMMUNICATION/STREAM_USAGE_VOICE_MESSAGE.
 *        Setting the device will ony takes effect if no other accessory such as headphoes are in use.
 *
 * @param renderer Renderer generated by OH_AudioStreamBuilder_GenerateRenderer()
 * @param deviceType The target device. The available deviceTypes are:
 *                                             EARPIECE: Built-in earpiece
 *                                             SPEAKER: Built-in speaker
 *                                             DEFAULT: System default output device
 * @return result code for this function.
 *         {@link #AUDIOSTREAM_SUCCESS} succeed in setting the default output device
 *         {@link #AUDIOSTREAM_ERROR_INVALID_PARAM}:
 *                                                 1.The param of renderer is nullptr;
 *                                                 2.The param of deviceType is not valid
 *         {@link #AUDIOSTREAM_ERROR_ILLEGAL_STATE} This audiorenderer can not reset the output device
 *         {@link #AUDIOSTREAM_ERROR_SYSTEM} system error when calling this function.
 * @since 12
 */
OH_AudioStream_Result OH_AudioRenderer_SetDefaultOutputDevice(
    OH_AudioRenderer* renderer, OH_AudioDevice_Type deviceType);

/**
 * @brief Callback function of interrupt event on AudioRenderer.
 *
 * This function is similar with OH_AudioRenderer_Callbacks_Struct.OH_AudioRenderer_OnInterruptEvent.
 *
 * @param renderer AudioRenderer where this callback occurs.
 * @param userData User data which is passed by user.
 * @param type Force type of this interrupt event.
 * @param hint Hint of this interrupt event.
 * @see OH_AudioRenderer_Callbacks_Struct.OH_AudioRenderer_OnInterruptEvent.
 * @since 18
 */
typedef void (*OH_AudioRenderer_OnInterruptCallback)(OH_AudioRenderer* renderer, void* userData,
    OH_AudioInterrupt_ForceType type, OH_AudioInterrupt_Hint hint);

/**
 * @brief Callback function of error on AudioRenderer.
 *
 * This function is similar with OH_AudioRenderer_Callbacks_Struct.OH_AudioRenderer_OnError.
 *
 * @param renderer AudioRenderer where this callback occurs.
 * @param userData User data which is passed by user.
 * @param error Error while using AudioRenderer.
 * @see OH_AudioRenderer_Callbacks_Struct.OH_AudioRenderer_OnError
 * @since 18
 */
typedef void (*OH_AudioRenderer_OnErrorCallback)(OH_AudioRenderer* renderer, void* userData,
    OH_AudioStream_Result error);

/**
 * @brief Gets audio renderer running status, check if it works in fast status.
 *
 * @param renderer Reference created by OH_AudioStreamBuilder_GenerateRenderer.
 * @param status Pointer to a variable to receive the status.
 * @return
 *     {@link AUDIOSTREAM_SUCCESS} if the execution is successful.
 *     {@link AUDIOSTREAM_ERROR_INVALID_PARAM} the param of renderer is nullptr.
 *     {@link AUDIOSTREAM_ERROR_ILLEGAL_STATE} function called in invalid state, only available before release state.
 * @since 20
 */
OH_AudioStream_Result OH_AudioRenderer_GetFastStatus(OH_AudioRenderer* renderer,
    OH_AudioStream_FastStatus* status);

/**
 * @brief Callback function of fast status change event for audio renderer.
 *
 * @param renderer Pointer to an audio renderer instance for which this callback occurs.
 * @param userData Userdata which is passed by register.
 * @param status Current fast status.
 * @since 20
 */
typedef void (*OH_AudioRenderer_OnFastStatusChange)(
    OH_AudioRenderer* renderer,
    void* userData,
    OH_AudioStream_FastStatus status
);

#ifdef __cplusplus
}
#endif
#endif // NATIVE_AUDIORENDERER_H
