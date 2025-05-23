/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
 * @since 12
 * @version 1.0
 */

/**
 * @file native_audio_routing_manager.h
 *
 * @brief Declare audio routing manager related interfaces.
 *
 * This file interface is used for the creation of audioRoutingManager
 * as well as the registration and deregistration functions when the device connection status changes
 * as well as the release of pointer arrays storing device information.
 *
 * @library libohaudio.so
 * @syscap SystemCapability.Multimedia.Audio.Core
 * @kit AudioKit
 * @since 12
 * @version 1.0
 */

#ifndef NATIVE_AUDIO_ROUTING_MANAGER_H
#define NATIVE_AUDIO_ROUTING_MANAGER_H

#include "native_audio_device_base.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Declaring the audio routing manager.
 * The handle of audio routing manager used for routing and device related functions.
 *
 * @since 12
 */
typedef struct OH_AudioRoutingManager OH_AudioRoutingManager;

/**
 * @brief This function pointer will point to the callback function that
 * is used to return the changing audio device descriptors.
 * There may be more than one audio device descriptor returned.
 *
 * @param type the {@link OH_AudioDevice_ChangeType} is connect or disconnect.
 * @param audioDeviceDescriptorArray the {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value.
 * Do not release the audioDeviceDescriptorArray pointer separately
 * instead call {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array
 * when it is no use anymore.
 * @since 12
 */
typedef int32_t (*OH_AudioRoutingManager_OnDeviceChangedCallback) (
    OH_AudioDevice_ChangeType type,
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray
);

/**
 * @brief This type defines the callback function that is used to receive the audio devices' block status.
 *
 * @param audioDeviceDescriptorArray The {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value.
 * Do not release the audioDeviceDescriptorArray pointer separately instead of calling
 * {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array when it is no use anymore.
 * @param status The {@link OH_AudioDevice_BlockStatus} is the block status.
 * @param userData User data which is passed by user.
 * @since 13
 */
typedef void (*OH_AudioRoutingManager_OnDeviceBlockStatusCallback)(
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray,
    OH_AudioDevice_BlockStatus status,
    void *userData);

/**
 * @brief Query whether microphone block detection is supported on current device.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager} handle returned by
 * {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param supported query result.
 * @return Function result code:
 *     {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *     {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                    1.The param of audioRoutingManager is nullptr;
 *                                                    2.The param of supported is nullptr.
 * @since 13
 */
OH_AudioCommon_Result OH_AudioRoutingManager_IsMicBlockDetectionSupported(
    OH_AudioRoutingManager *audioRoutingManager,
    bool *supported);

/**
 * @brief Query the audio routing manager handle.
 * which should be set as the first parameter in routing releated functions.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager}
 * handle returned by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioManager_GetAudioRoutingManager(OH_AudioRoutingManager **audioRoutingManager);

/**
 * @brief Query the available devices according to the input deviceFlag.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager}
 * handle returned by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param deviceFlag the {@link OH_AudioDevice_DeviceFlag} which is used as
 * the filter parameter for selecting the target devices.
 * @param audioDeviceDescriptorArray the {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value
 * Do not release the audioDeviceDescriptorArray pointer separately
 * instead call {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array
 * when it is no use anymore.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of deviceFlag invalid;
 *                                                        3.The param of audioDeviceDescriptorArray is nullptr.
 *         {@link AUDIOCOMMON_RESULT_ERROR_NO_MEMORY} The param of audioDeviceDescriptorArray is nullptr.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_GetDevices(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioDevice_Flag deviceFlag,
    OH_AudioDeviceDescriptorArray **audioDeviceDescriptorArray);

/**
 * @brief Get available devices by device usage.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager} handle returned
 * by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param deviceUsage the device usage.
 * @param audioDeviceDescriptorArray the {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value
 * Do not release the audioDeviceDescriptorArray pointer separately
 * instead call {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array
 * when it is no use anymore.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of deviceUsage invalid;
 *                                                        3.The param of audioDeviceDescriptorArray is nullptr.
 *         {@link AUDIOCOMMON_RESULT_ERROR_NO_MEMORY} No memory error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_GetAvailableDevices(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioDevice_Usage deviceUsage, OH_AudioDeviceDescriptorArray **audioDeviceDescriptorArray);

/**
 * @brief Get preferred ouput devices by audio usage.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager} handle returned
 * by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param streamUsage the audio stream usage.
 * @param audioDeviceDescriptorArray the {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value
 * Do not release the audioDeviceDescriptorArray pointer separately
 * instead call {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array
 * when it is no use anymore.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of streamUsage invalid;
 *                                                        3.The param of audioDeviceDescriptorArray is nullptr.
 *         {@link AUDIOCOMMON_RESULT_ERROR_NO_MEMORY} No memory error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_GetPreferredOutputDevice(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioStream_Usage streamUsage, OH_AudioDeviceDescriptorArray **audioDeviceDescriptorArray);

/**
 * @brief Get preferred input devices by audio source type.
 * @param audioRoutingManager the {@link OH_AudioRoutingManager} handle returned
 * by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param sourceType the audio source type.
 * @param audioDeviceDescriptorArray the {@link OH_AudioDeviceDescriptorArray}
 * pointer variable which will be set the audio device descriptors value
 * Do not release the audioDeviceDescriptorArray pointer separately
 * instead call {@link OH_AudioRoutingManager_ReleaseDevices} to release the DeviceDescriptor array
 * when it is no use anymore.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of sourceType invalid;
 *                                                        3.The param of audioDeviceDescriptorArray is nullptr.
 *         {@link AUDIOCOMMON_RESULT_ERROR_NO_MEMORY} No memory error.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_GetPreferredInputDevice(OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioStream_SourceType sourceType, OH_AudioDeviceDescriptorArray **audioDeviceDescriptorArray);
/**
 * @brief Register the device change callback of the audio routing manager.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager}
 * handle returned by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param deviceFlag the {@link OH_AudioDevice_DeviceFlag} which is used to register callback.
 * @param callback the {@link OH_AudioRoutingManager_OnDeviceChangedCallback}
 * Callback function which will be called when devices changed.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of deviceFlag invalid;
 *                                                        3.The param of callback is nullptr.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_RegisterDeviceChangeCallback(
    OH_AudioRoutingManager *audioRoutingManager, OH_AudioDevice_Flag deviceFlag,
    OH_AudioRoutingManager_OnDeviceChangedCallback callback);

/**
 * @brief Unregister the device change callback of the audio routing manager.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager}
 * handle returned by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param callback the {@link OH_AudioRoutingManager_OnDeviceChangedCallback}
 * Callback function which will be called when devices changed.
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of callback is nullptr.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_UnregisterDeviceChangeCallback(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioRoutingManager_OnDeviceChangedCallback callback);

/**
 * @brief Set the microphone block status callback. The caller will receive the callback only when it is recording
 * and the used microphones' block status have changed. Currently, block detecting is only support for microphones
 * located on the local device.
 *
 * @param audioRoutingManager The {@link OH_AudioRoutingManager} handle returned by
 * {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param callback The function pointer will point to the callback function that is used to receive the block status.
 * @param userData User data which is passed by user.
 * @return Function result code:
 *     {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *     {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                    1.The param of audioRoutingManager is nullptr;
 *                                                    2.The param of callback is nullptr.
 * @since 13
 */
OH_AudioCommon_Result OH_AudioRoutingManager_SetMicBlockStatusCallback(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioRoutingManager_OnDeviceBlockStatusCallback callback,
    void *userData);

/**
 * @brief Release the audio device descriptor array object.
 *
 * @param audioRoutingManager the {@link OH_AudioRoutingManager}
 * handle returned by {@link OH_AudioManager_GetAudioRoutingManager}.
 * @param audioDeviceDescriptorArray Audio device descriptors should be released.
 * and get from {@link OH_AudioRoutingManager_GetDevices}
 * @return Function result code:
 *         {@link AUDIOCOMMON_RESULT_SUCCESS} If the execution is successful.
 *         {@link AUDIOCOMMON_RESULT_ERROR_INVALID_PARAM}:
 *                                                        1.The param of audioRoutingManager is nullptr;
 *                                                        2.The param of audioDeviceDescriptorArray is nullptr.
 * @since 12
 */
OH_AudioCommon_Result OH_AudioRoutingManager_ReleaseDevices(
    OH_AudioRoutingManager *audioRoutingManager,
    OH_AudioDeviceDescriptorArray *audioDeviceDescriptorArray);
#ifdef __cplusplus
}
#endif
/** @} */
#endif // NATIVE_AUDIO_ROUTING_MANAGER_H
