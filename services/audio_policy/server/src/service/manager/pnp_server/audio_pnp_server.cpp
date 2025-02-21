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

#include "audio_pnp_server.h"

#include <cctype>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/netlink.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "hdf_base.h"
#include "hdf_device_object.h"
#include "osal_time.h"
#include "securec.h"
#include "audio_errors.h"
#include "audio_input_thread.h"
#include "audio_log.h"
#include "audio_socket_thread.h"

using namespace std;
namespace OHOS {
namespace AudioStandard {
static bool g_socketRunThread = false;
static bool g_inputRunThread = false;
#ifdef AUDIO_DOUBLE_PNP_DETECT
AudioEvent g_usbHeadset = {0};
#endif

static std::string GetAudioEventInfo(const AudioEvent audioEvent)
{
    int32_t ret;
    char event[AUDIO_PNP_INFO_LEN_MAX] = {0};
    if (audioEvent.eventType == AUDIO_EVENT_UNKNOWN || audioEvent.deviceType == AUDIO_DEVICE_UNKNOWN) {
        AUDIO_ERR_LOG("audio event is not updated");
        return event;
    }
    ret = snprintf_s(event, AUDIO_PNP_INFO_LEN_MAX, AUDIO_PNP_INFO_LEN_MAX - 1, "EVENT_TYPE=%u;DEVICE_TYPE=%u",
        audioEvent.eventType, audioEvent.deviceType);
    if (ret < 0) {
        AUDIO_ERR_LOG("snprintf_s failed");
        return event;
    }
    AUDIO_DEBUG_LOG("audio event info EVENT_TYPE = [%{public}u], DEVICE_TYPE = [%{public}u]",
        audioEvent.eventType, audioEvent.deviceType);
    return event;
}

bool AudioPnpServer::init(void)
{
    g_socketRunThread = true;
    g_inputRunThread = true;

    socketThread_ = std::make_unique<std::thread>(&AudioPnpServer::OpenAndReadWithSocket, this);
    pthread_setname_np(socketThread_->native_handle(), "OS_SocketEvent");
    inputThread_ = std::make_unique<std::thread>(&AudioPnpServer::OpenAndReadInput, this);
    pthread_setname_np(inputThread_->native_handle(), "OS_InputEvent");
    return true;
}

int32_t AudioPnpServer::RegisterPnpStatusListener(std::shared_ptr<AudioPnpDeviceChangeCallback> callback)
{
    {
        std::lock_guard<std::mutex> lock(pnpMutex_);
        pnpCallback_ = callback;
    }

    DetectAudioDevice();
    return SUCCESS;
}

int32_t AudioPnpServer::UnRegisterPnpStatusListener()
{
    std::lock_guard<std::mutex> lock(pnpMutex_);
    pnpCallback_ = nullptr;
    return SUCCESS;
}

void AudioPnpServer::OnPnpDeviceStatusChanged(const std::string &info)
{
    std::lock_guard<std::mutex> lock(pnpMutex_);
    if (pnpCallback_ != nullptr) {
        pnpCallback_->OnPnpDeviceStatusChanged(info);
    }
}

void AudioPnpServer::OpenAndReadInput()
{
    int32_t ret = -1;
    int32_t status = AudioInputThread::AudioPnpInputOpen();
    if (status != SUCCESS) {
        return;
    }

    do {
        ret = AudioInputThread::AudioPnpInputPollAndRead();
        if (ret != SUCCESS) {
            AUDIO_ERR_LOG("[AudioPnpInputPollAndRead] failed");
            return;
        }
        eventInfo_ = GetAudioEventInfo(AudioInputThread::audioInputEvent_);
        CHECK_AND_RETURN_LOG(!eventInfo_.empty(), "invalid input info");
        OnPnpDeviceStatusChanged(eventInfo_);
    } while (g_inputRunThread);
    return;
}

void AudioPnpServer::OpenAndReadWithSocket()
{
    ssize_t rcvLen;
    int32_t socketFd = -1;
    struct pollfd fd;
    char msg[UEVENT_MSG_LEN + 1] = {0};

    int32_t ret = AudioSocketThread::AudioPnpUeventOpen(&socketFd);
    if (ret != SUCCESS) {
        AUDIO_ERR_LOG("open audio pnp socket failed");
        return;
    }

    fd.fd = socketFd;
    fd.events = POLLIN | POLLERR;
    fd.revents = 0;

    while (g_socketRunThread) {
        if (poll(&fd, 1, -1) <= 0) {
            AUDIO_ERR_LOG("audio event poll fail %{public}d", errno);
            OsalMSleep(UEVENT_POLL_WAIT_TIME);
            continue;
        }

        if (((uint32_t)fd.revents & POLLIN) == POLLIN) {
            memset_s(&msg, sizeof(msg), 0, sizeof(msg));
            rcvLen = AudioSocketThread::AudioPnpReadUeventMsg(socketFd, msg, UEVENT_MSG_LEN);
            if (rcvLen <= 0) {
                continue;
            }
            bool status = AudioSocketThread::AudioPnpUeventParse(msg, rcvLen);
            if (!status) {
                continue;
            }
            eventInfo_ = GetAudioEventInfo(AudioSocketThread::audioSocketEvent_);
            CHECK_AND_RETURN_LOG(!eventInfo_.empty(), "invalid socket info");
            OnPnpDeviceStatusChanged(eventInfo_);
        } else if (((uint32_t)fd.revents & POLLERR) == POLLERR) {
            AUDIO_ERR_LOG("audio event poll error");
        }
    }
    close(socketFd);
    return;
}

#ifdef AUDIO_DOUBLE_PNP_DETECT
void AudioPnpServer::UpdateUsbHeadset()
{
    char pnpInfo[AUDIO_EVENT_INFO_LEN_MAX] = {0};
    int32_t ret;
    bool status = AudioSocketThread::IsUpdatePnpDeviceState(&g_usbHeadset);
    if (!status) {
        AUDIO_ERR_LOG("audio first pnp device[%{public}u] state[%{public}u] not need flush !",
            g_usbHeadset.deviceType, g_usbHeadset.eventType);
        return;
    }
    ret = snprintf_s(pnpInfo, AUDIO_EVENT_INFO_LEN_MAX, AUDIO_EVENT_INFO_LEN_MAX - 1, "EVENT_TYPE=%u;DEVICE_TYPE=%u",
        g_usbHeadset.eventType, g_usbHeadset.deviceType);
    if (ret < 0) {
        AUDIO_ERR_LOG("snprintf_s fail!");
        return;
    }
    AUDIO_DEBUG_LOG("g_usbHeadset.eventType [%{public}u], g_usbHeadset.deviceType [%{public}u]",
        g_usbHeadset.eventType, g_usbHeadset.deviceType);
    AudioSocketThread::UpdatePnpDeviceState(&g_usbHeadset);
    return;
}
#endif

void AudioPnpServer::DetectAudioDevice()
{
    int32_t ret;
    AudioEvent audioEvent = {0};

    OsalMSleep(AUDIO_DEVICE_WAIT_USB_ONLINE);
    ret = AudioSocketThread::DetectAnalogHeadsetState(&audioEvent);
    if ((ret == SUCCESS) && (audioEvent.eventType == AUDIO_DEVICE_ADD)) {
        AUDIO_INFO_LOG("audio detect analog headset");
        AudioSocketThread::UpdateDeviceState(audioEvent);

        eventInfo_ = GetAudioEventInfo(AudioSocketThread::audioSocketEvent_);
        CHECK_AND_RETURN_LOG(!eventInfo_.empty(), "invalid detect info");
        OnPnpDeviceStatusChanged(eventInfo_);
#ifndef AUDIO_DOUBLE_PNP_DETECT
        return;
#endif
    }
#ifdef AUDIO_DOUBLE_PNP_DETECT
    ret = AudioSocketThread::DetectUsbHeadsetState(&g_usbHeadset);
    if ((ret == SUCCESS) && (g_usbHeadset.eventType == AUDIO_DEVICE_ADD)) {
        AUDIO_INFO_LOG("audio detect usb headset");
        std::unique_ptr<std::thread> bootupThread_ = nullptr;
        bootupThread_ = std::make_unique<std::thread>(&AudioPnpServer::UpdateUsbHeadset, this);
        pthread_setname_np(bootupThread_->native_handle(), "OS_BootupEvent");
        OsalMSleep(AUDIO_DEVICE_WAIT_USB_EVENT_UPDATE);
        if (AudioSocketThread::audioSocketEvent_.eventType != AUDIO_EVENT_UNKNOWN &&
            AudioSocketThread::audioSocketEvent_.deviceType != AUDIO_DEVICE_UNKNOWN) {
            eventInfo_ = GetAudioEventInfo(AudioSocketThread::audioSocketEvent_);
            CHECK_AND_RETURN_LOG(!eventInfo_.empty(), "invalid detect info");
            OnPnpDeviceStatusChanged(eventInfo_);
        }
        if (bootupThread_ && bootupThread_->joinable()) {
            bootupThread_->join();
        }
    }
    return;
#else
    audioEvent.eventType = AUDIO_EVENT_UNKNOWN;
    audioEvent.deviceType = AUDIO_DEVICE_UNKNOWN;
    ret = AudioSocketThread::DetectUsbHeadsetState(&audioEvent);
    if ((ret == SUCCESS) && (audioEvent.eventType == AUDIO_DEVICE_ADD)) {
        AUDIO_INFO_LOG("audio detect usb headset");
        AudioSocketThread::UpdateDeviceState(audioEvent);
        eventInfo_ = GetAudioEventInfo(AudioSocketThread::audioSocketEvent_);
        CHECK_AND_RETURN_LOG(!eventInfo_.empty(), "invalid detect info");
        OnPnpDeviceStatusChanged(eventInfo_);
    }
#endif
}

void AudioPnpServer::StopPnpServer()
{
    g_socketRunThread = false;
    g_inputRunThread = false;
    if (socketThread_ && socketThread_->joinable()) {
        socketThread_->join();
    }

    if (inputThread_ && inputThread_->joinable()) {
        inputThread_->join();
    }
}
} // namespace AudioStandard
} // namespace OHOS