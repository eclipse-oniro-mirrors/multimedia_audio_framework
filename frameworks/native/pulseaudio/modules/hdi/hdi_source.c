/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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


#include <config.h>
#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/util.h>
#include <pulse/xmalloc.h>
#include <pulsecore/core.h>
#include <pulsecore/log.h>
#include <pulsecore/memchunk.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/thread.h>

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "audio_types.h"
#include "audio_manager.h"

#include "audio_log.h"
#include "audio_source_type.h"
#include "capturer_source_adapter.h"

#define DEFAULT_SOURCE_NAME "hdi_input"
#define DEFAULT_DEVICE_CLASS "primary"
#define DEFAULT_AUDIO_DEVICE_NAME "Internal Mic"
#define DEFAULT_DEVICE_NETWORKID "LocalDevice"

#define DEFAULT_BUFFER_SIZE (1024 * 16)
#define MAX_VOLUME_VALUE 15.0
#define DEFAULT_LEFT_VOLUME MAX_VOLUME_VALUE
#define DEFAULT_RIGHT_VOLUME MAX_VOLUME_VALUE
#define MAX_LATENCY_USEC (PA_USEC_PER_SEC * 2)
#define MIN_LATENCY_USEC 500
#define AUDIO_POINT_NUM  1024
#define AUDIO_FRAME_NUM_IN_BUF 30

const char *DEVICE_CLASS_REMOTE = "remote";

struct Userdata {
    pa_core *core;
    pa_module *module;
    pa_source *source;
    pa_thread *thread;
    pa_thread_mq thread_mq;
    pa_rtpoll *rtpoll;
    uint32_t buffer_size;
    uint32_t open_mic_speaker;
    pa_usec_t block_usec;
    pa_usec_t timestamp;
    SourceAttr attrs;
    bool IsCapturerStarted;
    struct CapturerSourceAdapter *sourceAdapter;
};

static int PaHdiCapturerInit(struct Userdata *u);
static void PaHdiCapturerExit(struct Userdata *u);

static char *GetStateInfo(pa_source_state_t state)
{
    switch (state) {
        case PA_SOURCE_INVALID_STATE:
            return "INVALID";
        case PA_SOURCE_RUNNING:
            return "RUNNING";
        case PA_SOURCE_IDLE:
            return "IDLE";
        case PA_SOURCE_SUSPENDED:
            return "SUSPENDED";
        case PA_SOURCE_INIT:
            return "INIT";
        case PA_SOURCE_UNLINKED:
            return "UNLINKED";
        default:
            return "error state";
    }
}

static void UserdataFree(struct Userdata *u)
{
    pa_assert(u);
    if (u->source) {
        pa_source_unlink(u->source);
    }

    if (u->thread) {
        pa_asyncmsgq_send(u->thread_mq.inq, NULL, PA_MESSAGE_SHUTDOWN, NULL, 0, NULL);
        pa_thread_free(u->thread);
    }

    pa_thread_mq_done(&u->thread_mq);

    if (u->source) {
        pa_source_unref(u->source);
    }

    if (u->rtpoll) {
        pa_rtpoll_free(u->rtpoll);
    }

    if (u->sourceAdapter) {
        u->sourceAdapter->CapturerSourceStop(u->sourceAdapter->wapper);
        u->sourceAdapter->CapturerSourceDeInit(u->sourceAdapter->wapper);
        UnLoadSourceAdapter(u->sourceAdapter);
    }

    pa_xfree(u);
}

static int SourceProcessMsg(pa_msgobject *o, int code, void *data, int64_t offset, pa_memchunk *chunk)
{
    struct Userdata *u = PA_SOURCE(o)->userdata;
    pa_assert(u);

    switch (code) {
        case PA_SOURCE_MESSAGE_GET_LATENCY: {
            pa_usec_t now;
            now = pa_rtclock_now();
            *((int64_t*)data) = (int64_t)now - (int64_t)u->timestamp;
            return 0;
        }
        default: {
            pa_log("SourceProcessMsg default case");
            return pa_source_process_msg(o, code, data, offset, chunk);
        }
    }
}

/* Called from the IO thread. */
static int SourceSetStateInIoThreadCb(pa_source *s, pa_source_state_t newState,
    pa_suspend_cause_t newSuspendCause)
{
    struct Userdata *u = NULL;
    pa_assert(s);
    pa_assert_se(u = s->userdata);
    AUDIO_INFO_LOG("Source[%{public}s] state change:[%{public}s]-->[%{public}s]",
        GetDeviceClass(u->sourceAdapter->deviceClass), GetStateInfo(s->thread_info.state), GetStateInfo(newState));

    if ((s->thread_info.state == PA_SOURCE_SUSPENDED || s->thread_info.state == PA_SOURCE_INIT) &&
        PA_SOURCE_IS_OPENED(newState)) {
        u->timestamp = pa_rtclock_now();
        if (newState == PA_SOURCE_RUNNING && !u->IsCapturerStarted) {
            if (u->sourceAdapter->CapturerSourceStart(u->sourceAdapter->wapper)) {
                AUDIO_ERR_LOG("HDI capturer start failed");
                return -PA_ERR_IO;
            }
            u->IsCapturerStarted = true;
            AUDIO_DEBUG_LOG("Successfully started HDI capturer");
        }
    } else if (s->thread_info.state == PA_SOURCE_IDLE) {
        if (newState == PA_SOURCE_SUSPENDED) {
            if (u->IsCapturerStarted) {
                u->sourceAdapter->CapturerSourceStop(u->sourceAdapter->wapper);
                u->IsCapturerStarted = false;
                AUDIO_DEBUG_LOG("Stopped HDI capturer");
            }
        } else if (newState == PA_SOURCE_RUNNING && !u->IsCapturerStarted) {
            AUDIO_DEBUG_LOG("Idle to Running starting HDI capturing device");
            if (u->sourceAdapter->CapturerSourceStart(u->sourceAdapter->wapper)) {
                AUDIO_ERR_LOG("Idle to Running HDI capturer start failed");
                return -PA_ERR_IO;
            }
            u->IsCapturerStarted = true;
            AUDIO_DEBUG_LOG("Idle to Running: Successfully reinitialized HDI renderer");
        }
    }

    return 0;
}

static int GetCapturerFrameFromHdi(pa_memchunk *chunk, const struct Userdata *u)
{
    uint64_t requestBytes;
    uint64_t replyBytes = 0;
    void *p = NULL;

    chunk->length = u->buffer_size;
    AUDIO_DEBUG_LOG("HDI Source: chunk.length = u->buffer_size: %{public}zu", chunk->length);
    chunk->memblock = pa_memblock_new(u->core->mempool, chunk->length);
    pa_assert(chunk->memblock);
    p = pa_memblock_acquire(chunk->memblock);
    pa_assert(p);

    requestBytes = pa_memblock_get_length(chunk->memblock);
    u->sourceAdapter->CapturerSourceFrame(u->sourceAdapter->wapper, (char *)p, (uint64_t)requestBytes, &replyBytes);

    pa_memblock_release(chunk->memblock);
    AUDIO_DEBUG_LOG("HDI Source: request bytes: %{public}" PRIu64 ", replyBytes: %{public}" PRIu64,
            requestBytes, replyBytes);
    if (replyBytes > requestBytes) {
        AUDIO_ERR_LOG("HDI Source: Error replyBytes > requestBytes. Requested data Length: "
                "%{public}" PRIu64 ", Read: %{public}" PRIu64 " bytes", requestBytes, replyBytes);
        pa_memblock_unref(chunk->memblock);
        return 0;
    }

    if (replyBytes == 0) {
        AUDIO_ERR_LOG("HDI Source: Failed to read, Requested data Length: %{public}" PRIu64 " bytes,"
                " Read: %{public}" PRIu64 " bytes", requestBytes, replyBytes);
        pa_memblock_unref(chunk->memblock);
        return 0;
    }

    chunk->index = 0;
    chunk->length = replyBytes;
    pa_source_post(u->source, chunk);
    pa_memblock_unref(chunk->memblock);

    return 0;
}

static void ThreadFuncCapturerTimer(void *userdata)
{
    struct Userdata *u = userdata;
    bool timerElapsed = false;

    pa_assert(u);

    if (u->core->realtime_scheduling) {
        pa_thread_make_realtime(u->core->realtime_priority);
    }

    pa_thread_mq_install(&u->thread_mq);
    u->timestamp = pa_rtclock_now();
    AUDIO_DEBUG_LOG("HDI Source: u->timestamp : %{public}" PRIu64, u->timestamp);

    while (true) {
        int ret = 0;

        if (PA_SOURCE_IS_OPENED(u->source->thread_info.state) && u->IsCapturerStarted) {
            pa_memchunk chunk;
            pa_usec_t now;

            now = pa_rtclock_now();
            AUDIO_DEBUG_LOG("HDI Source: now: %{public}" PRIu64 " timerElapsed: %{public}d", now, timerElapsed);

            if (timerElapsed) {
                chunk.length = pa_usec_to_bytes(now - u->timestamp, &u->source->sample_spec);
                if (chunk.length > 0) {
                    ret = GetCapturerFrameFromHdi(&chunk, u);
                    if (ret != 0) {
                        break;
                    }

                    u->timestamp += pa_bytes_to_usec(chunk.length, &u->source->sample_spec);
                    AUDIO_DEBUG_LOG("HDI Source: new u->timestamp : %{public}" PRIu64, u->timestamp);
                }
            }

            pa_rtpoll_set_timer_absolute(u->rtpoll, u->timestamp + u->block_usec);
        } else {
            pa_rtpoll_set_timer_disabled(u->rtpoll);
            AUDIO_DEBUG_LOG("HDI Source: pa_rtpoll_set_timer_disabled done ");
        }

        /* Hmm, nothing to do. Let's sleep */
        if ((ret = pa_rtpoll_run(u->rtpoll)) < 0) {
            /* If this was no regular exit from the loop we have to continue
            * processing messages until we received PA_MESSAGE_SHUTDOWN */
            AUDIO_ERR_LOG("HDI Source: pa_rtpoll_run ret:%{public}d failed", ret);
            pa_asyncmsgq_post(u->thread_mq.outq, PA_MSGOBJECT(u->core), PA_CORE_MESSAGE_UNLOAD_MODULE, u->module,
                0, NULL, NULL);
            pa_asyncmsgq_wait_for(u->thread_mq.inq, PA_MESSAGE_SHUTDOWN);
            return;
        }

        timerElapsed = pa_rtpoll_timer_elapsed(u->rtpoll);

        if (ret == 0) {
            return;
        }
    }
}

static int PaHdiCapturerInit(struct Userdata *u)
{
    int ret;
    ret = u->sourceAdapter->CapturerSourceInit(u->sourceAdapter->wapper, &u->attrs);
    if (ret != 0) {
        AUDIO_ERR_LOG("Audio capturer init failed!");
        return ret;
    }

    // No start test for remote device.
    if (strcmp(GetDeviceClass(u->sourceAdapter->deviceClass), DEVICE_CLASS_REMOTE)) {
        ret = u->sourceAdapter->CapturerSourceStart(u->sourceAdapter->wapper);
        if (ret != 0) {
            AUDIO_ERR_LOG("Audio capturer start failed!");
            goto fail;
        }
    }

    u->IsCapturerStarted = true;
    return ret;

fail:
    PaHdiCapturerExit(u);
    return ret;
}

static void PaHdiCapturerExit(struct Userdata *u)
{
    u->sourceAdapter->CapturerSourceStop(u->sourceAdapter->wapper);
    u->sourceAdapter->CapturerSourceDeInit(u->sourceAdapter->wapper);
}

static int PaSetSourceProperties(pa_module *m, pa_modargs *ma, const pa_sample_spec *ss, const pa_channel_map *map,
    struct Userdata *u)
{
    pa_source_new_data data;

    if (pa_modargs_get_value_u32(ma, "buffer_size", &u->buffer_size) < 0) {
        AUDIO_INFO_LOG("Failed to parse buffer_size argument.");
        u->buffer_size = DEFAULT_BUFFER_SIZE;
    }
    pa_source_new_data_init(&data);
    data.driver = __FILE__;
    data.module = m;
    pa_source_new_data_set_name(&data, pa_modargs_get_value(ma, "source_name", DEFAULT_SOURCE_NAME));
    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_STRING,
        (u->attrs.adapterName ? u->attrs.adapterName : DEFAULT_AUDIO_DEVICE_NAME));
    pa_proplist_setf(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "HDI source is %s",
        (u->attrs.adapterName ? u->attrs.adapterName : DEFAULT_AUDIO_DEVICE_NAME));
    pa_source_new_data_set_sample_spec(&data, ss);
    pa_source_new_data_set_channel_map(&data, map);
    pa_proplist_setf(data.proplist, PA_PROP_DEVICE_BUFFERING_BUFFER_SIZE, "%lu", (unsigned long)u->buffer_size);

    if (pa_modargs_get_proplist(ma, "source_properties", data.proplist, PA_UPDATE_REPLACE) < 0) {
        AUDIO_ERR_LOG("Invalid properties");
        pa_source_new_data_done(&data);
        return -1;
    }

    u->source = pa_source_new(m->core, &data, PA_SOURCE_HARDWARE | PA_SOURCE_LATENCY);
    pa_source_new_data_done(&data);

    if (!u->source) {
        AUDIO_ERR_LOG("Failed to create source object");
        return -1;
    }

    u->source->parent.process_msg = SourceProcessMsg;
    u->source->set_state_in_io_thread = SourceSetStateInIoThreadCb;
    u->source->userdata = u;

    pa_source_set_asyncmsgq(u->source, u->thread_mq.inq);
    pa_source_set_rtpoll(u->source, u->rtpoll);

    u->block_usec = pa_bytes_to_usec(u->buffer_size, &u->source->sample_spec);
    pa_source_set_latency_range(u->source, 0, u->block_usec);
    u->source->thread_info.max_rewind = pa_usec_to_bytes(u->block_usec, &u->source->sample_spec);

    return 0;
}

static enum AudioFormat ConvertToHDIAudioFormat(pa_sample_format_t format)
{
    enum AudioFormat hdiAudioFormat;
    switch (format) {
        case PA_SAMPLE_U8:
            hdiAudioFormat = AUDIO_FORMAT_TYPE_PCM_8_BIT;
            break;
        case PA_SAMPLE_S16LE:
        case PA_SAMPLE_S16BE:
            hdiAudioFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
        case PA_SAMPLE_S24LE:
        case PA_SAMPLE_S24BE:
            hdiAudioFormat = AUDIO_FORMAT_TYPE_PCM_24_BIT;
            break;
        case PA_SAMPLE_S32LE:
        case PA_SAMPLE_S32BE:
            hdiAudioFormat = AUDIO_FORMAT_TYPE_PCM_32_BIT;
            break;
        default:
            hdiAudioFormat = AUDIO_FORMAT_TYPE_PCM_16_BIT;
            break;
    }

    return hdiAudioFormat;
}

static enum AudioInputType ConvertToHDIAudioInputType(int32_t currSourceType)
{
    enum AudioInputType hdiAudioInputType;
    switch (currSourceType) {
        case SOURCE_TYPE_INVALID:
            hdiAudioInputType = AUDIO_INPUT_DEFAULT_TYPE;
            break;
        case SOURCE_TYPE_MIC:
        case SOURCE_TYPE_PLAYBACK_CAPTURE:
        case SOURCE_TYPE_ULTRASONIC:
            hdiAudioInputType = AUDIO_INPUT_MIC_TYPE;
            break;
        case SOURCE_TYPE_WAKEUP:
            hdiAudioInputType = AUDIO_INPUT_SPEECH_WAKEUP_TYPE;
            break;
        case SOURCE_TYPE_VOICE_COMMUNICATION:
            hdiAudioInputType = AUDIO_INPUT_VOICE_COMMUNICATION_TYPE;
            break;
        case SOURCE_TYPE_VOICE_RECOGNITION:
            hdiAudioInputType = AUDIO_INPUT_VOICE_RECOGNITION_TYPE;
            break;
        default:
            hdiAudioInputType = AUDIO_INPUT_MIC_TYPE;
            break;
    }
    return hdiAudioInputType;
}

static bool GetEndianInfo(pa_sample_format_t format)
{
    bool isBigEndian = false;
    switch (format) {
        case PA_SAMPLE_S16BE:
        case PA_SAMPLE_S24BE:
        case PA_SAMPLE_S32BE:
        case PA_SAMPLE_FLOAT32BE:
        case PA_SAMPLE_S24_32BE:
            isBigEndian = true;
            break;
        default:
            isBigEndian = false;
            break;
    }

    return isBigEndian;
}

pa_source *PaHdiSourceNew(pa_module *m, pa_modargs *ma, const char *driver)
{
    struct Userdata *u = NULL;
    pa_sample_spec ss;
    char *threadName = NULL;
    pa_channel_map map;
    int ret;

    pa_assert(m);
    pa_assert(ma);

    ss = m->core->default_sample_spec;
    map = m->core->default_channel_map;

    /* Override with modargs if provided */
    if (pa_modargs_get_sample_spec_and_channel_map(ma, &ss, &map, PA_CHANNEL_MAP_DEFAULT) < 0) {
        AUDIO_ERR_LOG("Failed to parse sample specification and channel map");
        return NULL;
    }

    u = pa_xnew0(struct Userdata, 1);
    pa_assert(u);

    u->core = m->core;
    u->module = m;
    u->rtpoll = pa_rtpoll_new();

    if (pa_thread_mq_init(&u->thread_mq, m->core->mainloop, u->rtpoll) < 0) {
        AUDIO_ERR_LOG("pa_thread_mq_init() failed.");
        goto fail;
    }

    if (pa_modargs_get_value_s32(ma, "sourceType", &u->attrs.sourceType) < 0) {
        AUDIO_ERR_LOG("Failed to parse sourceType argument");
    }
    u->attrs.sourceType = (int32_t)(ConvertToHDIAudioInputType(u->attrs.sourceType));
    AUDIO_INFO_LOG("after convert sourceType argument %{public}d", u->attrs.sourceType);

    ret = LoadSourceAdapter(pa_modargs_get_value(ma, "device_class", DEFAULT_DEVICE_CLASS),
        pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID),
        u->attrs.sourceType, &u->sourceAdapter);
    if (ret) {
        AUDIO_ERR_LOG("Load adapter failed");
        goto fail;
    }

    u->buffer_size = DEFAULT_BUFFER_SIZE;
    u->attrs.sampleRate = ss.rate;
    u->attrs.filePath = pa_modargs_get_value(ma, "file_path", "");
    if (pa_modargs_get_value_u32(ma, "open_mic_speaker", &u->open_mic_speaker) < 0) {
        AUDIO_ERR_LOG("Failed to parse open_mic_speaker argument");
        goto fail;
    }
    u->attrs.channel = ss.channels;
    u->attrs.format = ConvertToHDIAudioFormat(ss.format);
    u->attrs.isBigEndian = GetEndianInfo(ss.format);
    u->attrs.adapterName = pa_modargs_get_value(ma, "adapter_name", DEFAULT_DEVICE_CLASS);
    u->attrs.deviceNetworkId = pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID);
    if (pa_modargs_get_value_s32(ma, "device_type", &u->attrs.deviceType) < 0) {
        AUDIO_ERR_LOG("Failed to parse deviceType argument");
    }

    AUDIO_DEBUG_LOG("AudioDeviceCreateCapture format: %{public}d, isBigEndian: %{public}d channel: %{public}d,"
        "sampleRate: %{public}d", u->attrs.format, u->attrs.isBigEndian, u->attrs.channel, u->attrs.sampleRate);

    ret = PaSetSourceProperties(m, ma, &ss, &map, u);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to PaSetSourceProperties");
        goto fail;
    }

    u->attrs.bufferSize = u->buffer_size;
    u->attrs.open_mic_speaker = u->open_mic_speaker;

    ret = PaHdiCapturerInit(u);
    if (ret != 0) {
        AUDIO_ERR_LOG("Failed to PaHdiCapturerInit");
        goto fail;
    }

    threadName = "read-hdi";
    if (!(u->thread = pa_thread_new(threadName, ThreadFuncCapturerTimer, u))) {
        AUDIO_ERR_LOG("Failed to create hdi-source-record thread!");
        goto fail;
    }

    pa_source_put(u->source);
    return u->source;

fail:

    if (u->IsCapturerStarted) {
        PaHdiCapturerExit(u);
    }
    UserdataFree(u);

    return NULL;
}

void PaHdiSourceFree(pa_source *s)
{
    struct Userdata *u = NULL;
    pa_source_assert_ref(s);
    pa_assert_se(u = s->userdata);
    UserdataFree(u);
}
