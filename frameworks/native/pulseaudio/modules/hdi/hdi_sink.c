/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#define LOG_TAG "HdiSink"
#endif

#include <pa_config.h>
#include <pulse/rtclock.h>
#include <pulse/timeval.h>
#include <pulse/xmalloc.h>
#include <pulsecore/log.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/rtpoll.h>
#include <pulsecore/sink.h>
#include <pulsecore/thread-mq.h>
#include <pulsecore/thread.h>
#include <pulsecore/memblock.h>
#include <pulsecore/mix.h>
#include <pulse/volume.h>
#include <pulsecore/protocol-native.h>
#include <pulsecore/memblockq.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <inttypes.h>
#include <sys/types.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include "securec.h"

#include "audio_hdi_log.h"
#include "audio_schedule.h"
#include "parameter.h"
#include "audio_utils_c.h"
#include "volume_tools_c.h"
#include "audio_volume_c.h"
#include "common/hdi_adapter_info.h"
#include "sink/sink_intf.h"
#include "audio_effect_chain_adapter.h"
#include "audio_limiter_adapter.h"
#include "playback_capturer_adapter.h"
#include "sink_userdata.h"
#include "time.h"
#include "audio_performance_monitor_c.h"

#define DEFAULT_SINK_NAME "hdi_output"
#define DEFAULT_AUDIO_DEVICE_NAME "Speaker"
#define DEFAULT_DEVICE_CLASS "primary"
#define DEFAULT_DEVICE_NETWORKID "LocalDevice"
#define DEFAULT_BUFFER_SIZE 8192
#define MAX_SINK_VOLUME_LEVEL 1.0
#define DEFAULT_WRITE_TIME 1000
#define MIX_BUFFER_LENGTH (pa_page_size())
#define MAX_REWIND (7000 * PA_USEC_PER_MSEC)
#define USEC_PER_SEC 1000000
#define DEFAULT_IN_CHANNEL_NUM 2
#define PRIMARY_CHANNEL_NUM 2
#define IN_CHANNEL_NUM_MAX 16
#define DEFAULT_FRAMELEN 2048
#define SCENE_TYPE_NUM 9
#define HDI_MIN_MS_MAINTAIN 40
#define OFFLOAD_HDI_CACHE1 200 // ms, should equal with val in client
#define OFFLOAD_HDI_CACHE2 7000 // ms, should equal with val in client
#define OFFLOAD_HDI_CACHE3 500 // ms, should equal with val in client for movie
#define OFFLOAD_FRAME_SIZE 40
#define OFFLOAD_HDI_CACHE1_PLUS (OFFLOAD_HDI_CACHE1 + OFFLOAD_FRAME_SIZE + 5)   // ms, add 1 frame and 5ms
#define OFFLOAD_HDI_CACHE2_PLUS (OFFLOAD_HDI_CACHE2 + OFFLOAD_FRAME_SIZE + 5)   // to make sure get full
#define OFFLOAD_HDI_CACHE3_PLUS (OFFLOAD_HDI_CACHE3 + OFFLOAD_FRAME_SIZE + 5)   // to make sure get full for movie
#define SPRINTF_STR_LEN 100
#define DEFAULT_MULTICHANNEL_NUM 6
#define DEFAULT_NUM_CHANNEL 2
#define DEFAULT_MULTICHANNEL_CHANNELLAYOUT 1551
#define DEFAULT_CHANNELLAYOUT 3
#define OFFLOAD_SET_BUFFER_SIZE_NUM 5
#define OFFLOAD_HDI_FULL 1
#define POSSIBLY_UNUSED __attribute__((unused))
#define FADE_IN_BEGIN 0.0
#define FADE_IN_END 1.0
#define FADE_OUT_BEGIN 1.0
#define FADE_OUT_END 0.0
#define PRINT_INTERVAL_FRAME_COUNT 100
#define MIN_SLEEP_FOR_USEC 2000
#define DEFAULT_BLOCK_USEC 20000
#define EFFECT_PROCESS_RATE 48000
#define EFFECT_FRAME_LENGTH_MONO 960 // 48000Hz * 0.02s for 1 channel
#define EPSILON (1e-6f)

const int64_t LOG_LOOP_THRESHOLD = 50 * 60 * 9; // about 3 min
const uint64_t DEFAULT_GETLATENCY_LOG_THRESHOLD_MS = 100;
const uint32_t SAMPLE_RATE_96K_HZ = 96000; // 96khz
const uint8_t CHANNEL_COUNT_2 = 2; // 2ch

const char *DEVICE_CLASS_PRIMARY = "primary";
const char *DEVICE_CLASS_A2DP = "a2dp";
const char *DEVICE_CLASS_REMOTE = "remote";
const char *DEVICE_CLASS_OFFLOAD = "offload";
const char *DEVICE_CLASS_MULTICHANNEL = "multichannel";
const char *DEVICE_CLASS_DP = "dp";
const char *SINK_NAME_REMOTE_CAST_INNER_CAPTURER = "RemoteCastInnerCapturer";
const char *DUP_STEAM_NAME = "DupStream"; // should be same with DUP_STEAM in audio_info.h
const char *MCH_SINK_NAME = "MCH_Speaker";
const char *BT_SINK_NAME = "Bt_Speaker";
const char *OFFLOAD_SINK_NAME = "Offload_Speaker";
const char *DP_SINK_NAME = "DP_speaker";
const char *DP_MCH_SINK_NAME = "DP_MCH_speaker";

const int32_t WAIT_CLOSE_PA_OR_EFFECT_TIME = 4; // secs
const int32_t MONITOR_CLOSE_PA_TIME_SEC = 5 * 60; // 5min
bool g_effectAllStreamVolumeZeroMap[SCENE_TYPE_NUM] = {false, false, false, false, false, false, false};
bool g_effectHaveDisabledMap[SCENE_TYPE_NUM] = {false, false, false, false, false, false, false};
time_t g_effectStartVolZeroTimeMap[SCENE_TYPE_NUM] = {0, 0, 0, 0, 0, 0, 0};
char *const SCENE_TYPE_SET[SCENE_TYPE_NUM] = {"SCENE_DEFAULT", "SCENE_MUSIC", "SCENE_GAME", "SCENE_MOVIE",
    "SCENE_SPEECH", "SCENE_RING", "SCENE_VOIP_DOWN", "SCENE_OTHERS", "EFFECT_NONE"};
const int32_t COMMON_SCENE_TYPE_INDEX = 0;
const int32_t SUCCESS = 0;
const int32_t ERROR = -1;
const uint64_t FADE_OUT_TIME = 5000; // 5ms

enum HdiInputType { HDI_INPUT_TYPE_PRIMARY, HDI_INPUT_TYPE_OFFLOAD, HDI_INPUT_TYPE_MULTICHANNEL };

enum {
    HDI_INIT,
    HDI_DEINIT,
    HDI_START,
    HDI_STOP,
    HDI_RENDER,
    HDI_FLUSH,
    QUIT
};

enum AudioOffloadType {
    /**
     * Indicates audio offload state default.
     */
    OFFLOAD_DEFAULT = -1,
    /**
     * Indicates audio offload state : screen is active & app is foreground.
     */
    OFFLOAD_ACTIVE_FOREGROUND = 0,
    /**
     * Indicates audio offload state : screen is active & app is background.
     */
    OFFLOAD_ACTIVE_BACKGROUND = 1,
    /**
     * Indicates audio offload state : screen is inactive & app is background.
     */
    OFFLOAD_INACTIVE_BACKGROUND = 3,
};

static int32_t g_effectProcessFrameCount = 0;
static void UserdataFree(struct Userdata *u);
static int32_t PrepareDevice(struct Userdata *u, const char *filePath);

static int32_t PrepareDeviceOffload(struct Userdata *u);
static char *GetStateInfo(pa_sink_state_t state);
static char *GetInputStateInfo(pa_sink_input_state_t state);
static void PaInputStateChangeCb(pa_sink_input *i, pa_sink_input_state_t state);
static void OffloadLock(struct Userdata *u);
static void OffloadUnlock(struct Userdata *u);
static int32_t UpdatePresentationPosition(struct Userdata *u);
static bool InputIsPrimary(pa_sink_input *i);
static bool InputIsOffload(pa_sink_input *i);
static bool InputIsMovie(pa_sink_input *i);
static uint32_t getSinkInputUid(pa_sink_input *i);
static void GetSinkInputName(pa_sink_input *i, char *str, int len);
static const char *safeProplistGets(const pa_proplist *p, const char *key, const char *defstr);
static void StartOffloadHdi(struct Userdata *u, pa_sink_input *i);
static void StartPrimaryHdiIfRunning(struct Userdata *u);
static void StartMultiChannelHdiIfRunning(struct Userdata *u);
static void CheckInputChangeToOffload(struct Userdata *u, pa_sink_input *i);
static void ResetVolumeBySinkInputState(pa_sink_input *i, pa_sink_input_state_t state);
static void *AllocateBuffer(size_t size);
static bool AllocateEffectBuffer(struct Userdata *u);
static void FreeEffectBuffer(struct Userdata *u);
static void ResetBufferAttr(struct Userdata *u);
static void CreateLimiter(struct Userdata *u);
static void FreeLimiter(struct Userdata *u);
static enum AudioSampleFormatIntf ConvertPaToHdiAdapterFormat(pa_sample_format_t format);
static void UpdateStreamVolumeMap(struct Userdata *u);
static struct VolumeValues *GetVolumeFromStreamVolumeMap(struct Userdata *u, uint32_t sessionID);
static void RemoveVolumeFromStreamVolumeMap(struct Userdata *u, pa_sink_input *i);
static bool IsZeroVolume(float volume);

// BEGIN Utility functions
#define FLOAT_EPS 1e-6f
#define MEMBLOCKQ_MAXLENGTH (16*1024*16)
#define OFFSET_BIT_24 3
#define BIT_DEPTH_TWO 2
#define BIT_8 8
#define BIT_16 16
#define BIT_24 24
#define BIT_32 32
static bool IsSinkNameDp(const char *sinkName)
{
    if (sinkName == NULL) {
        AUDIO_ERR_LOG("ptr is null");
        return false;
    }

    if (strcmp(sinkName, DP_SINK_NAME) == 0) {
        return true;
    }

    if (strcmp(sinkName, DP_MCH_SINK_NAME) == 0) {
        return true;
    }

    return false;
}

static uint32_t Read24Bit(const uint8_t *p)
{
    return ((uint32_t) p[BIT_DEPTH_TWO] << BIT_16) | ((uint32_t) p[1] << BIT_8) | ((uint32_t) p[0]);
}

static void Write24Bit(uint8_t *p, uint32_t u)
{
    p[BIT_DEPTH_TWO] = (uint8_t) (u >> BIT_16);
    p[1] = (uint8_t) (u >> BIT_8);
    p[0] = (uint8_t) u;
}

static void ConvertFrom16BitToFloat(unsigned n, const int16_t *a, float *b)
{
    for (; n > 0; n--) {
        *(b++) = *(a++) * (1.0f / (1 << (BIT_16 - 1)));
    }
}

static void ConvertFrom24BitToFloat(unsigned n, const uint8_t *a, float *b)
{
    for (; n > 0; n--) {
        int32_t s = Read24Bit(a) << BIT_8;
        *b = s * (1.0f / (1U << (BIT_32 - 1)));
        a += OFFSET_BIT_24;
        b++;
    }
}

static void ConvertFrom32BitToFloat(unsigned n, const int32_t *a, float *b)
{
    for (; n > 0; n--) {
        *(b++) = *(a++) * (1.0f / (1U << (BIT_32 - 1)));
    }
}

static float CapMax(float v)
{
    float value = v;
    if (v >= 1.0f) {
        value = 1.0f - FLOAT_EPS;
    } else if (v <= -1.0f) {
        value = -1.0f + FLOAT_EPS;
    }
    return value;
}

static void ConvertFromFloatTo16Bit(unsigned n, const float *a, int16_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1 << (BIT_16 - 1));
        *(b++) = (int16_t) v;
    }
}

static void ConvertFromFloatTo24Bit(unsigned n, const float *a, uint8_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        Write24Bit(b, ((int32_t) v) >> BIT_8);
        b += OFFSET_BIT_24;
    }
}

static void ConvertFromFloatTo32Bit(unsigned n, const float *a, int32_t *b)
{
    for (; n > 0; n--) {
        float tmp = *a++;
        float v = CapMax(tmp) * (1U << (BIT_32 - 1));
        *(b++) = (int32_t) v;
    }
}

static void ConvertToFloat(pa_sample_format_t format, unsigned n, void *src, float *dst)
{
    CHECK_AND_RETURN_LOG(src != NULL, "src is null");
    CHECK_AND_RETURN_LOG(dst != NULL, "dst is null");
    int32_t ret;
    switch (format) {
        case PA_SAMPLE_S16LE:
            ConvertFrom16BitToFloat(n, src, dst);
            break;
        case PA_SAMPLE_S24LE:
            ConvertFrom24BitToFloat(n, src, dst);
            break;
        case PA_SAMPLE_S32LE:
            ConvertFrom32BitToFloat(n, src, dst);
            break;
        default:
            ret = memcpy_s(dst, n, src, n);
            CHECK_AND_RETURN_LOG(ret == 0, "ConvertToFloat: copy from src to dst fail!");
            break;
    }
}

static void ConvertFromFloat(pa_sample_format_t format, unsigned n, float *src, void *dst)
{
    CHECK_AND_RETURN_LOG(src != NULL, "src is null");
    CHECK_AND_RETURN_LOG(dst != NULL, "dst is null");
    int32_t ret;
    switch (format) {
        case PA_SAMPLE_S16LE:
            ConvertFromFloatTo16Bit(n, src, dst);
            break;
        case PA_SAMPLE_S24LE:
            ConvertFromFloatTo24Bit(n, src, dst);
            break;
        case PA_SAMPLE_S32LE:
            ConvertFromFloatTo32Bit(n, src, dst);
            break;
        default:
            ret = memcpy_s(dst, n, src, n);
            CHECK_AND_RETURN_LOG(ret == 0, "ConvertFromFloat: copy from src to dst fail!");
            break;
    }
}

static void updateResampler(pa_sink_input *sinkIn, const char *sceneType, bool mchFlag, pa_sink *si)
{
    uint32_t processChannels = DEFAULT_NUM_CHANNEL;
    uint64_t processChannelLayout = DEFAULT_CHANNELLAYOUT;
    if (mchFlag) {
        struct Userdata *u = sinkIn->sink->userdata;
        processChannels = u->multiChannel.sinkChannel;
        processChannelLayout = u->multiChannel.sinkChannelLayout;
    } else {
        EffectChainManagerReturnEffectChannelInfo(sceneType, &processChannels, &processChannelLayout);
    }

    pa_sample_spec outSampleSpec;
    pa_channel_map outChannelMap;
    if (pa_safe_streq(sceneType, "EFFECT_NONE")) {
        outSampleSpec = si->sample_spec;
        outChannelMap = si->channel_map;
    } else {
        outSampleSpec.channels = processChannels;
        outSampleSpec.rate = EFFECT_PROCESS_RATE;
        outSampleSpec.format = sinkIn->thread_info.resampler->o_ss.format;
        ConvertChLayoutToPaChMap(processChannelLayout, &outChannelMap);
        outChannelMap.channels = processChannels;
    }

    if (pa_sample_spec_equal(&sinkIn->thread_info.resampler->o_ss, &outSampleSpec) &&
        pa_channel_map_equal(&sinkIn->thread_info.resampler->o_cm, &outChannelMap)) {
        return;
    }
    AUDIO_INFO_LOG("Update Resampler before effectchain: sceneType [%{public}s], sink name [%{public}s], "
        "input sample rate [%{public}d], channels [%{public}d], format [%{public}d]; "
        "output rate [%{public}d], channels [%{public}d], format [%{public}d]",
        sceneType, si->name, sinkIn->thread_info.resampler->i_ss.rate, sinkIn->thread_info.resampler->i_ss.channels,
        sinkIn->thread_info.resampler->i_ss.format, outSampleSpec.rate, outSampleSpec.channels, outSampleSpec.format);

    pa_resampler *r = pa_resampler_new(
        sinkIn->thread_info.resampler->mempool,
        &sinkIn->thread_info.resampler->i_ss,
        &sinkIn->thread_info.resampler->i_cm,
        &outSampleSpec, &outChannelMap,
        sinkIn->core->lfe_crossover_freq,
        sinkIn->thread_info.resampler->method,
        sinkIn->thread_info.resampler->flags);
    
    pa_resampler_free(sinkIn->thread_info.resampler);
    sinkIn->thread_info.resampler = r;
    return;
}

static ssize_t RenderWrite(struct SinkAdapter *sinkAdapter, pa_memchunk *pchunk)
{
    size_t index;
    size_t length;
    ssize_t count = 0;
    void *p = NULL;

    CHECK_AND_RETURN_RET_LOG(pchunk != NULL, 0, "pchunk is null");

    index = pchunk->index;
    length = pchunk->length;
    p = pa_memblock_acquire(pchunk->memblock);
    pa_assert(p);

    while (true) {
        uint64_t writeLen = 0;

        int32_t ret = sinkAdapter->SinkAdapterRenderFrame(sinkAdapter, ((char*)p + index),
            (uint64_t)length, &writeLen);
        if (writeLen > length) {
            AUDIO_ERR_LOG("Error writeLen > actual bytes. Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        }
        if (writeLen == 0) {
            AUDIO_ERR_LOG("Failed to render Length: %{public}zu, Written: %{public}" PRIu64 " bytes, %{public}d ret",
                length, writeLen, ret);
            count = -1 - count;
            break;
        } else {
            count += (ssize_t)writeLen;
            index += writeLen;
            length -= writeLen;
            if (length == 0) {
                break;
            }
        }
    }
    pa_memblock_release(pchunk->memblock);
    pa_memblock_unref(pchunk->memblock);

    return count;
}

static enum AudioOffloadType GetInputPolicyState(pa_sink_input *i)
{
    CHECK_AND_RETURN_RET_LOG(i != NULL, OFFLOAD_DEFAULT, "sink input is null");
    return (enum AudioOffloadType)GetOffloadType(i->index);
}

static void OffloadSetHdiVolume(pa_sink_input *i)
{
    if (!InputIsOffload(i)) {
        return;
    }

    struct Userdata *u = i->sink->userdata;
    const char *streamType = safeProplistGets(i->proplist, "stream.type", "NULL");
    const char *sessionIDStr = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
    const char *deviceClass = u->offload.sinkAdapter->deviceClass;
    uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
    struct VolumeValues volumes = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float volumeEnd = GetCurVolume(sessionID, streamType, deviceClass, &volumes);
    float volumeBeg = volumes.volumeHistory;
    if (fabs(volumeBeg - volumeEnd) > EPSILON) {
        AUDIO_INFO_LOG("sessionID:%{public}u, volumeBeg:%{public}f, volumeEnd:%{public}f",
            sessionID, volumeBeg, volumeEnd);
        SetPreVolume(sessionID, volumeEnd);
        MonitorVolume(sessionID, true);
    }
    u->offload.sinkAdapter->SinkAdapterSetVolume(u->offload.sinkAdapter, volumeEnd, volumeEnd);
}

static void OffloadSetHdiBufferSize(pa_sink_input *i)
{
    if (!InputIsOffload(i)) {
        return;
    }

    struct Userdata *u = i->sink->userdata;
    const uint32_t bufSize = InputIsMovie(i) ? OFFLOAD_HDI_CACHE3 :
        (GetInputPolicyState(i) == OFFLOAD_INACTIVE_BACKGROUND ? OFFLOAD_HDI_CACHE2 : OFFLOAD_HDI_CACHE1);
    u->offload.sinkAdapter->SinkAdapterSetBufferSize(u->offload.sinkAdapter, bufSize);
}

static int32_t RenderWriteOffload(struct Userdata *u, pa_sink_input *i, pa_memchunk *pchunk)
{
    size_t index;
    size_t length;
    void *p = NULL;

    CHECK_AND_RETURN_RET_LOG(pchunk != NULL, 0, "pchunk is null");

    index = pchunk->index;
    length = pchunk->length;
    p = pa_memblock_acquire(pchunk->memblock);
    pa_assert(p);

    uint64_t writeLen = 0;
    uint64_t now = pa_rtclock_now();
    if (!u->offload.isHDISinkStarted) {
        AUDIO_DEBUG_LOG("StartOffloadHdi before write, because maybe sink switch");
        StartOffloadHdi(u, i);
    }
    if (u->offload.firstWriteHdi) {
        AudioRawFormat rawFormat;
        rawFormat.format = (uint32_t)ConvertPaToHdiAdapterFormat(u->ss.format);
        rawFormat.channels = (uint32_t)u->ss.channels;
        ProcessVol((uint8_t*)p + index, length, rawFormat, 0, 1); // do fade in
    }
    int32_t ret = u->offload.sinkAdapter->SinkAdapterRenderFrame(u->offload.sinkAdapter, ((char*)p + index),
        (uint64_t)length, &writeLen);
    pa_memblock_release(pchunk->memblock);
    if (writeLen != length && writeLen != 0) {
        AUDIO_ERR_LOG("Error writeLen != actual bytes. Length: %zu, Written: %" PRIu64 " bytes, %d ret",
            length, writeLen, ret);
        return -1;
    }
    if (ret == 0 && u->offload.firstWriteHdi && writeLen == length) {
        u->offload.firstWriteHdi = false;
        u->offload.hdiPosTs = now;
        u->offload.hdiPos = 0;
        // if the hdi is flushing, it will block the volume setting.
        // so the render frame judge it.
        OffloadSetHdiVolume(i);
    }
    if (ret == 0 && u->offload.setHdiBufferSizeNum > 0 && writeLen == length) {
        u->offload.setHdiBufferSizeNum--;
        OffloadSetHdiBufferSize(i);
    }
    if (ret == 0 && writeLen == 0 && !u->offload.firstWriteHdi) { // is full
        AUDIO_DEBUG_LOG("RenderWriteOffload, hdi is full, break");
        return OFFLOAD_HDI_FULL; // 1 indicates full
    } else if (writeLen == 0) {
        AUDIO_ERR_LOG("Failed to render Length: %{public}zu, Written: %{public}" PRIu64 " bytes, %{public}d ret",
            length, writeLen, ret);
        return -1;
    }
    return 0;
}

static void OffloadCallback(const enum RenderCallbackType type, int8_t *userdata)
{
    AUTO_CTRACE("hdi_sink::OffloadCallback type: %d", type);
    struct Userdata *u = (struct Userdata *)userdata;
    pthread_mutex_lock(&u->offload.lockCallback);
    switch (type) {
        case CB_NONBLOCK_WRITE_COMPLETED: { //need more data
            const int hdistate = pa_atomic_load(&u->offload.hdistate);
            if (hdistate == 1) {
                pa_atomic_store(&u->offload.hdistate, 0);
                OffloadLock(u);
                UpdatePresentationPosition(u);
            }
            if (u->thread_mq.inq) {
                pa_asyncmsgq_post(u->thread_mq.inq, NULL, 0, NULL, 0, NULL, NULL);
            }
            break;
        }
        case CB_DRAIN_COMPLETED:
        case CB_FLUSH_COMPLETED:
            break;
        case CB_RENDER_FULL: { // no need data
            const int hdistate = pa_atomic_load(&u->offload.hdistate);
            if (hdistate == 0) {
                pa_atomic_store(&u->offload.hdistate, 1);
                OffloadUnlock(u);
            }
            break;
        }
        case CB_ERROR_OCCUR:
            break;
        default:
            break;
    }
    pthread_mutex_unlock(&u->offload.lockCallback);
}

static void RegOffloadCallback(struct Userdata *u)
{
    u->offload.sinkAdapter->SinkAdapterRegistOffloadHdiCallback(u->offload.sinkAdapter, (int8_t *)OffloadCallback,
        (int8_t *)u);
}

static ssize_t TestModeRenderWrite(struct Userdata *u, pa_memchunk *pchunk)
{
    size_t index;
    size_t length;
    ssize_t count = 0;
    void *p = NULL;

    CHECK_AND_RETURN_RET_LOG(pchunk != NULL, 0, "pchunk is null");

    index = pchunk->index;
    length = pchunk->length;
    p = pa_memblock_acquire(pchunk->memblock);
    pa_assert(p);

    if (*((int32_t*)p) > 0) {
        AUDIO_DEBUG_LOG("RenderWrite Write: %{public}d", ++u->writeCount);
    }
    AUDIO_DEBUG_LOG("RenderWrite Write renderCount: %{public}d", ++u->renderCount);

    while (true) {
        uint64_t writeLen = 0;

        int32_t ret = u->primary.sinkAdapter->SinkAdapterRenderFrame(u->primary.sinkAdapter, ((char *)p + index),
            (uint64_t)length, &writeLen);
        if (writeLen > length) {
            AUDIO_ERR_LOG("Error writeLen > actual bytes. Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        }
        if (writeLen == 0) {
            AUDIO_ERR_LOG("Failed to render Length: %zu, Written: %" PRIu64 " bytes, %d ret",
                         length, writeLen, ret);
            count = -1 - count;
            break;
        } else {
            count += (ssize_t)writeLen;
            index += writeLen;
            length -= writeLen;
            if (length == 0) {
                break;
            }
        }
    }
    pa_memblock_release(pchunk->memblock);
    pa_memblock_unref(pchunk->memblock);

    return count;
}

static bool IsInnerCapturer(pa_sink_input *sinkIn)
{
    pa_sink_input_assert_ref(sinkIn);

    if (!GetInnerCapturerState()) {
        return false;
    }

    const char *usageStr = pa_proplist_gets(sinkIn->proplist, "stream.usage");
    const char *privacyTypeStr = pa_proplist_gets(sinkIn->proplist, "stream.privacyType");
    int32_t usage = -1;
    int32_t privacyType = -1;
    bool usageSupport = false;
    bool privacySupport = true;

    if (privacyTypeStr != NULL) {
        pa_atoi(privacyTypeStr, &privacyType);
        privacySupport = IsPrivacySupportInnerCapturer(privacyType);
    }

    if (usageStr != NULL) {
        pa_atoi(usageStr, &usage);
        usageSupport = IsStreamSupportInnerCapturer(usage);
    }
    return privacySupport && usageSupport;
}

static const char *safeProplistGets(const pa_proplist *p, const char *key, const char *defstr)
{
    const char *res = pa_proplist_gets(p, key);
    if (res == NULL) {
        return defstr;
    }
    return res;
}

//modify from pa inputs_drop
static void InputsDropFromInputs(pa_mix_info *infoInputs, unsigned nInputs, pa_mix_info *info, unsigned n,
    pa_memchunk *result);

static unsigned GetInputsInfo(enum HdiInputType type, bool isRun, pa_sink *s, pa_mix_info *info, unsigned maxinfo);

static unsigned SinkRenderPrimaryClusterCap(pa_sink *si, size_t *length, pa_mix_info *infoIn, unsigned maxInfo)
{
    pa_sink_input *sinkIn;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_RET_LOG(infoIn != NULL, 0, "infoIn is null");

    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryClusterCap:len:%zu", *length);

    unsigned n = 0;
    void *state = NULL;
    size_t mixlength = *length;
    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL)) && maxInfo > 0) {
        if (IsInnerCapturer(sinkIn) && InputIsPrimary(sinkIn)) {
            pa_sink_input_assert_ref(sinkIn);

            // max_rewind is 0 by default, need change to at least u->buffer_size for InnerCapSinkInputsRewind.
            if (pa_memblockq_get_maxrewind(sinkIn->thread_info.render_memblockq) == 0) {
                AUTO_CTRACE("hdi_sink::pa_sink_input_update_max_rewind:%u len:%zu", sinkIn->index, *length);
                pa_sink_input_update_max_rewind(sinkIn, *length);
            }
            AUTO_CTRACE("hdi_sink::ClusterCap::pa_sink_input_peek:%u len:%zu", sinkIn->index, *length);
            pa_sink_input_peek(sinkIn, *length, &infoIn->chunk, &infoIn->volume);

            if (mixlength == 0 || infoIn->chunk.length < mixlength)
                mixlength = infoIn->chunk.length;

            if (pa_memblock_is_silence(infoIn->chunk.memblock)) {
                AUTO_CTRACE("hdi_sink::SinkRenderPrimaryClusterCap::is_silence");
                pa_memblock_unref(infoIn->chunk.memblock);
                continue;
            }

            infoIn->userdata = pa_sink_input_ref(sinkIn);
            pa_assert(infoIn->chunk.memblock);
            pa_assert(infoIn->chunk.length > 0);

            infoIn++;
            n++;
            maxInfo--;
        }
    }

    if (mixlength > 0) {
        *length = mixlength;
    }

    return n;
}

static void SinkRenderPrimaryMix(pa_sink *si, size_t length, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    if (n == 0) {
        if (chunkIn->length > length)
            chunkIn->length = length;

        pa_silence_memchunk(chunkIn, &si->sample_spec);
    } else if (n == 1) {
        pa_cvolume volume;

        if (chunkIn->length > length)
            chunkIn->length = length;

        pa_sw_cvolume_multiply(&volume, &si->thread_info.soft_volume, &infoIn[0].volume);

        if (si->thread_info.soft_muted || pa_cvolume_is_muted(&volume)) {
            pa_silence_memchunk(chunkIn, &si->sample_spec);
        } else {
            pa_memchunk tmpChunk;

            tmpChunk = infoIn[0].chunk;
            pa_memblock_ref(tmpChunk.memblock);

            if (tmpChunk.length > length)
                tmpChunk.length = length;

            if (!pa_cvolume_is_norm(&volume)) {
                pa_memchunk_make_writable(&tmpChunk, 0);
                pa_volume_memchunk(&tmpChunk, &si->sample_spec, &volume);
            }

            pa_memchunk_memcpy(chunkIn, &tmpChunk);
            pa_memblock_unref(tmpChunk.memblock);
        }
    } else {
        void *ptr;

        ptr = pa_memblock_acquire(chunkIn->memblock);

        chunkIn->length = pa_mix(infoIn, n,
                                 (uint8_t*) ptr + chunkIn->index, length,
                                 &si->sample_spec,
                                 &si->thread_info.soft_volume,
                                 si->thread_info.soft_muted);

        pa_memblock_release(chunkIn->memblock);
    }
}

static void SinkRenderPrimaryMixCap(pa_sink *si, size_t length, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_LOG(infoIn != NULL, "infoIn is null");
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryMixCap:%u:len:%zu", n, chunkIn->length);
    if (n == 0) {
        if (chunkIn->length > length) {
            chunkIn->length = length;
        }

        pa_silence_memchunk(chunkIn, &si->sample_spec);
    } else if (n == 1) {
        pa_memchunk tmpChunk;
        // If chunkIn is not full filled, we need re-call SinkRenderPrimaryPeekCap.
        if (chunkIn->length > length) {
            chunkIn->length = length;
        }

        tmpChunk = infoIn[0].chunk;
        pa_memblock_ref(tmpChunk.memblock);

        if (tmpChunk.length > length) {
            tmpChunk.length = length;
        }

        pa_memchunk_memcpy(chunkIn, &tmpChunk);
        pa_memblock_unref(tmpChunk.memblock);
    } else {
        void *ptr;

        ptr = pa_memblock_acquire(chunkIn->memblock);

        for (unsigned index = 0; index < n; index++) {
            for (unsigned channel = 0; channel < si->sample_spec.channels; channel++) {
                infoIn[index].volume.values[channel] = PA_VOLUME_NORM;
            }
        }

        chunkIn->length = pa_mix(infoIn, n, (uint8_t*) ptr + chunkIn->index, length, &si->sample_spec, NULL, false);

        pa_memblock_release(chunkIn->memblock);
    }
}

static void SinkRenderPrimaryInputsDropCap(pa_sink *si, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_LOG(infoIn != NULL, "infoIn is null");
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    CHECK_AND_RETURN_LOG(chunkIn->memblock != NULL, "chunkIn->memblock is null");
    CHECK_AND_RETURN_LOG(chunkIn->length > 0, "chunkIn->length < 0");

    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryInputsDropCap:%u:len:%zu", n, chunkIn->length);

    /* We optimize for the case where the order of the inputs has not changed */

    pa_mix_info *infoCur = NULL;
    pa_sink_input *sceneSinkInput = NULL;
    bool isCaptureSilently = IsCaptureSilently();
    for (uint32_t k = 0; k < n; k++) {
        sceneSinkInput = infoIn[k].userdata;
        pa_sink_input_assert_ref(sceneSinkInput);
        AUTO_CTRACE("hdi_sink::InnerCap:pa_sink_input_drop:%u:len:%zu", sceneSinkInput->index, chunkIn->length);
        pa_sink_input_drop(sceneSinkInput, chunkIn->length);

        infoCur = infoIn + k;
        if (infoCur) {
            if (infoCur->chunk.memblock) {
                pa_memblock_unref(infoCur->chunk.memblock);
                pa_memchunk_reset(&infoCur->chunk);
            }

            pa_sink_input_unref(infoCur->userdata);

            if (isCaptureSilently) {
                infoCur->userdata = NULL;
            }
        }
    }
}

static int32_t SinkRenderPrimaryPeekCap(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_mix_info infoIn[MAX_MIX_CHANNELS];
    unsigned n;
    size_t length;
    size_t blockSizeMax;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryPeekCap:len:%zu", chunkIn->length);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    length = chunkIn->length;
    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    n = SinkRenderPrimaryClusterCap(si, &length, infoIn, MAX_MIX_CHANNELS);
    SinkRenderPrimaryMixCap(si, length, infoIn, n, chunkIn);

    SinkRenderPrimaryInputsDropCap(si, infoIn, n, chunkIn);
    pa_sink_unref(si);

    return n;
}

static int32_t SinkRenderPrimaryGetDataCap(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_memchunk chunk;
    size_t l;
    size_t d;
    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryGetDataCap:len:%zu", chunkIn->length);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    l = chunkIn->length;
    d = 0;

    int32_t nSinkInput = 0;
    while (l > 0) {
        chunk = *chunkIn;
        chunk.index += d;
        chunk.length -= d;

        nSinkInput = SinkRenderPrimaryPeekCap(si, &chunk);

        d += (size_t)chunk.length;
        l -= (size_t)chunk.length;
    }

    pa_sink_unref(si);

    return nSinkInput;
}

static bool monitorLinked(pa_sink *si, bool isRunning)
{
    if (isRunning) {
        return si->monitor_source && PA_SOURCE_IS_RUNNING(si->monitor_source->thread_info.state);
    } else {
        return si->monitor_source && PA_SOURCE_IS_LINKED(si->monitor_source->thread_info.state);
    }
}

static void InnerCapSinkInputsRewind(pa_sink *si, size_t length)
{
    AUTO_CTRACE("hdi_sink::InnerCapSinkInputsRewind:len:%zu", length);

    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);

    pa_sink_input *sinkIn = NULL;
    void *state = NULL;
    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL))) {
        if (IsInnerCapturer(sinkIn) && InputIsPrimary(sinkIn)) {
            pa_sink_input_assert_ref(sinkIn);
            pa_sink_input_process_rewind(sinkIn, length); // will not work well if maxrewind = 0
        }
    }
}

static void SinkRenderCapProcess(pa_sink *si, size_t length, pa_memchunk *capResult)
{
    AUTO_CTRACE("hdi_sink::SinkRenderCapProcess:len:%zu", length);
    capResult->memblock = pa_memblock_new(si->core->mempool, length);
    capResult->index = 0;
    capResult->length = length;
    SinkRenderPrimaryGetDataCap(si, capResult);
    if (monitorLinked(si, false)) {
        AUTO_CTRACE("hdi_sink::pa_source_post:len:%zu", capResult->length);
        pa_source_post(si->monitor_source, capResult);
    }

    //If not silent capture, we need to call rewind for Speak.
    if (!IsCaptureSilently()) {
        InnerCapSinkInputsRewind(si, capResult->length);
    }
    return;
}

static void SinkRenderPrimaryInputsDrop(pa_sink *si, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    unsigned nUnreffed = 0;

    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    CHECK_AND_RETURN_LOG(chunkIn->memblock != NULL, "chunkIn->memblock is null");
    CHECK_AND_RETURN_LOG(chunkIn->length > 0, "chunkIn->length < 0");

    /* We optimize for the case where the order of the inputs has not changed */
    pa_mix_info *infoCur = NULL;
    pa_sink_input *sceneSinkInput = NULL;
    for (uint32_t k = 0; k < n; k++) {
        sceneSinkInput = infoIn[k].userdata;
        pa_sink_input_assert_ref(sceneSinkInput);

        /* Drop read data */
        pa_sink_input_drop(sceneSinkInput, chunkIn->length);
        infoCur = infoIn + k;
        if (infoCur) {
            if (infoCur->chunk.memblock) {
                pa_memblock_unref(infoCur->chunk.memblock);
                pa_memchunk_reset(&infoCur->chunk);
            }

            pa_sink_input_unref(infoCur->userdata);
            infoCur->userdata = NULL;

            nUnreffed += 1;
        }
    }
    /* Now drop references to entries that are included in the
     * pa_mix_info array but don't exist anymore */

    if (nUnreffed < n) {
        for (; n > 0; infoIn++, n--) {
            if (infoIn->userdata)
                pa_sink_input_unref(infoIn->userdata);
            if (infoIn->chunk.memblock)
                pa_memblock_unref(infoIn->chunk.memblock);
        }
    }
}

static void SinkRenderMultiChannelInputsDrop(pa_sink *si, pa_mix_info *infoIn, unsigned n, pa_memchunk *chunkIn)
{
    AUDIO_DEBUG_LOG("mch inputs drop start");
    unsigned nUnreffed = 0;

    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    CHECK_AND_RETURN_LOG(chunkIn->memblock != NULL, "chunkIn->memblock is null");
    CHECK_AND_RETURN_LOG(chunkIn->length > 0, "chunkIn->length < 0");
    /* We optimize for the case where the order of the inputs has not changed */
    pa_mix_info *infoCur = NULL;
    pa_sink_input *sceneSinkInput = NULL;
    for (uint32_t k = 0; k < n; k++) {
        sceneSinkInput = infoIn[k].userdata;
        pa_sink_input_assert_ref(sceneSinkInput);

        /* Drop read data */
        pa_sink_input_drop(sceneSinkInput, chunkIn->length);
        infoCur = infoIn + k;
        if (infoCur) {
            if (infoCur->chunk.memblock) {
                pa_memblock_unref(infoCur->chunk.memblock);
                pa_memchunk_reset(&infoCur->chunk);
            }

            pa_sink_input_unref(infoCur->userdata);
            infoCur->userdata = NULL;

            nUnreffed += 1;
        }
    }
    /* Now drop references to entries that are included in the
     * pa_mix_info array but don't exist anymore */

    if (nUnreffed < n) {
        for (; n > 0; infoIn++, n--) {
            if (infoIn->userdata)
                pa_sink_input_unref(infoIn->userdata);
            if (infoIn->chunk.memblock)
                pa_memblock_unref(infoIn->chunk.memblock);
        }
    }
}

static void silenceData(pa_mix_info *infoIn, pa_sink *si, uint32_t streamIndex)
{
    pa_memchunk_make_writable(&infoIn->chunk, 0);
    void *tmpdata = pa_memblock_acquire_chunk(&infoIn->chunk);
    int32_t bufferAvg = GetSimpleBufferAvg(tmpdata, infoIn->chunk.length);
    AUDIO_INFO_LOG("do fading done for sink[%{public}d],buffer avg:%{public}d", streamIndex, bufferAvg);
    memset_s(tmpdata, infoIn->chunk.length, 0, infoIn->chunk.length);
    pa_memblock_release(infoIn->chunk.memblock);
}

static enum AudioSampleFormatIntf ConvertPaToHdiAdapterFormat(pa_sample_format_t format)
{
    enum AudioSampleFormatIntf adapterFormat;
    switch (format) {
        case PA_SAMPLE_U8:
            adapterFormat = SAMPLE_U8;
            break;
        case PA_SAMPLE_S16LE:
            adapterFormat = SAMPLE_S16;
            break;
        case PA_SAMPLE_S24LE:
            adapterFormat = SAMPLE_S24;
            break;
        case PA_SAMPLE_S32LE:
            adapterFormat = SAMPLE_S32;
            break;
        default:
            adapterFormat = INVALID_WIDTH;
            break;
    }

    return adapterFormat;
}

static void DoFading(void *data, int32_t length, uint32_t format, uint32_t channel, int32_t fadeType)
{
    AudioRawFormat rawFormat;
    rawFormat.format = format;
    rawFormat.channels = channel;
    AUDIO_INFO_LOG("length:%{public}d channels:%{public}d format:%{public}d fadeType:%{public}d",
        length, rawFormat.channels, rawFormat.format, fadeType);
    int32_t ret = 0;
    if (fadeType == 0) {
        ret = ProcessVol(data, length, rawFormat, FADE_IN_BEGIN, FADE_IN_END);
    } else {
        ret = ProcessVol(data, length, rawFormat, FADE_OUT_BEGIN, FADE_OUT_END);
    }
    if (ret != 0) {
        AUDIO_WARNING_LOG("ProcessVol failed:%{public}d", ret);
    }
}

static size_t GetbqlAlinLength(struct playback_stream *ps, pa_sink_input *sinkIn)
{
    const bool b = (bool)ps->sink_input->thread_info.resampler;
    const pa_sample_spec sampleSpecIn = b ? ps->sink_input->thread_info.resampler->i_ss : ps->sink_input->sample_spec;
    const pa_sample_spec sampleSpecOut = b ? ps->sink_input->thread_info.resampler->o_ss : ps->sink_input->sample_spec;
    const size_t bql = pa_memblockq_get_length(ps->memblockq);
    const size_t bqlResamp = pa_usec_to_bytes(pa_bytes_to_usec(bql, &sampleSpecIn), &sampleSpecOut);
    const size_t bqlRend = pa_memblockq_get_length(sinkIn->thread_info.render_memblockq);
    const size_t bqlAlin = pa_frame_align(bqlResamp + bqlRend, &sampleSpecOut);
    return bqlAlin;
}

static bool DoStopDrainFadeout(pa_sink_input *sinkIn, uint32_t streamIndex, int32_t length)
{
    playback_stream *ps = sinkIn->userdata;
    CHECK_AND_RETURN_RET_LOG(ps != NULL, false, "playback_stream is null");
    if (ps->drain_request) {
        uint32_t sinkStopFadeout = GetStopFadeoutState(streamIndex);
        if (sinkStopFadeout == DO_FADE) {
            const size_t bqlAlin = GetbqlAlinLength(ps, sinkIn);
            if (bqlAlin > 0 && (int32_t)bqlAlin == length) {
                AUDIO_INFO_LOG("drain_request bqlalin:%{public}zu", bqlAlin);
                RemoveStopFadeoutState(streamIndex);
                return true;
            }
        }
    }
    return false;
}

static int32_t GetFadeLenth(enum FadeStrategy fadeStrategy, size_t chunkLength, pa_sample_spec ss)
{
    if (fadeStrategy == FADE_STRATEGY_NONE) {
        // none fade
        return 0;
    }

    if (fadeStrategy == FADE_STRATEGY_SHORTER) {
        // do 5ms fade-in fade-out
        size_t fadeLenth = pa_usec_to_bytes(FADE_OUT_TIME, &ss);
        return ((fadeLenth < chunkLength) ? fadeLenth : chunkLength);
    }

    if (fadeStrategy == FADE_STRATEGY_DEFAULT) {
        return chunkLength;
    }

    return chunkLength;
}

static void PreparePrimaryFading(pa_sink_input *sinkIn, pa_mix_info *infoIn, pa_sink *si)
{
    CHECK_AND_RETURN_LOG(sinkIn != NULL, "sinkIn is null");
    CHECK_AND_RETURN_LOG(infoIn != NULL, "infoIn is null");
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    const char *streamType = safeProplistGets(sinkIn->proplist, "stream.type", "NULL");
    if (pa_safe_streq(streamType, "ultrasonic")) { return; }

    const char *strExpectedPlaybackDurationBytes = safeProplistGets(sinkIn->proplist,
        "expectedPlaybackDurationBytes", "0");
    uint64_t expectedPlaybackDurationBytes = 0;
    pa_atou64(strExpectedPlaybackDurationBytes, &expectedPlaybackDurationBytes);
    enum FadeStrategy fadeStrategy
        = GetFadeStrategy(pa_bytes_to_usec(expectedPlaybackDurationBytes, &(sinkIn->sample_spec)) / PA_USEC_PER_MSEC);

    uint32_t streamIndex = sinkIn->index;
    uint32_t sinkFadeoutPause = GetFadeoutState(streamIndex);
    if (DoStopDrainFadeout(sinkIn, streamIndex, infoIn->chunk.length)) { sinkFadeoutPause = DO_FADE;}

    if (sinkFadeoutPause == DONE_FADE && (sinkIn->thread_info.state == PA_SINK_INPUT_RUNNING)) {
        silenceData(infoIn, si, streamIndex);
        AUDIO_PRERELEASE_LOGI("after pause fadeout done, silenceData");
        return;
    }
    uint32_t format = (uint32_t)ConvertPaToHdiAdapterFormat(u->format);
    int32_t fadeLenth = GetFadeLenth(fadeStrategy, infoIn->chunk.length, u->ss);

    if (pa_atomic_load(&u->primary.fadingFlagForPrimary) == 1 &&
        u->primary.primarySinkInIndex == (int32_t)sinkIn->index) {
        if (pa_memblock_is_silence(infoIn->chunk.memblock)) {
            AUDIO_PRERELEASE_LOGI("pa_memblock_is_silence");
            return;
        }
        //do fading in
        pa_memchunk_make_writable(&infoIn->chunk, 0);
        void *data = pa_memblock_acquire_chunk(&infoIn->chunk);
        int32_t bufferAvg = GetSimpleBufferAvg(data, infoIn->chunk.length);
        AUDIO_INFO_LOG("do fading in for sink[%{public}d],buffer avg:%{public}d", streamIndex, bufferAvg);
        DoFading(data, fadeLenth, format, (uint32_t)u->ss.channels, 0);
        u->primary.primaryFadingInDone = 1;
        pa_memblock_release(infoIn->chunk.memblock);
    }
    if (sinkFadeoutPause == DO_FADE) {
        //do fading out
        pa_memchunk_make_writable(&infoIn->chunk, 0);
        void *data = pa_memblock_acquire_chunk(&infoIn->chunk);
        int32_t bufferAvg = GetSimpleBufferAvg(data, infoIn->chunk.length);
        AUDIO_INFO_LOG("do fading out for sink[%{public}d],buffer avg:%{public}d", streamIndex, bufferAvg);
        DoFading(data + infoIn->chunk.length - fadeLenth, fadeLenth, format, (uint32_t)u->ss.channels, 1);
        if (fadeStrategy == FADE_STRATEGY_DEFAULT) { SetFadeoutState(streamIndex, DONE_FADE); }
        pa_memblock_release(infoIn->chunk.memblock);
    }
}

static void CheckPrimaryFadeinIsDone(pa_sink *si, pa_sink_input *sinkIn)
{
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    if (u->primary.primaryFadingInDone && u->primary.primarySinkInIndex == (int32_t)sinkIn->index) {
        pa_atomic_store(&u->primary.fadingFlagForPrimary, 0);
    }
}

static void CheckAndPushUidToArr(pa_sink_input *sinkIn, int32_t appsUid[MAX_MIX_CHANNELS], size_t *count)
{
    const char *cstringClientUid = pa_proplist_gets(sinkIn->proplist, "stream.client.uid");
    if (cstringClientUid && (sinkIn->thread_info.state == PA_SINK_INPUT_RUNNING)) {
        appsUid[(*count)] = atoi(cstringClientUid);
        (*count)++;
    }
}

static void SafeRendererSinkUpdateAppsUid(struct SinkAdapter *sinkAdapter,
    const int32_t appsUid[MAX_MIX_CHANNELS], const size_t count)
{
    if (sinkAdapter) {
        sinkAdapter->SinkAdapterUpdateAppsUid(sinkAdapter, appsUid, count);
    }
}

static void RecordEffectChainStatus(bool existFlag, const char *sinkSceneType, const char *sinkSceneMode)
{
    if (g_effectProcessFrameCount == PRINT_INTERVAL_FRAME_COUNT) {
        AUDIO_DEBUG_LOG("Effect Chain Status is %{public}d, scene type is %{public}s, scene mode is %{public}s.",
            existFlag, sinkSceneType, sinkSceneMode);
    }
}

static bool GetExistFlag(pa_sink_input *sinkIn, const char *sinkSceneType, const char *sinkSceneMode)
{
    bool existFlag =
            EffectChainManagerExist(sinkSceneType, sinkSceneMode);
    const char *deviceString = pa_proplist_gets(sinkIn->sink->proplist, PA_PROP_DEVICE_STRING);
    if (pa_safe_streq(deviceString, "remote")) {
        existFlag = false;
    }

    return existFlag;
}

static void ProcessAudioVolume(pa_sink_input *sinkIn, size_t length, pa_memchunk *pchunk, pa_sink *si)
{
    AUTO_CTRACE("hdi_sink::ProcessAudioVolume: len:%zu", length);
    CHECK_AND_RETURN_LOG(sinkIn != NULL, "sinkIn is null");
    CHECK_AND_RETURN_LOG(pchunk != NULL, "pchunk is null");
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");
    const char *sessionIDStr = safeProplistGets(sinkIn->proplist, "stream.sessionID", "NULL");
    uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
    struct VolumeValues *volumes = GetVolumeFromStreamVolumeMap(u, sessionID);
    float volumeEnd = volumes != NULL ? volumes->volume : 1.0f;
    float volumeBeg = volumes != NULL ? volumes->volumeHistory : 1.0f;

    AUTO_CTRACE("Volume, sessionId: %u, volume: %.3f - %.3f", sessionID, volumeBeg, volumeEnd);

    if (pa_memblock_is_silence(pchunk->memblock)) {
        AUTO_CTRACE("hdi_sink::ProcessAudioVolume: is_silence");
        AUDIO_PRERELEASE_LOGI("pa_memblock_is_silence");
    } else {
        AudioRawFormat rawFormat;
        rawFormat.format = (uint32_t)ConvertPaToHdiAdapterFormat(si->sample_spec.format);
        rawFormat.channels = (uint32_t)si->sample_spec.channels;

        pa_memchunk_make_writable(pchunk, 0);
        void *data = pa_memblock_acquire_chunk(pchunk);

        AUDIO_DEBUG_LOG("length:%{public}zu channels:%{public}d format:%{public}d"
            " volumeBeg:%{public}f, volumeEnd:%{public}f",
            length, rawFormat.channels, rawFormat.format, volumeBeg, volumeEnd);
        int32_t ret = ProcessVol(data, length, rawFormat, volumeBeg, volumeEnd);
        if (ret != 0) {
            AUDIO_WARNING_LOG("ProcessVol failed:%{public}d", ret);
        }
        pa_memblock_release(pchunk->memblock);
    }
    if (volumeBeg != volumeEnd) {
        AUDIO_INFO_LOG("sessionID:%{public}u, volumeBeg:%{public}f, volumeEnd:%{public}f",
            sessionID, volumeBeg, volumeEnd);
        SetPreVolume(sessionID, volumeEnd);
        MonitorVolume(sessionID, true);
    }
}

static void HandleFading(pa_sink *si, size_t length, pa_sink_input *sinkIn, pa_mix_info *infoIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    CHECK_AND_RETURN_LOG(sinkIn != NULL, "sinkIn is null");
    CHECK_AND_RETURN_LOG(infoIn != NULL, "infoIn is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    infoIn->userdata = pa_sink_input_ref(sinkIn);
    pa_assert(infoIn->chunk.memblock);
    pa_assert(infoIn->chunk.length > 0);
    PreparePrimaryFading(sinkIn, infoIn, si);
    CheckPrimaryFadeinIsDone(si, sinkIn);

    uint32_t sinkFadeoutPause = GetFadeoutState(sinkIn->index);
    if (!sinkFadeoutPause && (length <= infoIn->chunk.length)) {
        u->streamAvailable++;
    }
}

static void SinkRenderPrimaryStateCheck(pa_mix_info *infoIn, pa_sink_input *sinkIn)
{
    const char *sessionIDStr = safeProplistGets(sinkIn->proplist, "stream.sessionID", "NULL");
    uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
    uint32_t uid = getSinkInputUid(sinkIn);
    if (pa_memblock_is_silence(infoIn->chunk.memblock) && sinkIn->thread_info.state == PA_SINK_INPUT_RUNNING) {
        AUTO_CTRACE("hdi_sink::PrimaryCluster::is_silence");
        RecordPaSilenceState(sessionID, true, PA_PIPE_TYPE_NORMAL, uid);
        pa_sink_input_handle_ohos_underrun(sinkIn);
    } else {
        AUTO_CTRACE("hdi_sink::PrimaryCluster::is_not_silence");
        RecordPaSilenceState(sessionID, false, PA_PIPE_TYPE_NORMAL, uid);
    }
}

static unsigned SinkRenderPrimaryCluster(pa_sink *si, size_t *length, pa_mix_info *infoIn,
    unsigned maxInfo, const char *sceneType)
{
    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryCluster:%s len:%zu", sceneType, *length);

    pa_sink_input *sinkIn;
    unsigned n = 0;
    void *state = NULL;
    size_t mixlength = *length;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "sink is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_RET_LOG(infoIn != NULL, 0, "infoIn is null");

    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_RET_LOG(u != NULL, 0, "u is NULL");

    int32_t appsUid[MAX_MIX_CHANNELS];
    size_t count = 0;
    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL)) && maxInfo > 0) {
        CheckAndPushUidToArr(sinkIn, appsUid, &count);
        const char *sSceneType = pa_proplist_gets(sinkIn->proplist, "scene.type");
        const char *sSceneMode = pa_proplist_gets(sinkIn->proplist, "scene.mode");
        bool existFlag = GetExistFlag(sinkIn, sSceneType, sSceneMode);
        bool sceneTypeFlag = EffectChainManagerSceneCheck(sSceneType, sceneType);
        AUTO_CTRACE("hdi_sink::PrimaryCluster:existFlag:%d sceneTypeFlag:%d", existFlag, sceneTypeFlag);
        if ((IsInnerCapturer(sinkIn) && IsCaptureSilently()) || !InputIsPrimary(sinkIn)) {
            AUTO_CTRACE("hdi_sink::PrimaryCluster:InnerCapturer and CaptureSilently or not primary");
            continue;
        } else if ((sceneTypeFlag && existFlag) || (pa_safe_streq(sceneType, "EFFECT_NONE") && (!existFlag))) {
            RecordEffectChainStatus(existFlag, sSceneType, sSceneMode);
            pa_sink_input_assert_ref(sinkIn);
            updateResampler(sinkIn, sceneType, false, si);

            AUTO_CTRACE("hdi_sink::PrimaryCluster:%u len:%zu", sinkIn->index, *length);
            pa_sink_input_peek(sinkIn, *length, &infoIn->chunk, &infoIn->volume);

            if (mixlength == 0 || infoIn->chunk.length < mixlength) {mixlength = infoIn->chunk.length;}

            ProcessAudioVolume(sinkIn, mixlength, &infoIn->chunk, si);

            SinkRenderPrimaryStateCheck(infoIn, sinkIn);
            HandleFading(si, *length, sinkIn, infoIn);

            infoIn++;
            n++;
            maxInfo--;
        }
    }

    SafeRendererSinkUpdateAppsUid(u->primary.sinkAdapter, appsUid, count);

    if (mixlength > 0) { *length = mixlength;}

    return n;
}

static bool IsSilentData(pa_memchunk *pchunk)
{
    CHECK_AND_RETURN_RET_LOG(pchunk != NULL, false, "pchunk is null");
    char *data = pa_memblock_acquire_chunk(pchunk);
    for (size_t i = 0; i < pchunk->length; i++) {
        if (data[i] != 0) {
            pa_memblock_release(pchunk->memblock);
            return false;
        }
    }
    pa_memblock_release(pchunk->memblock);
    return true;
}

static void PrepareMultiChannelFading(pa_sink_input *sinkIn, pa_mix_info *infoIn, pa_sink *si)
{
    CHECK_AND_RETURN_LOG(sinkIn != NULL, "sinkIn is null");
    CHECK_AND_RETURN_LOG(infoIn != NULL, "infoIn is null");
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    uint32_t streamIndex = sinkIn->index;
    uint32_t sinkFadeoutPause = GetFadeoutState(streamIndex);
    if (sinkFadeoutPause == DONE_FADE) {
        silenceData(infoIn, si, streamIndex);
        AUDIO_PRERELEASE_LOGI("silenceData.");
        return;
    }
    if (IsSilentData(&infoIn->chunk)) {
        AUDIO_PRERELEASE_LOGI("silent data, no need to fade in.");
        return;
    }
    uint32_t format = (uint32_t)ConvertPaToHdiAdapterFormat(u->format);
    if (pa_atomic_load(&u->multiChannel.fadingFlagForMultiChannel) == 1 &&
        u->multiChannel.multiChannelSinkInIndex == (int32_t)sinkIn->index) {
        if (pa_memblock_is_silence(infoIn->chunk.memblock)) {
            AUDIO_DEBUG_LOG("pa_memblock_is_silence");
            return;
        }
        //do fading in
        pa_memchunk_make_writable(&infoIn->chunk, 0);
        void *data = pa_memblock_acquire_chunk(&infoIn->chunk);
        DoFading(data, infoIn->chunk.length, format, (uint32_t)u->ss.channels, 0);
        u->multiChannel.multiChannelFadingInDone = 1;
        pa_memblock_release(infoIn->chunk.memblock);
    }
    if (sinkFadeoutPause == DO_FADE) {
        //do fading out
        pa_memchunk_make_writable(&infoIn->chunk, 0);
        void *data = pa_memblock_acquire_chunk(&infoIn->chunk);
        DoFading(data, infoIn->chunk.length, format, (uint32_t)u->ss.channels, 1);
        SetFadeoutState(streamIndex, DONE_FADE);
        pa_memblock_release(infoIn->chunk.memblock);
    }
}

static void CheckMultiChannelFadeinIsDone(pa_sink *si, pa_sink_input *sinkIn)
{
    CHECK_AND_RETURN_LOG(sinkIn != NULL, "sinkIn is null");
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    if (u->multiChannel.multiChannelFadingInDone &&
        u->multiChannel.multiChannelSinkInIndex == (int32_t)sinkIn->index) {
        pa_atomic_store(&u->multiChannel.fadingFlagForMultiChannel, 0);
    }
}

static void SinkRenderMultiChannelStateCheck(pa_sink *si, pa_mix_info *infoIn, pa_sink_input *sinkIn)
{
    const char *sessionIDStr = safeProplistGets(sinkIn->proplist, "stream.sessionID", "NULL");
    uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
    const char *sinkSpatializationEnabled = pa_proplist_gets(sinkIn->proplist, "spatialization.enabled");
    uint32_t uid = getSinkInputUid(sinkIn);
    if (pa_memblock_is_silence(infoIn->chunk.memblock) && sinkIn->thread_info.state == PA_SINK_INPUT_RUNNING) {
        AUTO_CTRACE("hdi_sink::SinkRenderMultiChannelCluster::is_silence");
        RecordPaSilenceState(sessionID, true, PA_PIPE_TYPE_MULTICHANNEL, uid);
        pa_sink_input_handle_ohos_underrun(sinkIn);
    } else if (pa_safe_streq(sinkSpatializationEnabled, "true")) {
        AUTO_CTRACE("hdi_sink::SinkRenderMultiChannelCluster::is_not_silence");
        RecordPaSilenceState(sessionID, false, PA_PIPE_TYPE_MULTICHANNEL, uid);
        pa_atomic_store(&sinkIn->isFirstReaded, 1);
    }
    PrepareMultiChannelFading(sinkIn, infoIn, si);
    CheckMultiChannelFadeinIsDone(si, sinkIn);
}

static unsigned SinkRenderMultiChannelCluster(pa_sink *si, size_t *length, pa_mix_info *infoIn,
    unsigned maxInfo)
{
    pa_sink_input *sinkIn;
    unsigned n = 0;
    void *state = NULL;
    size_t mixlength = *length;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_RET_LOG(infoIn != NULL, 0, "infoIn is null");

    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_RET_LOG(u != NULL, 0, "u is NULL");

    int32_t appsUid[MAX_MIX_CHANNELS];
    size_t count = 0;

    while ((sinkIn = pa_hashmap_iterate(si->thread_info.inputs, &state, NULL)) && maxInfo > 0) {
        CheckAndPushUidToArr(sinkIn, appsUid, &count);
        int32_t sinkChannels = sinkIn->sample_spec.channels;
        const char *sinkSceneType = pa_proplist_gets(sinkIn->proplist, "scene.type");
        const char *sinkSceneMode = pa_proplist_gets(sinkIn->proplist, "scene.mode");
        bool existFlag = EffectChainManagerExist(sinkSceneType, sinkSceneMode);
        if (!existFlag && sinkChannels > PRIMARY_CHANNEL_NUM) {
            pa_sink_input_assert_ref(sinkIn);
            updateResampler(sinkIn, NULL, true, si);
            pa_sink_input_peek(sinkIn, *length, &infoIn->chunk, &infoIn->volume);

            if (mixlength == 0 || infoIn->chunk.length < mixlength) {mixlength = infoIn->chunk.length;}

            ProcessAudioVolume(sinkIn, mixlength, &infoIn->chunk, si);

            SinkRenderMultiChannelStateCheck(si, infoIn, sinkIn);
            infoIn->userdata = pa_sink_input_ref(sinkIn);
            pa_assert(infoIn->chunk.memblock);
            pa_assert(infoIn->chunk.length > 0);

            infoIn++;
            n++;
            maxInfo--;
        }
    }

    SafeRendererSinkUpdateAppsUid(u->multiChannel.sinkAdapter, appsUid, count);

    if (mixlength > 0) {
        *length = mixlength;
    }

    return n;
}

static int32_t SinkRenderPrimaryPeek(pa_sink *si, pa_memchunk *chunkIn, const char *sceneType)
{
    pa_mix_info info[MAX_MIX_CHANNELS];
    unsigned n;
    size_t length;
    size_t blockSizeMax;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        AUTO_CTRACE("hdi_sink::Primary:PA_SINK_SUSPENDED");
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    length = chunkIn->length;
    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);
    n = SinkRenderPrimaryCluster(si, &length, info, MAX_MIX_CHANNELS, sceneType);

    AUTO_CTRACE("hdi_sink:Primary:SinkRenderPrimaryMix:%u len:%zu", n, length);
    SinkRenderPrimaryMix(si, length, info, n, chunkIn);

    SinkRenderPrimaryInputsDrop(si, info, n, chunkIn);
    pa_sink_unref(si);
    return n;
}

static int32_t SinkRenderMultiChannelPeek(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_mix_info info[MAX_MIX_CHANNELS];
    unsigned n;
    size_t length;
    size_t blockSizeMax;

    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        AUTO_CTRACE("hdi_sink::MultiCh:PA_SINK_SUSPENDED");
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    length = chunkIn->length;
    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    n = SinkRenderMultiChannelCluster(si, &length, info, MAX_MIX_CHANNELS);

    AUTO_CTRACE("hdi_sink:MultiCh:SinkRenderPrimaryMix:%u len:%zu", n, length);
    SinkRenderPrimaryMix(si, length, info, n, chunkIn);

    SinkRenderMultiChannelInputsDrop(si, info, n, chunkIn);
    pa_sink_unref(si);

    return n;
}

static int32_t SinkRenderPrimaryGetData(pa_sink *si, pa_memchunk *chunkIn, const char *sceneType)
{
    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryGetData:%s", sceneType);
    pa_memchunk chunk;
    size_t l;
    size_t d;
    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    l = chunkIn->length;
    d = 0;
    int32_t nSinkInput = 0;
    while (l > 0) {
        chunk = *chunkIn;
        chunk.index += d;
        chunk.length -= d;

        nSinkInput = SinkRenderPrimaryPeek(si, &chunk, sceneType);

        d += chunk.length;
        l -= chunk.length;
    }
    pa_sink_unref(si);

    return nSinkInput;
}

static int32_t SinkRenderMultiChannelGetData(pa_sink *si, pa_memchunk *chunkIn)
{
    pa_memchunk chunk;
    size_t l;
    size_t d;
    CHECK_AND_RETURN_RET_LOG(si != NULL, 0, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_RET_LOG(chunkIn != NULL, 0, "chunkIn is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->memblock != NULL, 0, "chunkIn->memblock is null");
    CHECK_AND_RETURN_RET_LOG(chunkIn->length > 0, 0, "chunkIn->length < 0");
    pa_assert(pa_frame_aligned(chunkIn->length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        pa_silence_memchunk(chunkIn, &si->sample_spec);
        return 0;
    }

    pa_sink_ref(si);

    l = chunkIn->length;
    d = 0;

    int32_t nSinkInput = 0;
    while (l > 0) {
        chunk = *chunkIn;
        chunk.index += d;
        chunk.length -= d;

        nSinkInput = SinkRenderMultiChannelPeek(si, &chunk);

        d += chunk.length;
        l -= chunk.length;
    }

    pa_sink_unref(si);

    return nSinkInput;
}

static void SinkRenderPrimaryAfterProcess(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");
    int32_t bitSize = (int32_t) pa_sample_size_of_format(u->format);
    u->bufferAttr->numChanIn = DEFAULT_IN_CHANNEL_NUM;
    void *dst = pa_memblock_acquire_chunk(chunkIn);
    int32_t frameLen = bitSize > 0 ? ((int32_t) length / bitSize) : 0;
    if (u->isLimiterCreated) {
        // limiter only support 2 channels and float format
        LimiterManagerProcess((int32_t)u->sink->index, frameLen, u->bufferAttr->tempBufOut, u->bufferAttr->bufOut);
        ConvertFromFloat(u->format, frameLen, u->bufferAttr->bufOut, dst);
    } else {
        ConvertFromFloat(u->format, frameLen, u->bufferAttr->tempBufOut, dst);
    }
    chunkIn->index = 0;
    chunkIn->length = length;
    pa_memblock_release(chunkIn->memblock);
}

static char *HandleSinkSceneType(struct Userdata *u, time_t currentTime, int32_t i)
{
    char *sinkSceneType = SCENE_TYPE_SET[i];
    if (g_effectAllStreamVolumeZeroMap[i] && PA_SINK_IS_RUNNING(u->sink->thread_info.state) &&
        difftime(currentTime, g_effectStartVolZeroTimeMap[i]) > WAIT_CLOSE_PA_OR_EFFECT_TIME) {
        sinkSceneType = SCENE_TYPE_SET[SCENE_TYPE_NUM - 1]; // EFFECT_NONE
        if (!g_effectHaveDisabledMap[i]) {
            AUDIO_INFO_LOG("volume change to zero over %{public}ds, close effect:%{public}s success.",
                WAIT_CLOSE_PA_OR_EFFECT_TIME, SCENE_TYPE_SET[i]);
            g_effectHaveDisabledMap[i] = true;
            g_effectStartVolZeroTimeMap[i] = 0;
        }
    } else {
        sinkSceneType = SCENE_TYPE_SET[i];
        if (g_effectHaveDisabledMap[i]) {
            g_effectHaveDisabledMap[i] = false;
            AUDIO_INFO_LOG("volume change to non zero, open effect:%{public}s success. ", SCENE_TYPE_SET[i]);
        }
    }
    return sinkSceneType;
}

static char *CheckAndDealEffectZeroVolume(struct Userdata *u, time_t currentTime, const char *sceneType)
{
    int32_t i;
    for (i = 0; i < SCENE_TYPE_NUM; i++) {
        if (!strcmp(SCENE_TYPE_SET[i], sceneType)) {
            break;
        }
    }

    i = i == SCENE_TYPE_NUM ? SCENE_TYPE_NUM - 1 : i;
    void *state = NULL;
    pa_sink_input *input;
    g_effectAllStreamVolumeZeroMap[i] = true;
    while ((input = pa_hashmap_iterate(u->sink->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(input);
        if (input->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        const char *sinkSceneTypeTmp = pa_proplist_gets(input->proplist, "scene.type");
        const char *streamType = safeProplistGets(input->proplist, "stream.type", "NULL");
        const char *sessionIDStr = safeProplistGets(input->proplist, "stream.sessionID", "NULL");
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        struct VolumeValues *volumes = GetVolumeFromStreamVolumeMap(u, sessionID);
        bool isZeroVolume = IsZeroVolume(volumes != NULL ? volumes->volume : 1.0f);
        if (EffectChainManagerSceneCheck(sinkSceneTypeTmp, SCENE_TYPE_SET[i]) && !isZeroVolume) {
            g_effectAllStreamVolumeZeroMap[i] = false;
            g_effectStartVolZeroTimeMap[i] = 0;
            AUDIO_DEBUG_LOG("SCENE_TYPE_SET[%{public}d]:%{public}s for streamtype:[%{public}s]'s"
                " volume is non zero, this effect all streamtype is non zero volume.", i,
                SCENE_TYPE_SET[i], streamType);
            break;
        }
    }

    if (g_effectAllStreamVolumeZeroMap[i] && !g_effectHaveDisabledMap[i] && (g_effectStartVolZeroTimeMap[i] == 0) &&
        PA_SINK_IS_RUNNING(u->sink->thread_info.state)) {
        AUDIO_INFO_LOG("Timing begins, will close [%{public}s] effect after [%{public}d]s", SCENE_TYPE_SET[i],
            WAIT_CLOSE_PA_OR_EFFECT_TIME);
        time(&g_effectStartVolZeroTimeMap[i]);
    }
    char *handledSceneType = HandleSinkSceneType(u, currentTime, i);
    AUDIO_DEBUG_LOG("handle sink scene type:%{public}s", handledSceneType);
    return handledSceneType;
}

static void CheckOnlyPrimarySpeakerPaLoading(struct Userdata *u)
{
    if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1 && strcmp(u->sink->name, "Speaker")) {
        AUDIO_DEBUG_LOG("Have new routing:[%{public}s] on primary, dont close it.", u->sink->name);
        u->primary.onlyPrimarySpeakerPaLoading = false;
        u->primary.speakerPaAllStreamVolumeZero = false;
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }

    if (strcmp(u->primary.sinkAdapter->deviceClass, "primary")) {
        AUDIO_DEBUG_LOG("Sink[%{public}s] -- no primary, dont close it.", u->primary.sinkAdapter->deviceClass);
        u->primary.onlyPrimarySpeakerPaLoading = false;
        u->primary.speakerPaAllStreamVolumeZero = false;
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }

    if (u->offload.isHDISinkStarted || u->multiChannel.isHDISinkStarted) {
        AUDIO_DEBUG_LOG("offload or multichannel started, dont close it.");
        u->primary.onlyPrimarySpeakerPaLoading = false;
        u->primary.speakerPaAllStreamVolumeZero = false;
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }

    if (PA_SINK_IS_RUNNING(u->sink->thread_info.state) && !u->primary.onlyPrimarySpeakerPaLoading &&
        u->primary.paHaveDisabled) {
        int32_t ret = u->primary.sinkAdapter->SinkAdapterSetPaPower(u->primary.sinkAdapter, 1);
        AUDIO_INFO_LOG("sink running, open closed pa:[%{public}s] -- [%{public}s], ret:%{public}d", u->sink->name,
            (ret == 0 ? "success" : "failed"), ret);
        u->primary.paHaveDisabled = false;
        u->primary.speakerPaHaveClosed = false;
    }
}

static void HandleClosePa(struct Userdata *u)
{
    if (!u->primary.paHaveDisabled) {
        int32_t ret = u->primary.sinkAdapter->SinkAdapterSetPaPower(u->primary.sinkAdapter, 0);
        AUDIO_INFO_LOG("Speaker pa volume change to zero over [%{public}d]s, close %{public}s pa [%{public}s], "
            "ret:%{public}d", WAIT_CLOSE_PA_OR_EFFECT_TIME, u->sink->name, (ret == 0 ? "success" : "failed"), ret);
        u->primary.paHaveDisabled = true;
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
        u->primary.speakerPaHaveClosed = true;
        time(&u->primary.speakerPaClosedTime);
    }
}

static void HandleOpenPa(struct Userdata *u)
{
    if (u->primary.paHaveDisabled) {
        int32_t ret = u->primary.sinkAdapter->SinkAdapterSetPaPower(u->primary.sinkAdapter, 1);
        AUDIO_INFO_LOG("volume change to non zero, open closed pa:[%{public}s] -- [%{public}s], ret:%{public}d",
            u->sink->name, (ret == 0 ? "success" : "failed"), ret);
        u->primary.paHaveDisabled = false;
        u->primary.speakerPaHaveClosed = false;
    }
}

static void CheckAndDealSpeakerPaZeroVolume(struct Userdata *u, time_t currentTime)
{
    if (!u->primary.onlyPrimarySpeakerPaLoading) {
        AUDIO_DEBUG_LOG("Not only the speaker pa, dont deal speaker pa.");
        return;
    }
    void *state = NULL;
    pa_sink_input *input;
    while ((input = pa_hashmap_iterate(u->sink->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(input);
        if (input->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        const char *sessionIDStr = safeProplistGets(input->proplist, "stream.sessionID", "NULL");
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        struct VolumeValues *volumes = GetVolumeFromStreamVolumeMap(u, sessionID);
        bool isZeroVolume = IsZeroVolume(volumes != NULL ? volumes->volume : 1.0f);
        if (!strcmp(u->sink->name, "Speaker") && !isZeroVolume) {
            u->primary.speakerPaAllStreamVolumeZero = false;
            u->primary.speakerPaAllStreamStartVolZeroTime = 0;
            break;
        }
    }

    if (u->primary.speakerPaAllStreamVolumeZero && !u->primary.paHaveDisabled &&
        (u->primary.speakerPaAllStreamStartVolZeroTime == 0) && PA_SINK_IS_RUNNING(u->sink->thread_info.state)) {
        AUDIO_INFO_LOG("Timing begins, will close speaker after [%{public}d]s", WAIT_CLOSE_PA_OR_EFFECT_TIME);
        time(&u->primary.speakerPaAllStreamStartVolZeroTime);
    }
    if (u->primary.speakerPaAllStreamVolumeZero && PA_SINK_IS_RUNNING(u->sink->thread_info.state) &&
        difftime(currentTime, u->primary.speakerPaAllStreamStartVolZeroTime) > WAIT_CLOSE_PA_OR_EFFECT_TIME &&
        u->primary.sinkAdapter->SinkAdapterGetAudioScene(u->primary.sinkAdapter) == 0) {
        HandleClosePa(u);
    } else {
        HandleOpenPa(u);
    }

    if (u->primary.speakerPaHaveClosed &&
        difftime(currentTime, u->primary.speakerPaClosedTime) >= MONITOR_CLOSE_PA_TIME_SEC) {
        time(&u->primary.speakerPaClosedTime);
        AUDIO_INFO_LOG("Speaker pa have closed [%{public}d]s.", MONITOR_CLOSE_PA_TIME_SEC);
    }
}

static void UpdateStreamAvailableMap(struct Userdata *u, const char *sceneType)
{
    if (u->streamAvailableMap == NULL) {
        AUDIO_ERR_LOG("streamAvailableMap is null");
        return;
    }
    uint32_t *num = (uint32_t *)pa_hashmap_get(u->streamAvailableMap, sceneType);
    if (num == NULL) {
        num = pa_xnew0(uint32_t, 1);
        (*num) = 0;
    }
    int32_t fadeDirection = (u->streamAvailable != 0) && ((*num) == 0) ? 0 :
                    (u->streamAvailable == 0 && (*num) != 0) ? 1 : -1;
    int32_t outLength = u->bufferAttr->frameLen * u->bufferAttr->numChanOut * sizeof(float);
    if (fadeDirection != -1) {
        AUDIO_INFO_LOG("do %{public}s for MIXED DATA", fadeDirection ? "fade-out" : "fade-in");
        DoFading(u->bufferAttr->bufOut, outLength, (uint32_t)SAMPLE_F32, (uint32_t)u->ss.channels, fadeDirection);
    }
    if (u->streamAvailable == 0 && (*num) == 0) {
        memset_s(u->bufferAttr->bufOut, outLength, 0, outLength);
    }

    if (pa_hashmap_get(u->streamAvailableMap, sceneType) != NULL) {
        (*num) = u->streamAvailable;
    } else {
        char *scene = strdup(sceneType);
        if (scene != NULL) {
            (*num) = u->streamAvailable;
            if (pa_hashmap_put(u->streamAvailableMap, scene, num) != 0) {
                AUDIO_ERR_LOG("pa_hashmap_put failed");
                free(scene);
                pa_xfree(num);
            }
        }
    }
}

/* For ResampleAffterEffectChain, update input channellayout and sample spec */
static pa_resampler *UpdateResamplerInChannelMap(const char *sceneType, struct Userdata *u)
{
    pa_resampler *resampler = (pa_resampler *)pa_hashmap_get(u->sceneToResamplerMap, sceneType);
    if (resampler == NULL) {
        return NULL;
    }
    pa_channel_map inChannelMap;
    inChannelMap.channels = u->bufferAttr->numChanOut;
    ConvertChLayoutToPaChMap(u->bufferAttr->outChanLayout, &inChannelMap);
    if (!pa_channel_map_valid(&inChannelMap)) {
        AUDIO_ERR_LOG("UpdateResampler: invalid channelmap, channels [%{public}d], channellayout [%{public}" PRIu64 "]",
            u->bufferAttr->numChanOut, u->bufferAttr->outChanLayout);
        inChannelMap.channels = DEFAULT_NUM_CHANNEL;
        ConvertChLayoutToPaChMap(DEFAULT_CHANNELLAYOUT, &inChannelMap);
    }
    if ((!pa_channel_map_equal(pa_resampler_input_channel_map(resampler), &inChannelMap))) {
        // for now, use sample_spec from sink
        pa_sample_spec inSpec = *(pa_resampler_input_sample_spec(resampler));
        inSpec.channels = (uint8_t)u->bufferAttr->numChanOut;
        AUDIO_INFO_LOG("UpdateResampler: sceneType [%{public}s], input channels [%{public}d], "
            "sample rate [%{public}d], format[%{public}d]. "
            "new input channels [%{public}d], sample rate [%{public}d], format[%{public}d]",
            (char *)sceneType, pa_resampler_input_sample_spec(resampler)->channels,
            pa_resampler_input_sample_spec(resampler)->rate, pa_resampler_input_sample_spec(resampler)->format,
            inChannelMap.channels, inSpec.rate, inSpec.format);
        pa_sample_spec sinkSpec = *(pa_resampler_output_sample_spec(resampler));
        pa_channel_map sinkChannelMap = *(pa_resampler_output_channel_map(resampler));
        char *dupSceneType = strdup(sceneType);
        if (dupSceneType == NULL) {
            AUDIO_ERR_LOG("UpdateResampler: [%{public}s], allocate new char fail! return old resampler!",
                sceneType);
            return resampler;
        }
        pa_hashmap_remove_and_free(u->sceneToResamplerMap, sceneType);
        resampler = pa_resampler_new(
            u->sink->core->mempool,
            &inSpec, &inChannelMap,
            &sinkSpec, &sinkChannelMap,
            u->sink->core->lfe_crossover_freq,
            PA_RESAMPLER_AUTO, PA_RESAMPLER_VARIABLE_RATE);
        pa_hashmap_put(u->sceneToResamplerMap, (void *)dupSceneType, (void *)resampler);
    }
    return resampler;
}

static void ResampleAfterEffectChain(const char* sceneType, struct Userdata *u)
{
    if (sceneType == NULL || pa_safe_streq(sceneType, "EFFECT_NONE") || u == NULL) {
        return;
    }
    pa_resampler *resampler = UpdateResamplerInChannelMap(sceneType, u);
    CHECK_AND_RETURN_LOG(resampler != NULL, "ResampleAfterEffectChain: resampler is null!");
    pa_memchunk unsampledChunk;
    unsampledChunk.length = (uint32_t)u->bufferAttr->frameLen * pa_resampler_input_sample_spec(resampler)->channels *
        sizeof(float);
    unsampledChunk.memblock = pa_memblock_new(u->core->mempool, unsampledChunk.length);
    void *dst = pa_memblock_acquire(unsampledChunk.memblock);
    if (dst == NULL) {
        AUDIO_ERR_LOG("ResampleAfterEffectChain: pa_memblock_acquire dst fail! skip resampler_run!");
        pa_memblock_release(unsampledChunk.memblock);
        pa_memblock_unref(unsampledChunk.memblock);
        return;
    }
    int32_t ret = memcpy_s(dst, unsampledChunk.length, u->bufferAttr->bufOut, unsampledChunk.length);
    CHECK_AND_RETURN_LOG(ret == 0, "ResampleAfterEffectChain: copy from bufOut to unsampled chunk fail!");
    pa_memblock_release(unsampledChunk.memblock);
    pa_memchunk sampledChunk;
    pa_resampler_run(resampler, &unsampledChunk, &sampledChunk);
    void *src = pa_memblock_acquire(sampledChunk.memblock);
    if (src == NULL) {
        AUDIO_ERR_LOG("ResampleAfterEffectChain: pa_memblock_acquire src fail! resampler_run fail!");
        pa_memblock_release(sampledChunk.memblock);
        pa_memblock_unref(unsampledChunk.memblock);
        pa_memblock_unref(sampledChunk.memblock);
        return;
    }
    ret = memcpy_s(u->bufferAttr->bufOut, sampledChunk.length, src, sampledChunk.length);
    CHECK_AND_RETURN_LOG(ret == 0, "ResampleAfterEffectChain: copy from sampled chunk to bufOut fail!");
    pa_memblock_release(sampledChunk.memblock);
    pa_memblock_unref(unsampledChunk.memblock);
    pa_memblock_unref(sampledChunk.memblock);
}

static void PrimaryEffectProcess(struct Userdata *u, char *sinkSceneType, const char *sceneType, size_t outBufferLen)
{
    AUTO_CTRACE("hdi_sink::EffectChainManagerProcess:%s", sinkSceneType);
    EffectChainManagerProcess(sinkSceneType, u->bufferAttr);
    UpdateStreamAvailableMap(u, sinkSceneType);
    ResampleAfterEffectChain(sceneType, u);
    for (uint32_t k = 0; k < outBufferLen; k++) {
        u->bufferAttr->tempBufOut[k] += u->bufferAttr->bufOut[k];
    }
    u->bufferAttr->numChanIn = DEFAULT_IN_CHANNEL_NUM;
}

static void GetHashMap(struct Userdata *u, const char *sceneType)
{
    uint32_t curNum = EffectChainManagerGetSceneCount(sceneType);
    uint32_t *num = (uint32_t *)pa_hashmap_get(u->sceneToCountMap, sceneType);
    if (curNum) {
        if (num) {
            (*num) = curNum;
        } else {
            char *scene = strdup(sceneType);
            int32_t ret = 0;
            if (scene) {
                num = pa_xnew0(uint32_t, 1);
                *num = curNum;
                ret = pa_hashmap_put(u->sceneToCountMap, scene, num);
            }
            if (ret != 0) {
                AUDIO_ERR_LOG("pa_hashmap_put failed");
                free(scene);
                pa_xfree(num);
            }
        }
    } else if (num) {
        pa_hashmap_remove_and_free(u->sceneToCountMap, sceneType);
    }
}

static void UpdateSceneToCountMap(struct Userdata *u)
{
    if (u->sceneToCountMap == NULL) {
        return;
    }
    for (int32_t i = 0; i < SCENE_TYPE_NUM - 1; i++) {
        GetHashMap(u, SCENE_TYPE_SET[i]);
    }
}

static void *AllocateBuffer(size_t size)
{
    if (size > 0 && size <= sizeof(float) * DEFAULT_FRAMELEN * IN_CHANNEL_NUM_MAX) {
        return malloc(size);
    } else {
        return NULL;
    }
}

static bool AllocateEffectBuffer(struct Userdata *u)
{
    if (u->bufferAttr == NULL) {
        return false;
    }
    float **buffers[] = { &u->bufferAttr->bufIn, &u->bufferAttr->bufOut,
        &u->bufferAttr->tempBufIn, &u->bufferAttr->tempBufOut };
    size_t numBuffers = sizeof(buffers) / sizeof(buffers[0]);
    for (size_t i = 0; i < numBuffers; i++) {
        *buffers[i] = (float *)AllocateBuffer(u->processSize);
        if (*buffers[i] == NULL) {
            AUDIO_ERR_LOG("failed to allocate effect buffer");
            FreeEffectBuffer(u);
            return false;
        }
    }
    return true;
}

static void FreeEffectBuffer(struct Userdata *u)
{
    if (u->bufferAttr == NULL) {
        return;
    }
    float **buffers[] = { &u->bufferAttr->bufIn, &u->bufferAttr->bufOut,
        &u->bufferAttr->tempBufIn, &u->bufferAttr->tempBufOut };
    size_t numBuffers = sizeof(buffers) / sizeof(buffers[0]);
    for (size_t i = 0; i < numBuffers; i++) {
        free(*buffers[i]);
        *buffers[i] = NULL;
    }
}

static void ResetBufferAttr(struct Userdata *u)
{
    size_t memsetInLen = sizeof(float) * DEFAULT_FRAMELEN * IN_CHANNEL_NUM_MAX;
    size_t memsetOutLen = sizeof(float) * DEFAULT_FRAMELEN * IN_CHANNEL_NUM_MAX;
    if (memset_s(u->bufferAttr->tempBufIn, u->processSize, 0, memsetInLen) != EOK) {
        AUDIO_WARNING_LOG("SinkRenderBufIn memset_s failed");
    }
    if (memset_s(u->bufferAttr->tempBufOut, u->processSize, 0, memsetOutLen) != EOK) {
        AUDIO_WARNING_LOG("SinkRenderBufOut memset_s failed");
    }
}

static void SceneToResamplerMapAddNewScene(pa_hashmap *sceneToResamplerMap, const char *sceneType, pa_sink *si)
{
    // output from effectchain must be 2 channels, 48k sample rate
    pa_sample_spec inSpec = {
        .format = PA_SAMPLE_FLOAT32LE,
        .rate = EFFECT_PROCESS_RATE,
        .channels = DEFAULT_NUM_CHANNEL
    };
    pa_sample_spec outSpec = si->sample_spec;
    outSpec.format = PA_SAMPLE_FLOAT32LE;
    AUDIO_INFO_LOG("SceneToResamplerMap new [%{public}s], output channels[%{public}d], sample rate[%{public}d]"
        ", format[%{public}d]", (char *)sceneType, outSpec.channels, outSpec.rate, outSpec.format);
    pa_resampler *resampler = pa_resampler_new(
        si->core->mempool,
        &inSpec, &si->channel_map,
        &outSpec, &si->channel_map,
        si->core->lfe_crossover_freq,
        PA_RESAMPLER_AUTO, PA_RESAMPLER_VARIABLE_RATE);
    char* newSceneType = strdup(sceneType);
    if (newSceneType == NULL) {
        AUDIO_ERR_LOG("SceneToResamplerMap: [%{public}s], allocate new char fail!", (char *)sceneType);
        return;
    }
    pa_hashmap_put(sceneToResamplerMap, (void *)newSceneType, (void *)resampler);
}

static void UpdateResamplerOutChannelMap(pa_hashmap *sceneToResamplerMap, const char *sceneType, pa_sink *si)
{
    // output from effect chain must be 2 channels, 48000Hz, float
    pa_sample_spec inSpec = {
        .format = PA_SAMPLE_FLOAT32LE,
        .rate = EFFECT_PROCESS_RATE,
        .channels = DEFAULT_NUM_CHANNEL
    };
    pa_sample_spec outSpec = si->sample_spec;
    outSpec.format = PA_SAMPLE_FLOAT32LE;
    char *dupSceneType = strdup(sceneType);
    if (dupSceneType == NULL) {
        AUDIO_ERR_LOG("SceneToResamplerMap: [%{public}s], allocate new char fail!", (char *)sceneType);
        return;
    }
    pa_hashmap_remove_and_free(sceneToResamplerMap, sceneType);
    pa_resampler *resampler = pa_resampler_new(
        si->core->mempool,
        &inSpec, &si->channel_map,
        &outSpec, &si->channel_map,
        si->core->lfe_crossover_freq,
        PA_RESAMPLER_AUTO, PA_RESAMPLER_VARIABLE_RATE);
    pa_hashmap_put(sceneToResamplerMap, (void *)dupSceneType, (void *)resampler);
}

static void UpdateSceneToResamplerMap(pa_hashmap *sceneToResamplerMap, pa_hashmap *sceneToCountMap, pa_sink *si)
{
    // sample rate and channellayout from audio effect chain -> resampler -> sample rate and channellayout for the sink
    const void* sceneType = NULL;
    void* count = NULL;
    while ((pa_hashmap_iterate(sceneToCountMap, &count, &sceneType))) {
        if (pa_safe_streq(sceneType, "EFFECT_NONE")) {
            continue;
        }
        pa_resampler* resampler = NULL;
        resampler = (pa_resampler*)pa_hashmap_get(sceneToResamplerMap, sceneType);
        if (resampler == NULL) {
            SceneToResamplerMapAddNewScene(sceneToResamplerMap, sceneType, si);
        } else {
            pa_sample_spec sinkSpec = si->sample_spec;
            sinkSpec.format = PA_SAMPLE_FLOAT32LE;
            if (pa_sample_spec_equal(pa_resampler_output_sample_spec(resampler), &sinkSpec) &&
                pa_channel_map_equal(pa_resampler_output_channel_map(resampler), &si->channel_map)) {
                continue;
            }
            AUDIO_INFO_LOG("SceneToResamplerMap: [%{public}s], output channels [%{public}d], sample rate [%{public}d],"
                " format[%{public}d]. new output channels [%{public}d], sample rate [%{public}d], format[%{public}d].",
                (char *)sceneType, pa_resampler_output_sample_spec(resampler)->channels,
                pa_resampler_output_sample_spec(resampler)->rate, pa_resampler_output_sample_spec(resampler)->format,
                sinkSpec.channels, sinkSpec.rate, sinkSpec.format);
            UpdateResamplerOutChannelMap(sceneToResamplerMap, sceneType, si);
        }
    }
    // delete entries that are not in scenemap
    void* resampler = NULL;
    while ((pa_hashmap_iterate(sceneToResamplerMap, &resampler, &sceneType))) {
        if (pa_hashmap_get(sceneToCountMap, sceneType) == NULL) {
            AUDIO_INFO_LOG("SceneToResamplerMap: sceneType [%{public}s] is removed", (char *)sceneType);
            pa_hashmap_remove_and_free(sceneToResamplerMap, sceneType);
        }
    }
}

uint32_t GetFrameSize(const char *sinkSceneType, size_t sinkLengthDefault, uint32_t byteSize, int processChannels)
{
    if (pa_safe_streq(sinkSceneType, "EFFECT_NONE")) {
        size_t sinkByteLength = sinkLengthDefault * (uint32_t)processChannels / (uint32_t)DEFAULT_IN_CHANNEL_NUM;
        uint32_t sinkLength = byteSize > 0 ? ((uint32_t)sinkByteLength / byteSize) : 0;
        return sinkLength;
    } else {
        uint32_t effectFrameSize = (uint32_t)EFFECT_FRAME_LENGTH_MONO * (uint32_t)processChannels;
        return effectFrameSize;
    }
}

static void UpdateStreamVolumeMap(struct Userdata *u)
{
    CHECK_AND_RETURN_LOG(u != NULL && u->streamVolumeMap != NULL, "streamVolumeMap is null");
    void *state = NULL;
    pa_sink_input *input;
    while ((input = pa_hashmap_iterate(u->sink->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(input);
        if (input->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        const char *streamType = safeProplistGets(input->proplist, "stream.type", "NULL");
        const char *sessionIDStr = safeProplistGets(input->proplist, "stream.sessionID", "NULL");
        const char *deviceClass = u->primary.sinkAdapter->deviceClass;
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        struct VolumeValues volumes = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float volume = GetCurVolume(sessionID, streamType, deviceClass, &volumes);
        struct VolumeValues *vol = (struct VolumeValues *)pa_hashmap_get(u->streamVolumeMap, &sessionID);
        AUTO_CTRACE("Volume, sessionId: %u, devClass: %s, volume: %.3f,"
            "volumeSystem: %.3f, volumeStream: %.3f, volumeApp: %.3f",
            sessionID, deviceClass, volumes.volume, volumes.volumeSystem, volumes.volumeStream, volumes.volumeApp);
        if (vol) {
            vol->volume = volume;
            vol->volumeHistory = volumes.volumeHistory;
            vol->volumeApp = volumes.volumeApp;
            vol->volumeStream = volumes.volumeStream;
            vol->volumeSystem = volumes.volumeSystem;
        } else {
            uint32_t *key = pa_xnew0(uint32_t, 1);
            *key = sessionID;
            vol = pa_xnew0(struct VolumeValues, 1);
            vol->volume = volume;
            vol->volumeHistory = volumes.volumeHistory;
            vol->volumeApp = volumes.volumeApp;
            vol->volumeStream = volumes.volumeStream;
            vol->volumeSystem = volumes.volumeSystem;
            if (pa_hashmap_put(u->streamVolumeMap, key, vol) != 0) {
                AUDIO_ERR_LOG("pa_hashmap_put failed");
                pa_xfree(key);
                pa_xfree(vol);
            }
        }
    }
}

static struct VolumeValues *GetVolumeFromStreamVolumeMap(struct Userdata *u, uint32_t sessionID)
{
    if (u == NULL || u->streamVolumeMap == NULL) {
        AUDIO_ERR_LOG("streamVolumeMap null");
        return NULL;
    }
    struct VolumeValues *vol = (struct VolumeValues *)pa_hashmap_get(u->streamVolumeMap, &sessionID);
    if (vol) {
        return vol;
    } else {
        AUDIO_ERR_LOG("pa_hashmap_get null");
    }
    return NULL;
}

static void RemoveVolumeFromStreamVolumeMap(struct Userdata *u, pa_sink_input *i)
{
    CHECK_AND_RETURN_LOG(i != NULL, "sink_input is null");
    CHECK_AND_RETURN_LOG(u != NULL && u->streamVolumeMap != NULL, "streamVolumeMap is null");
    const char *sessionIDStr = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
    uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
    int32_t ret = pa_hashmap_remove_and_free(u->streamVolumeMap, &sessionID);
    AUDIO_DEBUG_LOG("RemoveVolumeFromStreamVolumeMap stream volume, sessionId:%{public}u, ret:%{public}d",
        sessionID, ret);
}

static bool IsZeroVolume(float volume)
{
    float d = volume - 0.0f;
    if ((d >= 0 && d <= FLOAT_EPS) || (d <= 0 && d >= -FLOAT_EPS)) {
        return true;
    }
    return false;
}

static void SinkRenderPrimaryProcess(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    if (GetInnerCapturerState()) {
        pa_memchunk capResult;
        SinkRenderCapProcess(si, length, &capResult);
        pa_memblock_unref(capResult.memblock);
    }

    struct Userdata *u = si->userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is NULL");

    ResetBufferAttr(u);
    uint32_t byteSize = (uint32_t)pa_sample_size_of_format(u->format);
    chunkIn->memblock = pa_memblock_new(si->core->mempool, length * IN_CHANNEL_NUM_MAX / DEFAULT_IN_CHANNEL_NUM);
    time_t currentTime = time(NULL);
    g_effectProcessFrameCount++;
    const void *sceneType;
    UpdateSceneToCountMap(u);
    UpdateSceneToResamplerMap(u->sceneToResamplerMap, u->sceneToCountMap, si);
    UpdateStreamVolumeMap(u);
    void *state = NULL;
    while ((pa_hashmap_iterate(u->sceneToCountMap, &state, &sceneType))) {
        u->streamAvailable = 0;
        uint32_t processChannels = DEFAULT_NUM_CHANNEL;
        uint64_t processChannelLayout = DEFAULT_CHANNELLAYOUT;
        EffectChainManagerReturnEffectChannelInfo((char *)sceneType, &processChannels, &processChannelLayout);
        char *sinkSceneType = CheckAndDealEffectZeroVolume(u, currentTime, (char *)sceneType);
        uint32_t frameSize = GetFrameSize(sceneType, length, byteSize, processChannels);
        size_t frameByteSize = frameSize * byteSize;
        chunkIn->index = 0;
        chunkIn->length = frameByteSize;
        int32_t nSinkInput = SinkRenderPrimaryGetData(si, chunkIn, (char *)sceneType);
        if (nSinkInput == 0) { continue; }
        chunkIn->index = 0;
        chunkIn->length = frameByteSize;
        void *src = pa_memblock_acquire_chunk(chunkIn);
        ConvertToFloat(u->format, frameSize, src, u->bufferAttr->tempBufIn);
        int32_t ret = memcpy_s(u->bufferAttr->bufIn, frameSize * sizeof(float), u->bufferAttr->tempBufIn,
            frameSize * sizeof(float));
        CHECK_AND_RETURN_LOG(ret == 0, "SinkRenderPrimaryProcess: copy from bufIn to tempBufIn fail!");
        u->bufferAttr->numChanIn = (int32_t)processChannels;
        u->bufferAttr->frameLen = (int32_t)frameSize / u->bufferAttr->numChanIn;
        size_t outBufferLen = byteSize > 0 ? length / byteSize : 0;
        PrimaryEffectProcess(u, sinkSceneType, sceneType, outBufferLen);
        pa_memblock_release(chunkIn->memblock);
    }
    if (g_effectProcessFrameCount == PRINT_INTERVAL_FRAME_COUNT) { g_effectProcessFrameCount = 0; }
    CheckAndDealSpeakerPaZeroVolume(u, currentTime);
    SinkRenderPrimaryAfterProcess(si, length, chunkIn);
}

static void SinkRenderPrimary(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    pa_sink_ref(si);

    size_t blockSizeMax;

    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    pa_assert(length > 0);
    pa_assert(pa_frame_aligned(length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        chunkIn->memblock = pa_memblock_ref(si->silence.memblock);
        chunkIn->index = si->silence.index;
        chunkIn->length = PA_MIN(si->silence.length, length);
        return;
    }

    if (length == 0)
        length = pa_frame_align(MIX_BUFFER_LENGTH, &si->sample_spec);

    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);
    AUTO_CTRACE("hdi_sink::SinkRenderPrimaryProcess:len:%zu", length);
    SinkRenderPrimaryProcess(si, length, chunkIn);

    pa_sink_unref(si);
}

static void SetSinkVolumeByDeviceClass(pa_sink *s, const char *deviceClass)
{
    CHECK_AND_RETURN_LOG(s != NULL, "s is null");
    void *state = NULL;
    pa_sink_input *input;
    while ((input = pa_hashmap_iterate(s->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(input);
        if (input->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        const char *streamType = safeProplistGets(input->proplist, "stream.type", "NULL");
        const char *sessionIDStr = safeProplistGets(input->proplist, "stream.sessionID", "NULL");
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        struct VolumeValues volumes = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float volumeEnd = GetCurVolume(sessionID, streamType, deviceClass, &volumes);
        float volumeBeg = volumes.volumeHistory;
        if (volumeBeg != volumeEnd) {
            AUDIO_INFO_LOG("sessionID:%{public}u, volumeBeg:%{public}f, volumeEnd:%{public}f",
                sessionID, volumeBeg, volumeEnd);
            SetPreVolume(sessionID, volumeEnd);
            MonitorVolume(sessionID, true);
        }
        uint32_t volume = pa_sw_volume_from_linear(volumeEnd);
        pa_cvolume_set(&input->thread_info.soft_volume, input->thread_info.soft_volume.channels, volume);
    }
}

static void UnsetSinkVolume(pa_sink *s)
{
    CHECK_AND_RETURN_LOG(s != NULL, "s is null");
    void *state = NULL;
    pa_sink_input *input;
    while ((input = pa_hashmap_iterate(s->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(input);
        if (input->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        uint32_t volume = pa_sw_volume_from_linear(1.0f);
        pa_cvolume_set(&input->thread_info.soft_volume, input->thread_info.soft_volume.channels, volume);
    }
}

static void CreateLimiter(struct Userdata *u)
{
    if (!u->isLimiterCreated) {
        int32_t ret = LimiterManagerCreate((int32_t)u->sink->index);
        CHECK_AND_RETURN_LOG(ret == SUCCESS, "limiter manager create failed");
        // allocate limiter buffer; cal algoframelen and latency
        ret = LimiterManagerSetConfig((int32_t)u->sink->index, (int32_t)u->sink->thread_info.max_request,
            (int32_t)pa_sample_size_of_format(u->format), (int32_t)u->ss.rate, (int32_t)u->ss.channels);
        CHECK_AND_RETURN_LOG(ret == SUCCESS, "limiter manager set config failed");
        u->isLimiterCreated = true;
    }
}

static void ProcessRenderUseTiming(struct Userdata *u, pa_usec_t now)
{
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    // Fill the buffer up the latency size
    pa_memchunk chunk;

    AUTO_CTRACE("hdi_sink::SinkRenderPrimary");
    // Change from pa_sink_render to pa_sink_render_full for alignment issue in 3516

    if (IsSinkNameDp(u->sink->name) && u->render_full_enable) {
        // dp update volume
        SetSinkVolumeByDeviceClass(u->sink, u->primary.sinkAdapter->deviceClass);
        pa_sink_render_full(u->sink, u->sink->thread_info.max_request, &chunk); // only work for dp-96k-8ch
        UnsetSinkVolume(u->sink); // reset volume 1.0f
    } else {
        if (u->isEffectBufferAllocated || AllocateEffectBuffer(u)) {
            u->isEffectBufferAllocated = true;
            // limiter process only in normal render
            CreateLimiter(u);
            SinkRenderPrimary(u->sink, u->sink->thread_info.max_request, &chunk);
        }
    }
    pa_assert(chunk.length > 0);

    StartPrimaryHdiIfRunning(u);
    pa_asyncmsgq_post(u->primary.dq, NULL, HDI_RENDER, NULL, 0, &chunk, NULL);
    u->primary.timestamp += pa_bytes_to_usec(chunk.length, &u->sink->sample_spec);
}

static bool InputIsMovie(pa_sink_input *i)
{
    const char *streamType = safeProplistGets(i->proplist, "stream.type", "NULL");
    const bool isMovie = !strcmp(streamType, "movie");
    return isMovie;
}

static bool InputIsOffload(pa_sink_input *i)
{
    if (monitorLinked(i->sink, true)) {
        return false;
    }
    if (strncmp(i->sink->driver, "module_hdi_sink", 15)) { // 15 cmp length
        return false;
    }
    struct Userdata *u = i->sink->userdata;
    if (!u->offload_enable || !u->offload.inited) {
        return false;
    }
    const bool offloadEnable = (GetOffloadEnable(i->index) == 1);
    return offloadEnable;
}

static bool InputIsMultiChannel(pa_sink_input *i)
{
    bool effectOffloadFlag = EffectChainManagerCheckEffectOffload();
    if (effectOffloadFlag) {
        int32_t sinkChannels = i->sample_spec.channels;
        const char *sinkSceneType = pa_proplist_gets(i->proplist, "scene.type");
        const char *sinkSceneMode = pa_proplist_gets(i->proplist, "scene.mode");
        bool existFlag = EffectChainManagerExist(sinkSceneType, sinkSceneMode);
        if (!existFlag && sinkChannels > PRIMARY_CHANNEL_NUM) {
            return true;
        }
    }
    return false;
}

static bool InputIsPrimary(pa_sink_input *i)
{
    const bool isOffload = InputIsOffload(i);
    const bool isRunning = i->thread_info.state == PA_SINK_INPUT_RUNNING;
    return !isOffload && isRunning;
}

static unsigned GetInputsInfo(enum HdiInputType type, bool isRun, pa_sink *s, pa_mix_info *info, unsigned maxinfo)
{
    pa_sink_input *i;
    unsigned n = 0;
    void *state = NULL;

    pa_sink_assert_ref(s);
    pa_sink_assert_io_context(s);
    pa_assert(s);

    while ((i = pa_hashmap_iterate(s->thread_info.inputs, &state, NULL)) && maxinfo > 0) {
        pa_sink_input_assert_ref(i);

        bool flag = false;
        const bool isOffload = InputIsOffload(i);
        const bool isHD = false; // add func is hd
        const bool isRunning = i->thread_info.state == PA_SINK_INPUT_RUNNING;
        if (isRun && !isRunning) {
            continue;
        }
        switch (type) {
            case HDI_INPUT_TYPE_PRIMARY:
                flag = !isOffload && !isHD;
                break;
            case HDI_INPUT_TYPE_OFFLOAD:
                flag = isOffload;
                break;
            case HDI_INPUT_TYPE_MULTICHANNEL:
                flag = isHD;
                break;
            default:
                break;
        }
        if (flag) {
            info->userdata = pa_sink_input_ref(i);
        } else {
            const char *sessionIDStr = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
            AUDIO_PRERELEASE_LOGE("sink: %{public}s, sink_input: %{public}s, the type is not %{public}d",
                s->name, sessionIDStr, type);
            continue;
        }

        info++;
        n++;
        maxinfo--;
    }
    return n;
}

static int32_t GetInputsType(pa_sink *s, unsigned *nPrimary, unsigned *nOffload,
    unsigned *nMultiChannel, bool isRunning)
{
    int ret;
    struct Userdata *u;
    pa_assert_se(u = s->userdata);
    if ((ret = pthread_mutex_lock(&u->mutexPa2)) != 0) {
        AUDIO_WARNING_LOG("GetInputsType pthread_mutex_lock ret %d", ret);
    }
    pa_sink_input *i;
    void *state = NULL;
    *nPrimary = 0;
    *nOffload = 0;
    *nMultiChannel = 0;
    int n = 0;

    pa_sink_assert_ref(s);
    pa_sink_assert_io_context(s);
    pa_assert(s);
    while ((i = pa_hashmap_iterate(s->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(i);
        if (isRunning && i->thread_info.state != PA_SINK_INPUT_RUNNING) {
            continue;
        }
        n++;
        if (InputIsOffload(i)) {
            (*nOffload)++;
        } else if (!strcmp(u->sink->name, MCH_SINK_NAME) && InputIsMultiChannel(i)) {
            (*nMultiChannel)++;
        } else {
            (*nPrimary)++;
        }
    }
    if ((ret = pthread_mutex_unlock(&u->mutexPa2)) != 0) {
        AUDIO_WARNING_LOG("GetInputsType pthread_mutex_unlock ret %d", ret);
    }
    return n;
}

static size_t GetOffloadRenderLength(struct Userdata *u, pa_sink_input *i, bool *wait)
{
    size_t length;
    playback_stream *ps = i->userdata;
    const bool b = (bool)ps->sink_input->thread_info.resampler;
    const pa_sample_spec sampleSpecIn = b ? ps->sink_input->thread_info.resampler->i_ss : ps->sink_input->sample_spec;
    const pa_sample_spec sampleSpecOut = b ? ps->sink_input->thread_info.resampler->o_ss : ps->sink_input->sample_spec;
    size_t sizeFrame = pa_frame_align(pa_usec_to_bytes(OFFLOAD_FRAME_SIZE * PA_USEC_PER_MSEC, &sampleSpecOut),
        &sampleSpecOut);
    size_t tlengthHalfResamp = pa_frame_align(pa_usec_to_bytes(pa_bytes_to_usec(pa_memblockq_get_tlength(
        ps->memblockq) / 1.5, &sampleSpecIn), &sampleSpecOut), &sampleSpecOut); // 1.5 for half
    size_t sizeTgt = PA_MIN(sizeFrame, tlengthHalfResamp);

    const size_t bqlAlin = GetbqlAlinLength(ps, i);

    if (ps->drain_request) {
        if (i->thread_info.render_memblockq->maxrewind != 0) {
            pa_sink_input_update_max_rewind(i, 0);
        }
        const uint64_t hdiPos = u->offload.hdiPos + (pa_rtclock_now() - u->offload.hdiPosTs);
        *wait = u->offload.pos > hdiPos + HDI_MIN_MS_MAINTAIN * PA_USEC_PER_MSEC ? true : false;
        length = u->offload.pos > hdiPos + HDI_MIN_MS_MAINTAIN * PA_USEC_PER_MSEC ? 0 : sizeFrame;
    } else {
        bool waitable = false;
        const uint64_t hdiPos = u->offload.hdiPos + (pa_rtclock_now() - u->offload.hdiPosTs);
        if (u->offload.pos > hdiPos + 60 * PA_USEC_PER_MSEC) { // if hdi cache < 60ms, indicate no enough data
            // hdi left 100ms is triggered process_complete_msg, it leads to kartun. Could be stating time leads it.
            waitable = true;
        }
        length = PA_MIN(bqlAlin, sizeTgt);
        *wait = false;
        if (length < sizeTgt) {
            *wait = waitable || length == 0;
            length = waitable ? 0 : length;
            if (ps->memblockq->missing > 0) {
                playback_stream_request_bytes(ps);
            }
        }
    }
    return length;
}

static void InputsDropFromInputs2(pa_mix_info *info, unsigned n)
{
    for (; n > 0; info++, n--) {
        if (info->userdata) {
            pa_sink_input_unref(info->userdata);
            info->userdata = NULL;
        }
        if (info->chunk.memblock) {
            pa_memblock_unref(info->chunk.memblock);
        }
    }
}

static void InputsDropFromInputs(pa_mix_info *infoInputs, unsigned nInputs, pa_mix_info *info, unsigned n,
    pa_memchunk *result)
{
    unsigned p = 0;
    unsigned ii = 0;
    unsigned nUnreffed = 0;
    pa_assert(result && result->memblock && result->length > 0);
    /* We optimize for the case where the order of the inputs has not changed */

    for (ii = 0; ii < nInputs; ++ii) {
        pa_sink_input *i = infoInputs[ii].userdata;
        unsigned j;
        pa_mix_info *m = NULL;

        pa_sink_input_assert_ref(i);

        /* Let's try to find the matching entyr info the pa_mix_info array */
        for (j = 0; j < n; j++) {
            if (info[p].userdata == i) {
                m = info + p;
                break;
            }

            p++;
            if (p >= n) {
                p = 0;
            }
        }

        /* Drop read data */
        pa_sink_input_drop(i, result->length);

        if (m) {
            if (m->chunk.memblock) {
                pa_memblock_unref(m->chunk.memblock);
                pa_memchunk_reset(&m->chunk);
            }

            pa_sink_input_unref(m->userdata);
            m->userdata = NULL;

            nUnreffed += 1;
        }
    }

    /* Now drop references to entries that are included in the
     * pa_mix_info array but don't exist anymore */

    if (nUnreffed < n) {
        for (; n > 0; info++, n--) {
            if (info->userdata)
                pa_sink_input_unref(info->userdata);
            if (info->chunk.memblock) {
                pa_memblock_unref(info->chunk.memblock);
            }
        }
    }
}

static void PaSinkRenderIntoOffload(pa_sink *s, pa_mix_info *infoInputs, unsigned nInputs, pa_memchunk *target)
{
    unsigned n = 0;
    unsigned ii = 0;
    pa_mix_info info[MAX_MIX_CHANNELS];
    pa_sink_assert_ref(s);
    pa_sink_assert_io_context(s);

    size_t length = target->length;
    size_t mixlength = length;
    size_t blockSizeMax = pa_mempool_block_size_max(s->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &s->sample_spec);

    pa_assert(length > 0);

    for (ii = 0; ii < nInputs; ++ii) {
        pa_sink_input *i = infoInputs[ii].userdata;
        pa_sink_input_assert_ref(i);
        AUTO_CTRACE("hdi_sink::Offload:pa_sink_input_peek:%u len:%zu", i->index, length);
        pa_sink_input_peek(i, length, &info[n].chunk, &info[n].volume);
        if (mixlength == 0 || info[n].chunk.length < mixlength)
            mixlength = info[n].chunk.length;

        if (pa_memblock_is_silence(info[n].chunk.memblock)) {
            AUTO_CTRACE("hdi_sink::Offload::is_silence");
            pa_memblock_unref(info[n].chunk.memblock);
            continue;
        }

        info[n].userdata = pa_sink_input_ref(i);

        pa_assert(info[n].chunk.memblock);
        pa_assert(info[n].chunk.length > 0);

        n++;
    }
    length = mixlength > 0 ? mixlength : length;

    pa_assert(n == 1 || n == 0);
    if (n == 0) {
        if (target->length > length)
            target->length = length;

        pa_silence_memchunk(target, &s->sample_spec);
    } else if (n == 1) {
        if (target->length > length)
            target->length = length;

        pa_memchunk vchunk;
        vchunk = info[0].chunk;

        if (vchunk.length > length)
            vchunk.length = length;
        // if target lead pa_memblock_new memory leak, fixed chunk length can solve it.
        pa_memchunk_memcpy(target, &vchunk);
    }

    InputsDropFromInputs(infoInputs, nInputs, info, n, target);
}

static void OffloadReset(struct Userdata *u)
{
    u->offload.sessionID = -1;
    u->offload.pos = 0;
    u->offload.hdiPos = 0;
    u->offload.hdiPosTs = pa_rtclock_now();
    u->offload.firstWriteHdi = true;
    u->offload.setHdiBufferSizeNum = OFFLOAD_SET_BUFFER_SIZE_NUM;
    pa_atomic_store(&u->offload.hdistate, 0);
    u->offload.fullTs = 0;
}

static int32_t RenderWriteOffloadFunc(struct Userdata *u, size_t length, pa_mix_info *infoInputs, unsigned nInputs)
{
    pa_sink_input *i = infoInputs[0].userdata;

    pa_assert(length != 0);
    pa_memchunk *chunk = &(u->offload.chunk);
    chunk->index = 0;
    chunk->length = length;
    int64_t l;
    int64_t d;
    l = (int64_t)chunk->length;
    size_t blockSize = pa_memblock_get_length(u->offload.chunk.memblock);
    blockSize = PA_MAX(blockSize, pa_usec_to_bytes(0.6 * OFFLOAD_HDI_CACHE1 * PA_USEC_PER_MSEC, // 0.6 40% is hdi limit
        &u->sink->sample_spec));
    d = 0;
    while (l > 0) {
        pa_memchunk tchunk;
        tchunk = *chunk;
        tchunk.index += (size_t)d;
        tchunk.length = (size_t)l;

        PaSinkRenderIntoOffload(i->sink, infoInputs, nInputs, &tchunk);
        d += (int64_t)tchunk.length;
        l -= (int64_t)tchunk.length;
    }
    if (l < 0) {
        chunk->length += (size_t)-l;
    }

    int32_t appsUid[MAX_MIX_CHANNELS];
    size_t count = 0;

    const char *cstringClientUid = pa_proplist_gets(i->proplist, "stream.client.uid");
    if (cstringClientUid && (i->thread_info.state == PA_SINK_INPUT_RUNNING)) {
        appsUid[count++] = atoi(cstringClientUid);
    }

    SafeRendererSinkUpdateAppsUid(u->offload.sinkAdapter, appsUid, count);

    int ret = RenderWriteOffload(u, i, chunk);
    int32_t writen = ret == 0 ? (int32_t)chunk->length : 0;
    if (ret == OFFLOAD_HDI_FULL) { // 1 indicates full
        const int hdistate = pa_atomic_load(&u->offload.hdistate);
        if (hdistate == 0) {
            pa_atomic_store(&u->offload.hdistate, 1);
        }
        if (GetInputPolicyState(i) == OFFLOAD_INACTIVE_BACKGROUND || InputIsMovie(i)) {
            u->offload.fullTs = pa_rtclock_now();
        }
        pa_memblockq_rewind(i->thread_info.render_memblockq, chunk->length);
    } else if (ret < 0) {
        pa_memblockq_rewind(i->thread_info.render_memblockq, chunk->length);
    }

    u->offload.pos += pa_bytes_to_usec(writen, &u->sink->sample_spec);
    InputsDropFromInputs2(infoInputs, nInputs);
    return ret;
}

static int32_t ProcessRenderUseTimingOffload(struct Userdata *u, bool *wait, int32_t *nInput)
{
    *wait = true;
    pa_sink *s = u->sink;
    pa_mix_info infoInputs[MAX_MIX_CHANNELS];
    unsigned nInputs;

    pa_sink_assert_ref(s);
    pa_sink_assert_io_context(s);
    pa_assert(PA_SINK_IS_LINKED(s->thread_info.state));

    pa_assert(s->thread_info.rewind_nbytes == 0);

    if (s->thread_info.state == PA_SINK_SUSPENDED) {
        return 0;
    }

    pa_sink_ref(s);

    nInputs = GetInputsInfo(HDI_INPUT_TYPE_OFFLOAD, true, s, infoInputs, MAX_MIX_CHANNELS);
    *nInput = (int32_t)nInputs;

    if (nInputs == 0) {
        pa_sink_unref(s);
        return 0;
    } else if (nInputs > 1) {
        AUDIO_WARNING_LOG("GetInputsInfo offload input != 1");
    }

    pa_sink_input *i = infoInputs[0].userdata;
    if (GetFadeoutState(i->index) != NO_FADE) {
        InputsDropFromInputs2(infoInputs, nInputs);
        pa_sink_unref(s);
        AUDIO_WARNING_LOG("stream is croked, do not need peek");
        return 0;
    }
    CheckInputChangeToOffload(u, i);
    size_t length = GetOffloadRenderLength(u, i, wait);
    if (*wait && length == 0) {
        InputsDropFromInputs2(infoInputs, nInputs);
        AUTO_CTRACE("hdi_sink::ProcessRenderUseTimingOffload::underrun");
        pa_sink_input_handle_ohos_underrun(i);
        pa_sink_unref(s);
        return 0;
    }
    int ret = RenderWriteOffloadFunc(u, length, infoInputs, nInputs);
    pa_sink_unref(s);
    return ret;
}

static int32_t UpdatePresentationPosition(struct Userdata *u)
{
    uint64_t frames;
    int64_t timeSec;
    int64_t timeNanoSec;
    int ret = u->offload.sinkAdapter->SinkAdapterGetPresentationPosition(
        u->offload.sinkAdapter, &frames, &timeSec, &timeNanoSec);
    if (ret != 0) {
        AUDIO_ERR_LOG("RendererSinkGetPresentationPosition fail, ret %d", ret);
        return ret;
    }
    u->offload.hdiPos = frames;
    u->offload.hdiPosTs = (uint64_t)timeSec * USEC_PER_SEC + (uint64_t)timeNanoSec / PA_NSEC_PER_USEC;
    return 0;
}

uint64_t CalcOffloadCacheLenInHdi(struct Userdata *u)
{
    uint64_t pos = u->offload.pos;
    pa_usec_t now = pa_rtclock_now();
    uint64_t time = now > u->offload.hdiPosTs ? (now - u->offload.hdiPosTs) / PA_USEC_PER_MSEC : 0;
    uint64_t hdiPos = u->offload.hdiPos + time * PA_USEC_PER_MSEC;
    uint64_t cacheLenInHdi = pos > hdiPos ? (pos - hdiPos) : 0;
    AUDIO_DEBUG_LOG("offload latency: %{public}" PRIu64 " write pos: %{public}" PRIu64
                    " hdi pos: %{public}" PRIu64 " time: %{public}" PRIu64,
                    cacheLenInHdi, pos, u->offload.hdiPos, time * PA_USEC_PER_MSEC);
    return cacheLenInHdi;
}

static void OffloadRewindAndFlush(struct Userdata *u, pa_sink_input *i, bool afterRender)
{
    CHECK_AND_RETURN_LOG(i != NULL, "i is null");
    pa_sink_input_assert_ref(i);
    playback_stream *ps = i->userdata;
    CHECK_AND_RETURN_LOG(ps != NULL, "ps is null");

    OffloadLock(u); // flush will interrupt the offload callback, may be offload unlock.
    int32_t ret = u->offload.sinkAdapter->SinkAdapterFlush(u->offload.sinkAdapter);
    if (ret == 0) {
        uint64_t offloadFade = 180000; // 180000 us fade out
        uint64_t cacheLenInHdi = CalcOffloadCacheLenInHdi(u);
        cacheLenInHdi = cacheLenInHdi > offloadFade ? cacheLenInHdi - offloadFade : 0;
        uint64_t bufSizeInRender = pa_usec_to_bytes(cacheLenInHdi, &i->sink->sample_spec);
        const pa_sample_spec sampleSpecIn = i->thread_info.resampler ? i->thread_info.resampler->i_ss
                                                                        : i->sample_spec;
        uint64_t bufSizeInInput = pa_usec_to_bytes(cacheLenInHdi, &sampleSpecIn);
        bufSizeInInput += pa_usec_to_bytes(pa_bytes_to_usec(
            pa_memblockq_get_length(i->thread_info.render_memblockq), &i->sink->sample_spec), &sampleSpecIn);
        uint64_t rewindSize = afterRender ? bufSizeInRender : bufSizeInInput;
        uint64_t maxRewind = afterRender ? i->thread_info.render_memblockq->maxrewind : ps->memblockq->maxrewind;
        if (rewindSize > maxRewind) {
            AUDIO_WARNING_LOG("OffloadRewindAndFlush, rewindSize(%" PRIu64 ") > maxrewind(%u), afterRender(%d)",
                rewindSize, (uint32_t)(afterRender ? i->thread_info.render_memblockq->maxrewind :
                                        ps->memblockq->maxrewind), afterRender);
            rewindSize = maxRewind;
        }

        AUDIO_DEBUG_LOG("OffloadRewindAndFlush rewind data length %{public}" PRIu64, rewindSize);
        if (afterRender) {
            pa_memblockq_rewind(i->thread_info.render_memblockq, rewindSize);
        } else {
            pa_memblockq_rewind(ps->memblockq, rewindSize);
            pa_memblockq_flush_read(i->thread_info.render_memblockq);
        }
    }
    OffloadReset(u);
}

static void GetSinkInputName(pa_sink_input *i, char *str, int len)
{
    const char *streamUid = safeProplistGets(i->proplist, "stream.client.uid", "NULL");
    const char *streamPid = safeProplistGets(i->proplist, "stream.client.pid", "NULL");
    const char *streamType = safeProplistGets(i->proplist, "stream.type", "NULL");
    const char *sessionID = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
    int ret = sprintf_s(str, len, "%s_%s_%s_%s_of%d", streamType, streamUid, streamPid, sessionID, InputIsOffload(i));
    if (ret < 0) {
        AUDIO_ERR_LOG("sprintf_s fail! ret %d", ret);
    }
}

static int32_t getSinkInputSessionID(pa_sink_input *i)
{
    const char *res = pa_proplist_gets(i->proplist, "stream.sessionID");
    if (res == NULL) {
        return -1;
    } else {
        return atoi(res);
    }
}

static uint32_t getSinkInputUid(pa_sink_input *i)
{
    const char *res = pa_proplist_gets(i->proplist, "stream.client.uid");
    if (res == NULL) {
        return 0;
    } else {
        return atoi(res);
    }
}

static void OffloadLock(struct Userdata *u)
{
    if (!u->offload.runninglocked) {
        u->offload.sinkAdapter->SinkAdapterLockOffloadRunningLock(u->offload.sinkAdapter);
        u->offload.runninglocked = true;
    } else {
    }
}

static void OffloadUnlock(struct Userdata *u)
{
    if (u->offload.runninglocked) {
        u->offload.sinkAdapter->SinkAdapterUnLockOffloadRunningLock(u->offload.sinkAdapter);
        u->offload.runninglocked = false;
    } else {
    }
}

static void offloadSetMaxRewind(struct Userdata *u, pa_sink_input *i)
{
    pa_sink_input_assert_ref(i);
    pa_sink_input_assert_io_context(i);
    pa_assert(PA_SINK_INPUT_IS_LINKED(i->thread_info.state));

    size_t blockSize = pa_memblock_get_length(u->offload.chunk.memblock);
    pa_memblockq_set_maxrewind(i->thread_info.render_memblockq, blockSize);

    size_t nbytes = pa_usec_to_bytes(MAX_REWIND, &i->sink->sample_spec);

    if (i->update_max_rewind) {
        i->update_max_rewind(i, i->thread_info.resampler ?
                                pa_resampler_request(i->thread_info.resampler, nbytes) : nbytes);
    }
}

static void CheckInputChangeToOffload(struct Userdata *u, pa_sink_input *i)
{
    // if maxrewind is set to buffer_size while inner-cap, reset it to 0 for offload.
    if (pa_memblockq_get_maxrewind(i->thread_info.render_memblockq) == u->buffer_size) {
        AUDIO_INFO_LOG("Reset maxwind to 0 to enable offload for sink-input:%{public}u", i->index);
        pa_sink_input_update_max_rewind(i, 0);
    }
    if (InputIsOffload(i) && pa_memblockq_get_maxrewind(i->thread_info.render_memblockq) == 0) {
        offloadSetMaxRewind(u, i);
        pa_sink_input *it;
        void *state = NULL;
        while ((it = pa_hashmap_iterate(u->sink->thread_info.inputs, &state, NULL))) {
            if (it != i && pa_memblockq_get_maxrewind(it->thread_info.render_memblockq) != 0) {
                pa_sink_input_update_max_rewind(it, 0);
                drop_backlog(it->thread_info.render_memblockq);
                playback_stream *ps = it->userdata;
                drop_backlog(ps->memblockq);
            }
        }
    }
}

static void StartOffloadHdi(struct Userdata *u, pa_sink_input *i)
{
    CheckInputChangeToOffload(u, i);
    int32_t sessionID = getSinkInputSessionID(i);
    if (u->offload.isHDISinkStarted) {
        AUDIO_INFO_LOG("StartOffloadHdi, sessionID : %{public}d -> %{public}d", u->offload.sessionID, sessionID);
        if (sessionID != u->offload.sessionID) {
            if (u->offload.sessionID != -1) {
                u->offload.sinkAdapter->SinkAdapterReset(u->offload.sinkAdapter);
                OffloadReset(u);
            }
            u->offload.sessionID = sessionID;
        }
    } else {
        AUDIO_INFO_LOG("StartOffloadHdi, Restart offload with rate:%{public}d, channels:%{public}d",
            u->ss.rate, u->ss.channels);
        if (u->offload.sinkAdapter->SinkAdapterStart(u->offload.sinkAdapter)) {
            AUDIO_WARNING_LOG("StartOffloadHdi, audiorenderer control start failed!");
        } else {
            RegOffloadCallback(u);
            u->offload.isHDISinkStarted = true;
            AUDIO_INFO_LOG("StartOffloadHdi, Successfully restarted offload HDI renderer");
            OffloadLock(u);
            u->offload.sessionID = sessionID;
            OffloadSetHdiVolume(i);
        }
    }
}

static void PaInputStateChangeCbOffload(struct Userdata *u, pa_sink_input *i, pa_sink_input_state_t state)
{
    const bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    const bool starting = i->thread_info.state == PA_SINK_INPUT_CORKED && state == PA_SINK_INPUT_RUNNING;
    const bool stopping = state == PA_SINK_INPUT_UNLINKED;

    if (!u->offload.inited && PrepareDeviceOffload(u) == 0) {
        u->offload.inited = true;
    }
    if (starting) {
        StartOffloadHdi(u, i);
    } else if (corking) {
        pa_atomic_store(&u->offload.hdistate, 2); // 2 indicates corking
        OffloadRewindAndFlush(u, i, false);
    } else if (stopping) {
        u->offload.sinkAdapter->SinkAdapterFlush(u->offload.sinkAdapter);
        OffloadReset(u);
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }
    ResetVolumeBySinkInputState(i, state);
}

static void ResetVolumeBySinkInputState(pa_sink_input *i, pa_sink_input_state_t state)
{
    CHECK_AND_RETURN_LOG(i != NULL, "i is null");
    const bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    if (corking) {
        const char *sessionIDStr = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        SetPreVolume(sessionID, 0.0f);
    }
}

// call from IO thread(OS_ProcessData)
static void PaInputStateChangeCbPrimary(struct Userdata *u, pa_sink_input *i, pa_sink_input_state_t state)
{
    const bool starting = i->thread_info.state == PA_SINK_INPUT_CORKED && state == PA_SINK_INPUT_RUNNING;
    const bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    uint32_t streamIndex = i->index;
    if (corking) {
        SetFadeoutState(streamIndex, NO_FADE);
    }

    if (starting) {
        u->primary.timestamp = pa_rtclock_now();
        if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
            pa_atomic_store(&u->primary.fadingFlagForPrimary, 1);
            AUDIO_INFO_LOG("store fadingFlagForPrimary for 1");
            SetFadeoutState(streamIndex, NO_FADE);
            u->primary.primaryFadingInDone = 0;
            u->primary.primarySinkInIndex = (int32_t)(i->index);
            AUDIO_INFO_LOG("PaInputStateChangeCb, HDI renderer already started");
            return;
        }
        AUDIO_INFO_LOG("PaInputStateChangeCb, Restart with rate:%{public}d,channels:%{public}d, format:%{public}d",
            u->ss.rate, u->ss.channels, (int)pa_sample_size_of_format(u->format));
        if (pa_asyncmsgq_send(u->primary.dq, NULL, HDI_START, NULL, 0, NULL)) {
            AUDIO_ERR_LOG("audiorenderer control start failed!");
            u->primary.sinkAdapter->SinkAdapterDeInit(u->primary.sinkAdapter);
        } else {
            pa_atomic_store(&u->primary.isHDISinkStarted, 1);
            u->writeCount = 0;
            u->renderCount = 0;
            pa_atomic_store(&u->primary.fadingFlagForPrimary, 1);
            AUDIO_INFO_LOG("store fadingFlagForPrimary for 1");
            SetFadeoutState(streamIndex, NO_FADE);
            u->primary.primaryFadingInDone = 0;
            u->primary.primarySinkInIndex = (int32_t)(i->index);
            AUDIO_INFO_LOG("PaInputStateChangeCb, Successfully restarted HDI renderer");
        }
    }
    const char *strExpectedPlaybackDurationBytes = safeProplistGets(i->proplist, "expectedPlaybackDurationBytes", "0");
    uint64_t expectedPlaybackDurationBytes = 0;
    pa_atou64(strExpectedPlaybackDurationBytes, &expectedPlaybackDurationBytes);
    enum FadeStrategy fadeStrategy
        = GetFadeStrategy(pa_bytes_to_usec(expectedPlaybackDurationBytes, &(i->sample_spec)) / PA_USEC_PER_MSEC);
    if (fadeStrategy == FADE_STRATEGY_DEFAULT) {
        ResetVolumeBySinkInputState(i, state);
    }
}

// call from IO thread(OS_ProcessData)
static void StartPrimaryHdiIfRunning(struct Userdata *u)
{
    AUTO_CTRACE("hdi_sink::StartPrimaryHdiIfRunning");
    if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
        return;
    }

    unsigned nPrimary = 0;
    unsigned nOffload = 0;
    unsigned nMultiChannel = 0;
    GetInputsType(u->sink, &nPrimary, &nOffload, &nMultiChannel, true);
    if (nPrimary == 0) {
        return;
    }

    if (pa_asyncmsgq_send(u->primary.dq, NULL, HDI_START, NULL, 0, NULL)) {
        AUDIO_ERR_LOG("audiorenderer control start failed!");
        u->primary.sinkAdapter->SinkAdapterDeInit(u->primary.sinkAdapter);
    } else {
        pa_atomic_store(&u->primary.isHDISinkStarted, 1);
        u->writeCount = 0;
        u->renderCount = 0;
        AUDIO_INFO_LOG("StartPrimaryHdiIfRunning, Successfully restarted HDI renderer");
    }
}

static void ResetMultiChannelHdiState(struct Userdata *u)
{
    if (u->multiChannel.sinkAdapter == NULL) {
        return;
    }
    if (u->multiChannel.isHDISinkInited) {
        if (u->multiChannel.sample_attrs.channel != (uint32_t)u->multiChannel.sinkChannel) {
            u->multiChannel.sinkAdapter->SinkAdapterStop(u->multiChannel.sinkAdapter);
            u->multiChannel.isHDISinkStarted = false;
            u->multiChannel.sinkAdapter->SinkAdapterDeInit(u->multiChannel.sinkAdapter);
            u->multiChannel.isHDISinkInited = false;
            u->multiChannel.sample_attrs.adapterName = u->defaultAdapterEnable ? "dp" : "primary";
            u->multiChannel.sample_attrs.channel = (uint32_t)u->multiChannel.sinkChannel;
            u->multiChannel.sample_attrs.channelLayout = u->multiChannel.sinkChannelLayout;
            u->multiChannel.sinkAdapter->SinkAdapterInit(u->multiChannel.sinkAdapter, &u->multiChannel.sample_attrs);
            u->multiChannel.isHDISinkInited = true;
        } else {
            if (u->multiChannel.isHDISinkStarted) {
                u->multiChannel.multiChannelSinkInIndex = u->multiChannel.multiChannelTmpSinkInIndex;
                return;
            }
        }
    } else {
        u->multiChannel.sample_attrs.adapterName = u->defaultAdapterEnable ? "dp" : "primary";
        u->multiChannel.sample_attrs.channel = (uint32_t)u->multiChannel.sinkChannel;
        u->multiChannel.sample_attrs.channelLayout = u->multiChannel.sinkChannelLayout;
        u->multiChannel.sinkAdapter->SinkAdapterInit(u->multiChannel.sinkAdapter, &u->multiChannel.sample_attrs);
        u->multiChannel.isHDISinkInited = true;
    }
    if (u->multiChannel.sinkAdapter->SinkAdapterStart(u->multiChannel.sinkAdapter)) {
        u->multiChannel.isHDISinkStarted = false;
        u->multiChannel.sinkAdapter->SinkAdapterDeInit(u->multiChannel.sinkAdapter);
        u->multiChannel.isHDISinkInited = false;
        AUDIO_INFO_LOG("ResetMultiChannelHdiState deinit success");
    } else {
        u->multiChannel.isHDISinkStarted = true;
        AUDIO_INFO_LOG("ResetMultiChannelHdiState start success");
        u->writeCount = 0;
        u->renderCount = 0;
        pa_atomic_store(&u->multiChannel.fadingFlagForMultiChannel, 1);
        u->multiChannel.multiChannelFadingInDone = 0;
        u->multiChannel.multiChannelSinkInIndex = u->multiChannel.multiChannelTmpSinkInIndex;
    }
}

static void StartMultiChannelHdiIfRunning(struct Userdata *u)
{
    ResetMultiChannelHdiState(u);
}

static void PaInputStateChangeCbMultiChannel(struct Userdata *u, pa_sink_input *i, pa_sink_input_state_t state)
{
    const bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    const bool starting = i->thread_info.state == PA_SINK_INPUT_CORKED && state == PA_SINK_INPUT_RUNNING;
    const bool stopping = state == PA_SINK_INPUT_UNLINKED;

    if (corking) {
        SetFadeoutState(i->index, NO_FADE);
    }
    if (starting) {
        u->multiChannel.timestamp = pa_rtclock_now();
        u->multiChannel.multiChannelTmpSinkInIndex = (int32_t)(i->index);
    } else if (stopping) {
        // Continuously dropping data clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("PaInputStateChangeCbMultiChannel, HDI-sink continuously dropping data - clear statistics "
                           "(%zu -> 0 bytes dropped)", u->bytes_dropped);
            u->bytes_dropped = 0;
        }
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }
    ResetVolumeBySinkInputState(i, state);
}

static void ResetFadeoutPause(pa_sink_input *i, pa_sink_input_state_t state)
{
    bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    bool starting = i->thread_info.state == PA_SINK_INPUT_CORKED && state == PA_SINK_INPUT_RUNNING;
    if (corking || starting) {
        AUDIO_INFO_LOG("set fadeoutPause to 0");
        SetFadeoutState(i->index, NO_FADE);
    }
}

static void RendererSinkSetPriPaPower(pa_sink_input *i, pa_sink_input_state_t state, struct Userdata *u)
{
    if (state == PA_SINK_INPUT_RUNNING) {
        if (u->primary.sinkAdapter == NULL) {
            return;
        }
        const char *streamType = safeProplistGets(i->proplist, "stream.type", "NULL");
        const char *sessionIDStr = safeProplistGets(i->proplist, "stream.sessionID", "NULL");
        const char *deviceClass = u->primary.sinkAdapter->deviceClass;
        uint32_t sessionID = sessionIDStr != NULL ? (uint32_t)atoi(sessionIDStr) : 0;
        struct VolumeValues volumes = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        float volume = GetCurVolume(sessionID, streamType, deviceClass, &volumes);
        bool isZeroVolume = IsZeroVolume(volume);
        AUDIO_INFO_LOG(
            "session %{public}u, stream %{public}s, zerovol %{public}d", sessionID, streamType, isZeroVolume);
        if (!isZeroVolume) {
            u->primary.sinkAdapter->SinkAdapterSetPriPaPower(u->primary.sinkAdapter);
        }
    }
}

static void PaInputStateChangeCb(pa_sink_input *i, pa_sink_input_state_t state)
{
    struct Userdata *u = NULL;

    CHECK_AND_RETURN_LOG(i != NULL, "i is null");
    pa_sink_input_assert_ref(i);
    CHECK_AND_RETURN_LOG(i->sink != NULL, "i->sink is null");

    const bool corking = i->thread_info.state == PA_SINK_INPUT_RUNNING && state == PA_SINK_INPUT_CORKED;
    const bool starting = i->thread_info.state == PA_SINK_INPUT_CORKED && state == PA_SINK_INPUT_RUNNING;
    const bool stopping = state == PA_SINK_INPUT_UNLINKED;

    corking ? pa_atomic_store(&i->isFirstReaded, 0) : (void)0;
    starting ? pa_atomic_store(&i->isFirstReaded, 1) : (void)0;

    if (IsInnerCapSinkName(i->sink->name) ||
        !strcmp(i->sink->name, SINK_NAME_REMOTE_CAST_INNER_CAPTURER) ||
        !strcmp(i->sink->driver, "module_split_stream_sink.c")) {
        ResetFadeoutPause(i, state);
        AUDIO_INFO_LOG("PaInputStateChangeCb inner_cap return");
        return;
    }
    pa_assert_se(u = i->sink->userdata);

    RendererSinkSetPriPaPower(i, state, u);

    char str[SPRINTF_STR_LEN] = {0};
    GetSinkInputName(i, str, SPRINTF_STR_LEN);
    AUDIO_INFO_LOG(
        "PaInputStateChangeCb, Sink[%{public}s]->SinkInput[%{public}s] state change:[%{public}s]-->[%{public}s]",
        u->primary.sinkAdapter->deviceClass, str, GetInputStateInfo(i->thread_info.state),
        GetInputStateInfo(state));

    if (i->thread_info.state == state) {
        return;
    }

    if (!corking && !starting && !stopping) {
        AUDIO_WARNING_LOG("PaInputStateChangeCb, input state change: invalid");
        return;
    }

    if (u->offload_enable && !strcmp(i->sink->name, OFFLOAD_SINK_NAME)) {
        ResetFadeoutPause(i, state);
        PaInputStateChangeCbOffload(u, i, state);
    } else if (u->multichannel_enable && !strcmp(i->sink->name, MCH_SINK_NAME)) {
        PaInputStateChangeCbMultiChannel(u, i, state);
        stopping ? RemoveVolumeFromStreamVolumeMap(u, i) : (void)0;
    } else {
        PaInputStateChangeCbPrimary(u, i, state);
        stopping ? RemoveVolumeFromStreamVolumeMap(u, i) : (void)0;
    }
}

static void ThreadFuncRendererTimerOffloadProcess(struct Userdata *u, pa_usec_t now, int64_t *sleepForUsec)
{
    static uint32_t timeWait = 1; // 1ms init
    const uint64_t pos = u->offload.pos;
    const uint64_t hdiPos = u->offload.hdiPos + (pa_rtclock_now() - u->offload.hdiPosTs);
    int64_t blockTime = (int64_t)pa_bytes_to_usec(u->sink->thread_info.max_request, &u->sink->sample_spec);

    int32_t nInput = -1;
    const int hdistate = (int)pa_atomic_load(&u->offload.hdistate);
    if (hdistate == 0) {
        bool wait;
        int ret = ProcessRenderUseTimingOffload(u, &wait, &nInput);
        if (ret < 0) {
            blockTime = 20 * PA_USEC_PER_MSEC; // 20ms for render write error
        } else if (wait) {
            blockTime = (int64_t)(timeWait * PA_USEC_PER_MSEC); // timeWait ms for first write no data
            if (timeWait < 20) { // 20ms max wait no data
                timeWait++;
            }
        } else {
            timeWait = 1; // 1ms have data reset timeWait
            blockTime = 1 * PA_USEC_PER_MSEC; // 1ms for min wait
        }
    } else if (hdistate == 1) {
        blockTime = (int64_t)(pos - hdiPos - HDI_MIN_MS_MAINTAIN * PA_USEC_PER_MSEC);
        if (blockTime < 0) {
            blockTime = OFFLOAD_FRAME_SIZE * PA_USEC_PER_MSEC; // block for one frame
        }
    }
    if (pos < hdiPos) {
        if (pos != 0) {
            AUDIO_DEBUG_LOG("ThreadFuncRendererTimerOffload hdiPos wrong need sync, pos %" PRIu64 ", hdiPos %" PRIu64,
                pos, hdiPos);
        }
        if (u->offload.hdiPosTs + 300 * PA_USEC_PER_MSEC < now) { // 300ms for update pos
            UpdatePresentationPosition(u);
        }
    }
    if (blockTime != -1) {
        *sleepForUsec = PA_MAX(blockTime, 0) - (int64_t)(pa_rtclock_now() - now);
        *sleepForUsec = PA_MAX(*sleepForUsec, 0);
    }
}

static void ThreadFuncRendererTimerOffloadFlag(struct Userdata *u, pa_usec_t now, bool *flagOut, int64_t *sleepForUsec)
{
    bool flag = PA_SINK_IS_RUNNING(u->sink->thread_info.state);
    if (!flag && !PA_SINK_IS_OPENED(u->sink->thread_info.state)) {
        OffloadUnlock(u);
    }
    *flagOut = flag;
}

static void SinkRenderMultiChannelProcess(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_assert_io_context(si);
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");

    struct Userdata *u;
    pa_assert_se(u = si->userdata);

    UpdateStreamVolumeMap(u);

    chunkIn->memblock = pa_memblock_new(si->core->mempool, length * IN_CHANNEL_NUM_MAX / DEFAULT_IN_CHANNEL_NUM);
    size_t tmpLength = length * u->multiChannel.sinkChannel / DEFAULT_IN_CHANNEL_NUM;
    chunkIn->index = 0;
    chunkIn->length = tmpLength;
    SinkRenderMultiChannelGetData(si, chunkIn);
    chunkIn->index = 0;
    chunkIn->length = tmpLength;
}

static void SinkRenderMultiChannel(pa_sink *si, size_t length, pa_memchunk *chunkIn)
{
    size_t blockSizeMax;

    CHECK_AND_RETURN_LOG(si != NULL, "si is null");
    pa_sink_ref(si);
    pa_sink_assert_ref(si);
    pa_sink_assert_io_context(si);
    pa_assert(PA_SINK_IS_LINKED(si->thread_info.state));
    CHECK_AND_RETURN_LOG(chunkIn != NULL, "chunkIn is null");
    CHECK_AND_RETURN_LOG(length > 0, "length <= 0");
    pa_assert(pa_frame_aligned(length, &si->sample_spec));

    pa_assert(!si->thread_info.rewind_requested);
    pa_assert(si->thread_info.rewind_nbytes == 0);

    if (si->thread_info.state == PA_SINK_SUSPENDED) {
        chunkIn->memblock = pa_memblock_ref(si->silence.memblock);
        chunkIn->index = si->silence.index;
        chunkIn->length = PA_MIN(si->silence.length, length);
        return;
    }

    if (length == 0)
        length = pa_frame_align(MIX_BUFFER_LENGTH, &si->sample_spec);

    blockSizeMax = pa_mempool_block_size_max(si->core->mempool);
    if (length > blockSizeMax)
        length = pa_frame_align(blockSizeMax, &si->sample_spec);

    pa_assert(length > 0);

    SinkRenderMultiChannelProcess(si, length, chunkIn);

    pa_sink_unref(si);
}

static void ProcessRenderUseTimingMultiChannel(struct Userdata *u, pa_usec_t now)
{
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    // Fill the buffer up the latency size
    pa_memchunk chunk;
    chunk.memblock = NULL;
    chunk.length = 0;
    chunk.index = 0;

    // Change from pa_sink_render to pa_sink_render_full for alignment issue in 3516
    SinkRenderMultiChannel(u->sink, u->sink->thread_info.max_request, &chunk);
    pa_assert(chunk.length > 0);

    StartMultiChannelHdiIfRunning(u);

    if (!chunk.memblock) {
        if (pa_atomic_load(&u->multiChannel.dflag) == 1) {
            pa_atomic_sub(&u->multiChannel.dflag, 1);
        }
        return;
    }
    pa_asyncmsgq_post(u->multiChannel.dq, NULL, HDI_RENDER, NULL, 0, &chunk, NULL);
    u->multiChannel.timestamp += pa_bytes_to_usec(u->sink->thread_info.max_request, &u->sink->sample_spec);
}

static bool POSSIBLY_UNUSED ThreadFuncRendererTimerMultiChannelFlagJudge(struct Userdata *u)
{
    CHECK_AND_RETURN_RET_LOG(u != NULL, false, "u is null");
    bool flag = (u->render_in_idle_state && PA_SINK_IS_OPENED(u->sink->thread_info.state)) ||
        (!u->render_in_idle_state && PA_SINK_IS_RUNNING(u->sink->thread_info.state)) ||
        (u->sink->thread_info.state == PA_SINK_IDLE && u->sink->monitor_source &&
        PA_SOURCE_IS_RUNNING(u->sink->monitor_source->thread_info.state));
    pa_sink_input *i;
    void *state = NULL;
    int nMultiChannel = 0;
    while ((i = pa_hashmap_iterate(u->sink->thread_info.inputs, &state, NULL))) {
        pa_sink_input_assert_ref(i);
        if (InputIsMultiChannel(i)) {
            nMultiChannel++;
        }
    }
    flag = flag && (nMultiChannel > 0);
    return flag;
}

static void ReleaseEffectBufferAndLimiter(struct Userdata *u)
{
    if (u->isEffectBufferAllocated == true) {
        FreeEffectBuffer(u);
        u->isEffectBufferAllocated = false;
    }
    // free limiter buffer
    FreeLimiter(u);
}

static void ProcessNormalData(struct Userdata *u)
{
    AUTO_CTRACE("ProcessNormalData");
    int64_t sleepForUsec = -1;
    pa_usec_t now = 0;

    if (u->sink->thread_info.state == PA_SINK_SUSPENDED) {
        ReleaseEffectBufferAndLimiter(u);
    }

    bool flag = (((u->render_in_idle_state && PA_SINK_IS_OPENED(u->sink->thread_info.state)) ||
                (!u->render_in_idle_state && PA_SINK_IS_RUNNING(u->sink->thread_info.state))) &&
                !(u->sink->thread_info.state == PA_SINK_IDLE && u->primary.previousState == PA_SINK_SUSPENDED) &&
                !(u->sink->thread_info.state == PA_SINK_IDLE && u->primary.previousState == PA_SINK_INIT)) ||
                (u->sink->thread_info.state == PA_SINK_IDLE && monitorLinked(u->sink, true));
    if (flag) {
        now = pa_rtclock_now();
    }

    if (PA_UNLIKELY(u->sink->thread_info.rewind_requested)) {
        pa_sink_process_rewind(u->sink, 0);
    }

    if (flag) {
        pa_usec_t frameUsec = pa_bytes_to_usec(u->sink->thread_info.max_request, &u->sink->sample_spec);
        pa_usec_t blockTime = u->primary.timestamp + frameUsec - now;
        if (blockTime > frameUsec) { blockTime = frameUsec; }
        if (pa_atomic_load(&u->primary.dflag) == 1) {
            sleepForUsec = (int64_t)blockTime -
                ((int64_t)pa_rtclock_now() - (int64_t)(u->primary.lastProcessDataTime));
            if (sleepForUsec < MIN_SLEEP_FOR_USEC) {
                sleepForUsec = MIN_SLEEP_FOR_USEC;
            }
        } else {
            if (u->primary.timestamp <= now + u->primary.prewrite || IsSinkNameDp(u->sink->name)) {
                pa_atomic_add(&u->primary.dflag, 1);
                u->primary.lastProcessDataTime = pa_rtclock_now();
                ProcessRenderUseTiming(u, now);
            }
            sleepForUsec = (int64_t)blockTime - ((int64_t)pa_rtclock_now() - (int64_t)now);
            if (u->primary.timestamp <= now + u->primary.prewrite) {
                sleepForUsec = PA_MIN(sleepForUsec, (int64_t)u->primary.writeTime);
            }
        }
        sleepForUsec = PA_MAX(sleepForUsec, 0);
    }

    if (sleepForUsec != -1) {
        if (u->timestampSleep == -1) {
            u->timestampSleep = (int64_t)pa_rtclock_now() + sleepForUsec;
        } else {
            u->timestampSleep = PA_MIN(u->timestampSleep, (int64_t)pa_rtclock_now() + sleepForUsec);
        }
    }
}

static void ProcessMCHData(struct Userdata *u)
{
    AUTO_CTRACE("ProcessMCHData");
    const uint64_t pw = u->multiChannel.prewrite;

    pa_usec_t now = 0;

    int64_t sleepForUsec = -1;

    now = pa_rtclock_now();

    if (PA_UNLIKELY(u->sink->thread_info.rewind_requested)) {
        pa_sink_process_rewind(u->sink, 0);
    }

    if (!ThreadFuncRendererTimerMultiChannelFlagJudge(u)) {
        return;
    }

    if (u->multiChannel.timestamp <= now + pw && pa_atomic_load(&u->multiChannel.dflag) == 0) {
        pa_atomic_add(&u->multiChannel.dflag, 1);
        ProcessRenderUseTimingMultiChannel(u, now);
    }
    pa_usec_t blockTime = pa_bytes_to_usec(u->sink->thread_info.max_request, &u->sink->sample_spec);
    sleepForUsec = PA_MIN((int64_t)blockTime - ((int64_t)pa_rtclock_now() - (int64_t)now),
        (int64_t)(u->multiChannel.writeTime));
    sleepForUsec = PA_MAX(sleepForUsec, 0);
    if (sleepForUsec != -1) {
        if (u->timestampSleep == -1) {
            u->timestampSleep = (int64_t)pa_rtclock_now() + sleepForUsec;
        } else {
            u->timestampSleep = PA_MIN(u->timestampSleep, (int64_t)pa_rtclock_now() + sleepForUsec);
        }
    }
}

static void ProcessOffloadData(struct Userdata *u)
{
    AUTO_CTRACE("ProcessOffloadData");
    pa_usec_t now = pa_rtclock_now();
    int64_t sleepForUsec = -1;
    bool flag;
    ThreadFuncRendererTimerOffloadFlag(u, now, &flag, &sleepForUsec);

    if (flag) {
        ThreadFuncRendererTimerOffloadProcess(u, now, &sleepForUsec);
        sleepForUsec = PA_MAX(sleepForUsec, 0);
    }

    if (u->offload.fullTs != 0) {
        if (u->offload.fullTs + 10 * PA_USEC_PER_MSEC > now) { // 10 is min checking size
            const int64_t s = ((int64_t)(u->offload.fullTs) + 10 * PA_USEC_PER_MSEC) - (int64_t)now;
            sleepForUsec = sleepForUsec == -1 ? s : PA_MIN(s, sleepForUsec);
        } else if (pa_atomic_load(&u->offload.hdistate) == 1) {
            u->offload.fullTs = 0;
            OffloadUnlock(u);
        } else {
            u->offload.fullTs = 0;
        }
    }

    if (sleepForUsec != -1) {
        if (u->timestampSleep == -1) {
            u->timestampSleep = (int64_t)pa_rtclock_now() + sleepForUsec;
        } else {
            u->timestampSleep = PA_MIN(u->timestampSleep, (int64_t)pa_rtclock_now() + sleepForUsec);
        }
    }
}

static void ThreadFuncRendererTimerProcessData(struct Userdata *u)
{
    if (u->timestampSleep < (int64_t)pa_rtclock_now()) {
        u->timestampSleep = -1;
    }

    pthread_rwlock_unlock(&u->rwlockSleep);

    static int64_t logCnt = 0;
    if (logCnt == 0) {
        AUDIO_INFO_LOG("Bus thread still running");
    }
    ++logCnt;
    if (logCnt > LOG_LOOP_THRESHOLD) {
        logCnt = 0;
    }

    u->primary.onlyPrimarySpeakerPaLoading = true;
    u->primary.speakerPaAllStreamVolumeZero = true;
    CheckOnlyPrimarySpeakerPaLoading(u);
    if (!strcmp(u->sink->name, MCH_SINK_NAME)) {
        ProcessMCHData(u);
    } else if (!strcmp(u->sink->name, OFFLOAD_SINK_NAME) && u->offload_enable) {
        ProcessOffloadData(u);
    } else {
        ProcessNormalData(u);
    }
}

static void SetThreadPriority(char *sinkName)
{
    if (!strcmp(sinkName, OFFLOAD_SINK_NAME)) {
        // offload process data thread does not need to set qos priority
        ScheduleThreadInServer(getpid(), gettid());
        return;
    }

    AUDIO_INFO_LOG("set thread priority start");
    int32_t setpriority = GetIntParameter("const.multimedia.audio_setPriority", 1);
    SetProcessDataThreadPriority(setpriority);
}

static void UnsetThreadPriority(char *sinkName)
{
    if (!strcmp(sinkName, OFFLOAD_SINK_NAME)) {
        // offload case
        UnscheduleThreadInServer(getpid(), gettid());
        return;
    }

    // primary case
    ResetProcessDataThreadPriority();
}

static void ThreadFuncRendererTimerBus(void *userdata)
{
    struct Userdata *u = userdata;

    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    // set audio thread priority
    SetThreadPriority(u->sink->name);

    const char *deviceClass = u->primary.sinkAdapter->deviceClass;
    AUDIO_INFO_LOG("Thread %s(use timing bus) starting up, pid %d, tid %d", deviceClass, getpid(), gettid());
    pa_thread_mq_install(&u->thread_mq);

    if (!strcmp(u->sink->name, OFFLOAD_SINK_NAME)) {
        OffloadReset(u);
        CHECK_AND_RETURN_LOG(u->offload.sinkAdapter != NULL, "offload.sinkAdapter is NULL");
    }
    while (true) {
        int ret;
        pthread_rwlock_wrlock(&u->rwlockSleep);

        int64_t sleepForUsec = 0;

        if (u->timestampSleep == -1) {
            pa_rtpoll_set_timer_disabled(u->rtpoll); // sleep forever
        } else if ((sleepForUsec = u->timestampSleep - (int64_t)(pa_rtclock_now())) <= 0) {
            pa_rtpoll_set_timer_relative(u->rtpoll, 0);
        } else {
            pa_rtpoll_set_timer_relative(u->rtpoll, sleepForUsec);
        }

        AUTO_CTRACE("ProcessDataLoop %s sleep:%lld us", deviceClass, sleepForUsec);
        // Hmm, nothing to do. Let's sleep
        if ((ret = pa_rtpoll_run(u->rtpoll)) < 0) {
            AUDIO_ERR_LOG("Thread %{public}s(use timing bus) shutting down, error %{public}d, "
                "pid %{public}d, tid %{public}d", deviceClass, ret, getpid(), gettid());
            if (!strcmp(deviceClass, DEVICE_CLASS_PRIMARY)) {
                AUDIO_ERR_LOG("Primary sink's pa_rtpoll_run error, exit");
                _Exit(0);
            }

            // If this was no regular exit from the loop we have to continue
            // processing messages until we received PA_MESSAGE_SHUTDOWN
            pa_asyncmsgq_post(u->thread_mq.outq, PA_MSGOBJECT(u->core), PA_CORE_MESSAGE_UNLOAD_MODULE,
                u->module, 0, NULL, NULL);
            pa_asyncmsgq_wait_for(u->thread_mq.inq, PA_MESSAGE_SHUTDOWN);
            pthread_rwlock_unlock(&u->rwlockSleep);
            break;
        }

        if (ret == 0) {
            AUDIO_INFO_LOG("Thread %{public}s(use timing bus) shutting down, pid %{public}d, tid %{public}d",
                deviceClass, getpid(), gettid());
            pthread_rwlock_unlock(&u->rwlockSleep);
            break;
        }

        ThreadFuncRendererTimerProcessData(u);
    }

    // Unset audio thread priority
    UnsetThreadPriority(u->sink->name);
}

static void ThreadFuncWriteHDIMultiChannel(void *userdata)
{
    AUDIO_DEBUG_LOG("ThreadFuncWriteHDIMultiChannel start");
    // set audio thread priority
    ScheduleThreadInServer(getpid(), gettid());

    struct Userdata *u = userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    int32_t quit = 0;

    do {
        int32_t code = 0;
        pa_memchunk chunk;

        pa_assert_se(pa_asyncmsgq_get(u->multiChannel.dq, NULL, &code, NULL, NULL, &chunk, 1) == 0);

        switch (code) {
            case HDI_RENDER: {
                pa_usec_t now = pa_rtclock_now();
                if (RenderWrite(u->multiChannel.sinkAdapter, &chunk) < 0) {
                    AUDIO_DEBUG_LOG("ThreadFuncWriteHDIMultiChannel RenderWrite");
                    u->bytes_dropped += chunk.length;
                }
                if (pa_atomic_load(&u->multiChannel.dflag) == 1) {
                    pa_atomic_sub(&u->multiChannel.dflag, 1);
                }
                u->multiChannel.writeTime = pa_rtclock_now() - now;
                break;
            }
            case QUIT:
                quit = 1;
                break;
            default:
                break;
        }
        pa_asyncmsgq_done(u->multiChannel.dq, 0);
    } while (!quit);
    UnscheduleThreadInServer(getpid(), gettid());
}

static void ProcessHdiRendererPrimary(struct Userdata *u, pa_memchunk *pChunk)
{
    pa_usec_t now = pa_rtclock_now();
    if (pa_atomic_load(&u->primary.isHDISinkStarted) != 1 && now - u->timestampLastLog > USEC_PER_SEC) {
        u->timestampLastLog = now;
        const char *deviceClass = u->primary.sinkAdapter->deviceClass;
        AUDIO_DEBUG_LOG("HDI not started, skip RenderWrite, wait sink[%s] suspend", deviceClass);
        pa_memblock_unref(pChunk->memblock);
    } else if (pa_atomic_load(&u->primary.isHDISinkStarted) != 1) {
        pa_memblock_unref(pChunk->memblock);
    } else if (RenderWrite(u->primary.sinkAdapter, pChunk) < 0) {
        u->bytes_dropped += pChunk->length;
        AUDIO_ERR_LOG("RenderWrite failed");
    }
    if (pa_atomic_load(&u->primary.dflag) == 1) {
        pa_atomic_sub(&u->primary.dflag, 1);
    }
    u->primary.writeTime = pa_rtclock_now() - now;
}

static void ThreadFuncWriteHDI(void *userdata)
{
    // set audio thread priority
    ScheduleThreadInServer(getpid(), gettid());

    struct Userdata *u = userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    int32_t quit = 0;

    do {
        int32_t code = 0;
        pa_memchunk chunk;

        CHECK_AND_RETURN_LOG(u->primary.dq != NULL, "u->primary.dq is NULL");
        pa_assert_se(pa_asyncmsgq_get(u->primary.dq, NULL, &code, NULL, NULL, &chunk, 1) == 0);

        int ret = 0;
        AUTO_CTRACE("hdi_sink::ThreadFuncWriteHDI code: %d", code);
        switch (code) {
            case HDI_RENDER: {
                ProcessHdiRendererPrimary(u, &chunk);
                break;
            }
            case HDI_STOP: {
                if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
                    u->primary.sinkAdapter->SinkAdapterStop(u->primary.sinkAdapter);
                    AUDIO_INFO_LOG("Stopped HDI renderer");
                    pa_atomic_store(&u->primary.isHDISinkStarted, 0);
                }
                break;
            }
            case HDI_START: {
                ret = u->primary.sinkAdapter->SinkAdapterStart(u->primary.sinkAdapter);
                break;
            }
            case QUIT:
                quit = 1;
                break;
            default:
                break;
        }
        AUTO_CTRACE("hdi_sink::ThreadFuncWriteHDI done ret: %d", ret);
        pa_asyncmsgq_done(u->primary.dq, ret);
    } while (!quit);
    UnscheduleThreadInServer(getpid(), gettid());
}

static void TestModeThreadFuncWriteHDI(void *userdata)
{
    // set audio thread priority
    ScheduleThreadInServer(getpid(), gettid());

    struct Userdata *u = userdata;
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");

    int32_t quit = 0;

    do {
        int32_t code = 0;
        pa_memchunk chunk;

        pa_assert_se(pa_asyncmsgq_get(u->primary.dq, NULL, &code, NULL, NULL, &chunk, 1) == 0);

        switch (code) {
            case HDI_RENDER:
                if (TestModeRenderWrite(u, &chunk) < 0) {
                    u->bytes_dropped += chunk.length;
                    AUDIO_ERR_LOG("TestModeRenderWrite failed");
                }
                if (pa_atomic_load(&u->primary.dflag) == 1) {
                    pa_atomic_sub(&u->primary.dflag, 1);
                }
                break;
            case QUIT:
                quit = 1;
                break;
            default:
                break;
        }
        pa_asyncmsgq_done(u->primary.dq, 0);
    } while (!quit);
    UnscheduleThreadInServer(getpid(), gettid());
}

static void SinkUpdateRequestedLatencyCb(pa_sink *s)
{
    struct Userdata *u = NULL;
    size_t nbytes;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    u->block_usec = pa_sink_get_requested_latency_within_thread(s);

    if (u->block_usec == (pa_usec_t) - 1)
        u->block_usec = s->thread_info.max_latency;

    if (u->block_usec < DEFAULT_BLOCK_USEC) {
        AUDIO_WARNING_LOG("block_usec is less than 20000, block_usec: %{public}" PRIu64, u->block_usec);
        u->block_usec = DEFAULT_BLOCK_USEC;
    }
    nbytes = pa_usec_to_bytes(u->block_usec, &s->sample_spec);
    pa_sink_set_max_request_within_thread(s, nbytes);
}

static void CheckAndPrintLatency(uint64_t lastRecodedLatency, uint64_t latency, bool getLatencyFromHdiSucess,
    uint32_t continuesGetLatencyErrCount, uint64_t logThreshold)
{
    uint64_t latencyDifference = (latency > lastRecodedLatency)
        ? (latency - lastRecodedLatency) : (lastRecodedLatency - latency);
    if (latencyDifference > logThreshold) {
        AUDIO_INFO_LOG("lastLatency: %{public}" PRIu64 " latency: %{public}" PRIu64 ""
            " getLatencyFromHdiSucess: %{public}d continuesGetLatencyErrCount: %{public}u",
            lastRecodedLatency, latency, getLatencyFromHdiSucess, continuesGetLatencyErrCount);
    }
}

static int32_t SinkProcessMsg(pa_msgobject *o, int32_t code, void *data, int64_t offset,
    pa_memchunk *chunk)
{
    AUDIO_DEBUG_LOG("SinkProcessMsg: code: %{public}d", code);
    AUTO_CTRACE("hdi_sink::SinkProcessMsg code: %d", code);
    struct Userdata *u = PA_SINK(o)->userdata;
    CHECK_AND_RETURN_RET_LOG(u != NULL, 0, "u is null");

    switch (code) {
        case PA_SINK_MESSAGE_GET_LATENCY: {
            if (!strcmp(u->primary.sinkAdapter->deviceClass, DEVICE_CLASS_OFFLOAD)) {
                *((uint64_t *)data) = CalcOffloadCacheLenInHdi(u);
            } else if (u->sink_latency) {
                *((uint64_t *)data) = u->sink_latency * PA_USEC_PER_MSEC;
            } else if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
                uint64_t latency;
                uint32_t hdiLatency;
                bool getLatencyFromHdiSucess = true;
                // Tries to fetch latency from HDI else will make an estimate based
                // on samples to be rendered based on the timestamp and current time
                if (u->primary.sinkAdapter->SinkAdapterGetLatency(u->primary.sinkAdapter, &hdiLatency) == 0) {
                    latency = (PA_USEC_PER_MSEC * hdiLatency);
                } else {
                    pa_usec_t now = pa_rtclock_now();
                    latency = (now - u->primary.timestamp);
                    getLatencyFromHdiSucess = false;
                }

                *((uint64_t *)data) = latency;
                CheckAndPrintLatency(u->lastRecodedLatency, latency, getLatencyFromHdiSucess,
                    u->continuesGetLatencyErrCount, DEFAULT_GETLATENCY_LOG_THRESHOLD_MS);
                u->lastRecodedLatency = latency;
                u->continuesGetLatencyErrCount = getLatencyFromHdiSucess ? 0 : (u->continuesGetLatencyErrCount + 1);
            }
            return 0;
        }
        default:
            break;
    }
    return pa_sink_process_msg(o, code, data, offset, chunk);
}

static char *GetStateInfo(pa_sink_state_t state)
{
    switch (state) {
        case PA_SINK_INVALID_STATE:
            return "INVALID";
        case PA_SINK_RUNNING:
            return "RUNNING";
        case PA_SINK_IDLE:
            return "IDLE";
        case PA_SINK_SUSPENDED:
            return "SUSPENDED";
        case PA_SINK_INIT:
            return "INIT";
        case PA_SINK_UNLINKED:
            return "UNLINKED";
        default:
            return "error state";
    }
}

static char *GetInputStateInfo(pa_sink_input_state_t state)
{
    switch (state) {
        case PA_SINK_INPUT_INIT:
            return "INIT";
        case PA_SINK_INPUT_RUNNING:
            return "RUNNING";
        case PA_SINK_INPUT_CORKED:
            return "CORKED";
        case PA_SINK_INPUT_UNLINKED:
            return "UNLINKED";
        default:
            return "UNKNOWN";
    }
}

// call from IO thread(OS_ProcessData)
static int32_t RemoteSinkStateChange(pa_sink *s, pa_sink_state_t newState)
{
    struct Userdata *u = s->userdata;
    if (s->thread_info.state == PA_SINK_INIT || newState == PA_SINK_INIT) {
        u->isFirstStarted = false;
    }

    if (!u->isFirstStarted && (newState == PA_SINK_RUNNING)) {
        u->primary.timestamp = pa_rtclock_now();
        u->isFirstStarted = true;
    }

    if (s->thread_info.state == PA_SINK_INIT && newState == PA_SINK_IDLE) {
        AUDIO_INFO_LOG("First start.");
    }

    if (s->thread_info.state == PA_SINK_SUSPENDED && PA_SINK_IS_OPENED(newState)) {
        u->primary.timestamp = pa_rtclock_now();
        if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
            return 0;
        }

        if (pa_asyncmsgq_send(u->primary.dq, NULL, HDI_START, NULL, 0, NULL)) {
            AUDIO_ERR_LOG("audiorenderer control start failed!");
        } else {
            pa_atomic_store(&u->primary.isHDISinkStarted, 1);
            u->render_in_idle_state = 1; // enable to reduce noise from idle to running.
            u->writeCount = 0;
            u->renderCount = 0;
            AUDIO_INFO_LOG("Successfully restarted remote renderer");
        }
    }
    if (PA_SINK_IS_OPENED(s->thread_info.state) && newState == PA_SINK_SUSPENDED) {
        // Continuously dropping data (clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("HDI-sink continuously dropping data - clear statistics (%zu -> 0 bytes dropped)",
                           u->bytes_dropped);
            u->bytes_dropped = 0;
        }

        if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
            pa_asyncmsgq_post(u->primary.dq, NULL, HDI_STOP, NULL, 0, NULL, NULL);
        }
    }

    return 0;
}

// call from IO thread(OS_ProcessData)
static int32_t SinkSetStateInIoThreadCbStartPrimary(struct Userdata *u, pa_sink_state_t newState)
{
    if (!PA_SINK_IS_OPENED(newState)) {
        return 0;
    }

    u->primary.timestamp = pa_rtclock_now();
    if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
        return 0;
    }

    if (u->sink->thread_info.state == PA_SINK_SUSPENDED && newState == PA_SINK_IDLE) {
        AUDIO_INFO_LOG("Primary sink from suspend to idle");
        return 0;
    }

    if (pa_asyncmsgq_send(u->primary.dq, NULL, HDI_START, NULL, 0, NULL)) {
        AUDIO_ERR_LOG("audiorenderer control start failed!");
        u->primary.sinkAdapter->SinkAdapterDeInit(u->primary.sinkAdapter);
    } else {
        pa_atomic_store(&u->primary.isHDISinkStarted, 1);
        u->writeCount = 0;
        u->renderCount = 0;
        AUDIO_INFO_LOG("SinkSetStateInIoThreadCbStartPrimary, Successfully restarted HDI renderer");
    }
    return 0;
}

static int32_t SinkSetStateInIoThreadCbStartMultiChannel(struct Userdata *u, pa_sink_state_t newState)
{
    if (!PA_SINK_IS_OPENED(newState)) {
        return 0;
    }

    u->multiChannel.timestamp = pa_rtclock_now();

    ResetMultiChannelHdiState(u);
    return 0;
}

static void OffloadSinkStateChangeCb(pa_sink *sink, pa_sink_state_t newState)
{
    struct Userdata *u = (struct Userdata *)(sink->userdata);
    const bool starting = PA_SINK_IS_OPENED(newState);
    const bool stopping = newState == PA_SINK_SUSPENDED;
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");
    AUDIO_INFO_LOG("starting: %{public}d, stopping: %{public}d, offload_enable: %{public}d",
        starting, stopping, u->offload_enable);
    if (starting && u->offload_enable && !u->offload.inited && PrepareDeviceOffload(u) == 0) {
        u->offload.inited = true;
    } else if (stopping && u->offload_enable) {
        if (u->offload.isHDISinkStarted) {
            u->offload.sinkAdapter->SinkAdapterStop(u->offload.sinkAdapter);
            AUDIO_INFO_LOG("Stopped Offload HDI renderer, DeInit later");
            u->offload.isHDISinkStarted = false;
        }
        OffloadReset(u);
        OffloadUnlock(u);
        u->primary.speakerPaAllStreamStartVolZeroTime = 0;
    }
}

static void MultiChannelSinkStateChangeCb(pa_sink *sink, pa_sink_state_t newState)
{
    struct Userdata *u = (struct Userdata *)(sink->userdata);
    CHECK_AND_RETURN_LOG(u != NULL, "u is null");
    if (sink->thread_info.state == PA_SINK_SUSPENDED || sink->thread_info.state == PA_SINK_INIT ||
        newState == PA_SINK_RUNNING) {
        if (EffectChainManagerCheckEffectOffload()) {
            SinkSetStateInIoThreadCbStartMultiChannel(u, newState);
        }
    } else if (PA_SINK_IS_OPENED(sink->thread_info.state)) {
        if (newState != PA_SINK_SUSPENDED) {
            return;
        }
        // Continuously dropping data (clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("HDI-sink continuously dropping data - clear statistics (%zu -> 0 bytes dropped)",
                           u->bytes_dropped);
            u->bytes_dropped = 0;
        }

        if (u->multiChannel.isHDISinkStarted) {
            u->multiChannel.sinkAdapter->SinkAdapterStop(u->multiChannel.sinkAdapter);
            AUDIO_INFO_LOG("MultiChannel Stopped HDI renderer");
            u->multiChannel.isHDISinkStarted = false;
        }
    }
}

// Called from the IO thread.
static int32_t SinkSetStateInIoThreadCb(pa_sink *s, pa_sink_state_t newState, pa_suspend_cause_t newSuspendCause)
{
    struct Userdata *u = NULL;

    CHECK_AND_RETURN_RET_LOG(s != NULL, 0, "s is null");
    pa_assert_se(u = s->userdata);

    AUDIO_INFO_LOG("Sink[%{public}s] state change:[%{public}s]-->[%{public}s]",
        u->primary.sinkAdapter->deviceClass, GetStateInfo(s->thread_info.state),
        GetStateInfo(newState));
    u->primary.previousState = u->sink->thread_info.state;

    if (!strcmp(u->primary.sinkAdapter->deviceClass, DEVICE_CLASS_REMOTE)) {
        return RemoteSinkStateChange(s, newState);
    }

    if (!strcmp(u->sink->name, OFFLOAD_SINK_NAME)) {
        OffloadSinkStateChangeCb(s, newState);
        return 0;
    }

    if (!strcmp(u->sink->name, MCH_SINK_NAME)) {
        MultiChannelSinkStateChangeCb(s, newState);
        return 0;
    }

    if (s->thread_info.state == PA_SINK_SUSPENDED || s->thread_info.state == PA_SINK_INIT ||
        newState == PA_SINK_RUNNING) {
        if (strcmp(u->sink->name, BT_SINK_NAME) || newState == PA_SINK_RUNNING) {
            return SinkSetStateInIoThreadCbStartPrimary(u, newState);
        }
    } else if (PA_SINK_IS_OPENED(s->thread_info.state)) {
        if (newState != PA_SINK_SUSPENDED) {
            return 0;
        }
        // Continuously dropping data (clear counter on entering suspended state.
        if (u->bytes_dropped != 0) {
            AUDIO_INFO_LOG("HDI-sink continuously dropping data - clear statistics (%zu -> 0 bytes dropped)",
                           u->bytes_dropped);
            u->bytes_dropped = 0;
        }

        if (pa_atomic_load(&u->primary.isHDISinkStarted) == 1) {
            pa_asyncmsgq_post(u->primary.dq, NULL, HDI_STOP, NULL, 0, NULL, NULL);
        }
    }

    return 0;
}

static pa_hook_result_t SinkInputMoveStartCb(pa_core *core, pa_sink_input *i, struct Userdata *u)
{
    pa_sink_input_assert_ref(i);
    char str[SPRINTF_STR_LEN] = {0};
    GetSinkInputName(i, str, SPRINTF_STR_LEN);
    AUDIO_INFO_LOG("SinkInputMoveStartCb sink[%{public}s] - %{public}s", i->sink->name, str);
    if (u->offload_enable && !strcmp(i->sink->name, OFFLOAD_SINK_NAME) &&
        i->thread_info.state == PA_SINK_INPUT_RUNNING) {
        const bool maybeOffload = pa_memblockq_get_maxrewind(i->thread_info.render_memblockq) != 0;
        if (maybeOffload || InputIsOffload(i)) {
            OffloadRewindAndFlush(u, i, false);
            pa_sink_input_update_max_rewind(i, 0);
        }
    }
    if (strcmp(i->sink->name, OFFLOAD_SINK_NAME)) {
        // move sink input from xx_speaker, need remove stream volume map for xx_speaker, except offload
        RemoveVolumeFromStreamVolumeMap(u, i);
    }
    return PA_HOOK_OK;
}

static pa_hook_result_t SinkInputStateChangedCb(pa_core *core, pa_sink_input *i, struct Userdata *u)
{
    pa_sink_input_assert_ref(i);
    char str[SPRINTF_STR_LEN] = {0};
    GetSinkInputName(i, str, SPRINTF_STR_LEN);
    AUDIO_INFO_LOG("SinkInputStateChangedCb sink[%{public}s] - %{public}s", i->sink->name, str);
    if (u->offload_enable && !strcmp(i->sink->name, OFFLOAD_SINK_NAME)) {
        if (i->thread_info.state == PA_SINK_INPUT_CORKED) {
            pa_atomic_store(&u->offload.hdistate, 0);
        }
    }
    return PA_HOOK_OK;
}

static pa_hook_result_t SinkInputPutCb(pa_core *core, pa_sink_input *i, struct Userdata *u)
{
    pa_sink_input_assert_ref(i);
    const char *streamMode = pa_proplist_gets(i->proplist, "stream.mode");
    if (streamMode != NULL && !strcmp(streamMode, DUP_STEAM_NAME)) {
        AUDIO_INFO_LOG("Dup stream is dismissed:%{public}u", i->index);
        return PA_HOOK_OK;
    }
    i->state_change = PaInputStateChangeCb;
    return PA_HOOK_OK;
}

static int32_t PrepareDevice(struct Userdata *u, const char *filePath)
{
    struct SinkAdapterAttr sample_attrs;
    int32_t ret;

    sample_attrs.format = ConvertPaToHdiAdapterFormat(u->ss.format);
    sample_attrs.adapterName = u->defaultAdapterEnable ? "dp" : u->adapterName;
    sample_attrs.openMicSpeaker = u->open_mic_speaker;
    sample_attrs.sampleRate = (uint32_t) u->ss.rate;
    sample_attrs.channel = u->ss.channels;
    sample_attrs.volume = MAX_SINK_VOLUME_LEVEL;
    sample_attrs.filePath = filePath;
    sample_attrs.deviceNetworkId = u->deviceNetworkId;
    sample_attrs.deviceType =  u->deviceType;

    if (!strcmp(u->primary.sinkAdapter->deviceClass, DEVICE_CLASS_MULTICHANNEL)) {
        sample_attrs.channel = DEFAULT_MULTICHANNEL_NUM;
        sample_attrs.channelLayout = DEFAULT_MULTICHANNEL_CHANNELLAYOUT;
    }

    ret = u->primary.sinkAdapter->SinkAdapterInit(u->primary.sinkAdapter, &sample_attrs);
    if (ret != 0) {
        AUDIO_ERR_LOG("audiorenderer Init failed!");
        return -1;
    }

    if (ret != 0) {
        AUDIO_ERR_LOG("audiorenderer control start failed!");
        u->primary.sinkAdapter->SinkAdapterDeInit(u->primary.sinkAdapter);
        return -1;
    }

    return 0;
}

static int32_t PrepareDeviceOffload(struct Userdata *u)
{
    const char *adapterName = safeProplistGets(u->sink->proplist, PA_PROP_DEVICE_STRING, "");
    const char *filePath = safeProplistGets(u->sink->proplist, "filePath", "");
    const char *deviceNetworkId = safeProplistGets(u->sink->proplist, "NetworkId", "");
    AUDIO_INFO_LOG("PrepareDeviceOffload enter, deviceClass %s, filePath %s",
        u->offload.sinkAdapter->deviceClass, filePath);
    struct SinkAdapterAttr sample_attrs;
    int32_t ret;

    enum AudioSampleFormatIntf format = ConvertPaToHdiAdapterFormat(u->ss.format);
    sample_attrs.format = format;
    AUDIO_INFO_LOG("PrepareDeviceOffload audiorenderer format: %d ,adapterName %s",
        sample_attrs.format, u->offload.sinkAdapter->deviceClass);
    sample_attrs.adapterName = u->defaultAdapterEnable ? "dp" : adapterName;
    sample_attrs.openMicSpeaker = u->open_mic_speaker;
    sample_attrs.sampleRate = u->ss.rate;
    sample_attrs.channel = u->ss.channels;
    sample_attrs.volume = MAX_SINK_VOLUME_LEVEL;
    sample_attrs.filePath = filePath;
    sample_attrs.deviceNetworkId = deviceNetworkId;
    sample_attrs.deviceType = u->deviceType;

    ret = u->offload.sinkAdapter->SinkAdapterInit(u->offload.sinkAdapter, &sample_attrs);
    if (ret != 0) {
        AUDIO_ERR_LOG("PrepareDeviceOffload audiorenderer Init failed!");
        return -1;
    }

    return 0;
}

static int32_t PrepareDeviceMultiChannel(struct Userdata *u, struct SinkAdapter *sinkAdapter,
    const char *filePath)
{
    int32_t ret;

    enum AudioSampleFormatIntf format = ConvertPaToHdiAdapterFormat(u->ss.format);

    u->multiChannel.sample_attrs.format = format;
    u->multiChannel.sample_attrs.sampleRate = u->ss.rate;
    AUDIO_INFO_LOG("PrepareDeviceMultiChannel format: %d ,adapterName %s",
        u->multiChannel.sample_attrs.format, sinkAdapter->deviceClass);
    u->multiChannel.sample_attrs.adapterName = u->defaultAdapterEnable ? "dp" : u->adapterName;
    u->multiChannel.sample_attrs.openMicSpeaker = u->open_mic_speaker;
    u->multiChannel.sample_attrs.sampleRate = u->ss.rate;
    u->multiChannel.sample_attrs.channel = DEFAULT_MULTICHANNEL_NUM;
    u->multiChannel.sample_attrs.channelLayout = DEFAULT_MULTICHANNEL_CHANNELLAYOUT;
    u->multiChannel.sinkChannel = DEFAULT_MULTICHANNEL_NUM;
    u->multiChannel.sinkChannelLayout = DEFAULT_MULTICHANNEL_CHANNELLAYOUT;
    u->multiChannel.sample_attrs.volume = MAX_SINK_VOLUME_LEVEL;
    u->multiChannel.sample_attrs.filePath = filePath;
    u->multiChannel.sample_attrs.deviceNetworkId = u->deviceNetworkId;
    u->multiChannel.sample_attrs.deviceType = u->deviceType;

    ret = sinkAdapter->SinkAdapterInit(sinkAdapter, &u->multiChannel.sample_attrs);
    if (ret != 0) {
        AUDIO_ERR_LOG("PrepareDeviceMultiChannel Init failed!");
        return -1;
    }
    u->multiChannel.isHDISinkInited = true;
    AUDIO_DEBUG_LOG("PrepareDeviceMultiChannel init success");
    return 0;
}

static void PaHdiSinkUserdataInit(struct Userdata *u)
{
    u->format = u->ss.format;
    u->processLen = IN_CHANNEL_NUM_MAX * DEFAULT_FRAMELEN;
    u->processSize = (uint32_t)u->processLen * sizeof(float);
    u->bufferAttr = pa_xnew0(BufferAttr, 1);
    u->bufferAttr->samplingRate = (int32_t)u->ss.rate;
    u->bufferAttr->frameLen = DEFAULT_FRAMELEN;
    u->bufferAttr->numChanIn = u->ss.channels;
    u->bufferAttr->numChanOut = u->ss.channels;
    u->bufferAttr->bufOutUsed = true;
    u->bufferAttr->outChanLayout = DEFAULT_CHANNELLAYOUT;
    u->sinkSceneMode = -1;
    u->sinkSceneType = -1;
}

static pa_sink *PaHdiSinkInit(struct Userdata *u, pa_modargs *ma, const char *driver)
{
    pa_sink_new_data data;
    pa_module *m;
    pa_sink *sink = NULL;

    m = u->module;
    u->ss = m->core->default_sample_spec;
    u->map = m->core->default_channel_map;
    if (pa_modargs_get_sample_spec_and_channel_map(ma, &u->ss, &u->map, PA_CHANNEL_MAP_DEFAULT) < 0) {
        AUDIO_ERR_LOG("Failed to parse sample specification and channel map");
        goto fail;
    }

    AUDIO_INFO_LOG("Initializing HDI rendering device with rate: %{public}d, channels: %{public}d",
        u->ss.rate, u->ss.channels);
    if (PrepareDevice(u, pa_modargs_get_value(ma, "file_path", "")) < 0) { goto fail; }

    u->primary.prewrite = 0;
    if (u->offload_enable && !strcmp(u->primary.sinkAdapter->deviceClass, DEVICE_CLASS_PRIMARY)) {
        u->primary.prewrite = u->block_usec * 7; // 7 frame, set cache len in hdi, avoid pop
    }

    AUDIO_DEBUG_LOG("Initialization of HDI rendering device[%{public}s] completed", u->adapterName);
    pa_sink_new_data_init(&data);
    data.driver = driver;
    data.module = m;

    PaHdiSinkUserdataInit(u);
    pa_sink_new_data_set_name(&data, pa_modargs_get_value(ma, "sink_name", DEFAULT_SINK_NAME));
    pa_sink_new_data_set_sample_spec(&data, &u->ss);
    pa_sink_new_data_set_channel_map(&data, &u->map);
    pa_proplist_sets(data.proplist, PA_PROP_DEVICE_STRING,
        (u->adapterName ? u->adapterName : DEFAULT_AUDIO_DEVICE_NAME));
    pa_proplist_setf(data.proplist, PA_PROP_DEVICE_DESCRIPTION, "HDI sink is %s",
        (u->adapterName ? u->adapterName : DEFAULT_AUDIO_DEVICE_NAME));
    pa_proplist_sets(data.proplist, "filePath", pa_modargs_get_value(ma, "file_path", ""));
    pa_proplist_sets(data.proplist, "networkId", pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID));

    if (pa_modargs_get_proplist(ma, "sink_properties", data.proplist, PA_UPDATE_REPLACE) < 0) {
        AUDIO_ERR_LOG("Invalid properties");
        pa_sink_new_data_done(&data);
        goto fail;
    }

    if (u->fixed_latency) {
        sink = pa_sink_new(m->core, &data, PA_SINK_HARDWARE | PA_SINK_LATENCY);
    } else {
        sink = pa_sink_new(m->core, &data,
            PA_SINK_HARDWARE | PA_SINK_LATENCY | PA_SINK_DYNAMIC_LATENCY);
    }
    pa_sink_new_data_done(&data);

    return sink;

fail:
    AUDIO_ERR_LOG("PaHdiSinkInit fail");
    return NULL;
}

static int32_t PaHdiSinkNewInitThreadMultiChannel(pa_module *m, pa_modargs *ma, struct Userdata *u)
{
    pa_atomic_store(&u->multiChannel.dflag, 0);
    u->multiChannel.dq = pa_asyncmsgq_new(0);
    u->multiChannel.sinkAdapter = GetSinkAdapter(DEVICE_CLASS_MULTICHANNEL, NULL);
    if (u->multiChannel.sinkAdapter == NULL) {
        AUDIO_ERR_LOG("Load mch adapter failed");
        return -1;
    }
    if (PrepareDeviceMultiChannel(u, u->multiChannel.sinkAdapter, pa_modargs_get_value(ma, "file_path", "")) < 0) {
        return -1;
    }

    u->multiChannel.used = true;

    u->multiChannel.chunk.memblock = pa_memblock_new(u->sink->core->mempool, -1); // -1 == pa_mempool_block_size_max

    return 0;
}

static int32_t PaHdiSinkNewInitThread(pa_module *m, pa_modargs *ma, struct Userdata *u)
{
    char *paThreadName = NULL;

    const char *deviceClass = u->primary.sinkAdapter->deviceClass;
    if (!strcmp(u->sink->name, OFFLOAD_SINK_NAME) && u->offload_enable) {
        AUDIO_DEBUG_LOG("PaHdiSinkNew device[%s] sink[%s] init offload thread", deviceClass, u->sink->name);
        u->offload.sinkAdapter = GetSinkAdapter(DEVICE_CLASS_OFFLOAD, NULL);
        if (u->offload.sinkAdapter == NULL) {
            AUDIO_ERR_LOG("Load adapter failed");
            return -1;
        }
        pthread_mutex_init(&u->offload.lockCallback, NULL);
        u->offload.inited = false;
        u->offload.dq = pa_asyncmsgq_new(0);
        pa_atomic_store(&u->offload.dflag, 0);
        pa_atomic_store(&u->offload.hdistate, 0);
        u->offload.chunk.memblock = pa_memblock_new(u->sink->core->mempool,
            pa_usec_to_bytes(200 * PA_USEC_PER_MSEC, &u->sink->sample_spec)); // 200ms for max len once offload render
        pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SINK_INPUT_MOVE_START], PA_HOOK_LATE,
            (pa_hook_cb_t)SinkInputMoveStartCb, u);
        pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SINK_INPUT_STATE_CHANGED], PA_HOOK_NORMAL,
            (pa_hook_cb_t)SinkInputStateChangedCb, u);
    } else {
        AUDIO_INFO_LOG("PaHdiSinkNew device[%s] sink[%s] skip offload thread", deviceClass, u->sink->name);
    }

    if (!strcmp(u->sink->name, MCH_SINK_NAME)) {
        PaHdiSinkNewInitThreadMultiChannel(m, ma, u);
        u->multichannel_enable = true;
    } else {
        u->multichannel_enable = false;
    }

    if (!strcmp(u->sink->name, "Speaker") || !strcmp(u->sink->name, MCH_SINK_NAME) ||
        !strcmp(u->sink->name, OFFLOAD_SINK_NAME) || IsSinkNameDp(u->sink->name)) {
        pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SINK_INPUT_PUT], PA_HOOK_EARLY,
            (pa_hook_cb_t)SinkInputPutCb, u);
    }

    paThreadName = "OS_ProcessData";
    if (!(u->thread = pa_thread_new(paThreadName, ThreadFuncRendererTimerBus, u))) {
        AUDIO_ERR_LOG("Failed to create bus thread.");
        return -1;
    }

    return 0;
}

static int32_t PaHdiSinkNewInitUserData(pa_module *m, pa_modargs *ma, struct Userdata *u)
{
    CHECK_AND_RETURN_RET_LOG(m != NULL, -1, "m is null");
    u->core = m->core;
    u->module = m;

    pa_memchunk_reset(&u->memchunk);
    u->rtpoll = pa_rtpoll_new();
    pthread_rwlock_init(&u->rwlockSleep, NULL);
    pthread_mutex_init(&u->mutexPa, NULL);
    pthread_mutex_init(&u->mutexPa2, NULL);

    if (pa_thread_mq_init(&u->thread_mq, m->core->mainloop, u->rtpoll) < 0) {
        AUDIO_ERR_LOG("pa_thread_mq_init() failed.");
        return -1;
    }

    AUDIO_INFO_LOG("Load sink adapter");
    const char *deviceClass = pa_modargs_get_value(ma, "device_class", DEFAULT_DEVICE_CLASS);
    u->primary.sinkAdapter = GetSinkAdapter(deviceClass, pa_modargs_get_value(ma, "network_id",
        DEFAULT_DEVICE_NETWORKID));
    if (u->primary.sinkAdapter == NULL) {
        AUDIO_ERR_LOG("Load adapter failed");
        return -1;
    }
    if (pa_modargs_get_value_u32(ma, "fixed_latency", &u->fixed_latency) < 0) {
        AUDIO_ERR_LOG("Failed to parse fixed latency argument.");
        return -1;
    }
    if (pa_modargs_get_value_s32(ma, "device_type", &u->deviceType) < 0) {
        AUDIO_ERR_LOG("Failed to parse deviceType argument.");
        return -1;
    }

    u->adapterName = pa_modargs_get_value(ma, "adapter_name", DEFAULT_DEVICE_CLASS);
    u->sink_latency = 0;
    if (pa_modargs_get_value_u32(ma, "sink_latency", &u->sink_latency) < 0) {
        AUDIO_ERR_LOG("No sink_latency argument.");
    }

    u->deviceNetworkId = pa_modargs_get_value(ma, "network_id", DEFAULT_DEVICE_NETWORKID);

    if (pa_modargs_get_value_u32(ma, "render_in_idle_state", &u->render_in_idle_state) < 0) {
        AUDIO_ERR_LOG("Failed to parse render_in_idle_state  argument.");
        return -1;
    }

    if (pa_modargs_get_value_u32(ma, "open_mic_speaker", &u->open_mic_speaker) < 0) {
        AUDIO_ERR_LOG("Failed to parse open_mic_speaker argument.");
        return -1;
    }

    u->test_mode_on = false;
    if (pa_modargs_get_value_boolean(ma, "test_mode_on", &u->test_mode_on) < 0) {
        AUDIO_INFO_LOG("No test_mode_on arg. Normal mode it is.");
    }

    return 0;
}

static void InitStreamAvailable(struct Userdata *u)
{
    u->lastStreamAvailable = 0;
    u->streamAvailable = 0;
    u->streamAvailableMap = pa_hashmap_new_full(pa_idxset_string_hash_func, pa_idxset_string_compare_func,
        pa_xfree, pa_xfree);
    
    u->sceneToCountMap = pa_hashmap_new_full(pa_idxset_string_hash_func, pa_idxset_string_compare_func,
        pa_xfree, pa_xfree);
    char *sceneType = strdup("EFFECT_NONE");
    if (sceneType != NULL) {
        uint32_t *num = NULL;
        num = pa_xnew0(uint32_t, 1);
        *num = 1;
        if (pa_hashmap_put(u->sceneToCountMap, sceneType, num) != 0) {
            AUDIO_ERR_LOG("pa_hashmap_put failed");
            free(sceneType);
            pa_xfree(num);
        }
    }
    u->sceneToResamplerMap = pa_hashmap_new_full(pa_idxset_string_hash_func, pa_idxset_string_compare_func,
        pa_xfree, (pa_free_cb_t) pa_resampler_free);

    u->streamVolumeMap = pa_hashmap_new_full(pa_idxset_string_hash_func, pa_idxset_string_compare_func,
        pa_xfree, pa_xfree);
}

static int32_t PaHdiSinkNewInitUserDataAndSink(pa_module *m, pa_modargs *ma, const char *driver, struct Userdata *u)
{
    if (pa_modargs_get_value_boolean(ma, "offload_enable", &u->offload_enable) < 0) {
        AUDIO_ERR_LOG("Failed to parse offload_enable argument.");
        return -1;
    }

    if (pa_modargs_get_value_boolean(ma, "default_adapter_enable", &u->defaultAdapterEnable) < 0) {
        AUDIO_ERR_LOG("Failed to parse defaultAdapterEnable argument.");
        return -1;
    }

    pa_atomic_store(&u->primary.dflag, 0);
    u->primary.dq = pa_asyncmsgq_new(0);
    CHECK_AND_RETURN_RET_LOG(u->primary.dq, -1, "Failed to create u->primary.dq");

    u->sink = PaHdiSinkInit(u, ma, driver);
    CHECK_AND_RETURN_RET_LOG(u->sink, -1, "Failed to create sink object");

    u->render_full_enable = false; // default to false.
    if (u->ss.channels > CHANNEL_COUNT_2) {
        AUDIO_INFO_LOG("multichannel case, will call render_full for dp");
        u->render_full_enable = true;
    }

    u->sink->parent.process_msg = SinkProcessMsg;
    u->sink->set_state_in_io_thread = SinkSetStateInIoThreadCb;
    if (!u->fixed_latency) {
        u->sink->update_requested_latency = SinkUpdateRequestedLatencyCb;
    }
    u->sink->userdata = u;

    pa_sink_set_asyncmsgq(u->sink, u->thread_mq.inq);
    pa_sink_set_rtpoll(u->sink, u->rtpoll);

    u->bytes_dropped = 0;
    u->buffer_size = DEFAULT_BUFFER_SIZE;
    if (pa_modargs_get_value_u32(ma, "buffer_size", &u->buffer_size) < 0) {
        AUDIO_ERR_LOG("Failed to parse buffer_size argument.");
        return -1;
    }

    u->block_usec = pa_bytes_to_usec(u->buffer_size, &u->sink->sample_spec);

    if ((u->primary.sinkAdapter) && !strcmp(u->primary.sinkAdapter->deviceClass, DEVICE_CLASS_DP)) {
        u->primary.prewrite = u->block_usec * 2; // 2 frame, set cache len in hdi, avoid pop
    }

    u->lastRecodedLatency = 0;
    u->continuesGetLatencyErrCount = 0;

    if (u->fixed_latency) {
        pa_sink_set_fixed_latency(u->sink, u->block_usec);
    } else {
        pa_sink_set_latency_range(u->sink, 0, u->block_usec);
    }

    pa_sink_set_max_request(u->sink, u->buffer_size);

    InitStreamAvailable(u);
    return 0;
}

pa_sink *PaHdiSinkNew(pa_module *m, pa_modargs *ma, const char *driver)
{
    AUDIO_INFO_LOG("In");
    struct Userdata *u = NULL;
    char *hdiThreadName = NULL;
    char *hdiThreadNameMch = NULL;

    pa_assert(m);
    pa_assert(ma);

    u = pa_xnew0(struct Userdata, 1);
    pa_assert(u);

    if (PaHdiSinkNewInitUserData(m, ma, u) < 0) {
        goto fail;
    }

    if (PaHdiSinkNewInitUserDataAndSink(m, ma, driver, u) < 0) {
        goto fail;
    }

    int32_t ret = PaHdiSinkNewInitThread(m, ma, u);
    if (ret) {
        AUDIO_ERR_LOG("PaHdiSinkNewInitThread failed");
        goto fail;
    }

    if (u->test_mode_on) {
        u->writeCount = 0;
        u->renderCount = 0;
        hdiThreadName = "OS_WriteHdiTest";
        if (!(u->primary.threadHdi = pa_thread_new(hdiThreadName, TestModeThreadFuncWriteHDI, u))) {
            AUDIO_ERR_LOG("Failed to test-mode-write-hdi thread.");
            goto fail;
        }
    } else {
        if (!strcmp(u->sink->name, MCH_SINK_NAME)) {
            hdiThreadNameMch = "OS_WriteHdiMch";
            if (!(u->multiChannel.threadHdi = pa_thread_new(hdiThreadNameMch, ThreadFuncWriteHDIMultiChannel, u))) {
                AUDIO_ERR_LOG("Failed to write-hdi-multichannel thread.");
                goto fail;
            }
        } else if (!strcmp(u->sink->name, OFFLOAD_SINK_NAME)) {
            // offload not need write hdi thread
            u->offload.threadHdi = NULL;
        } else {
            hdiThreadName = "OS_WriteHdi";
            if (!(u->primary.threadHdi = pa_thread_new(hdiThreadName, ThreadFuncWriteHDI, u))) {
                AUDIO_ERR_LOG("Failed to write-hdi-primary2 thread.");
                goto fail;
            }
        }
    }

    u->primary.writeTime = DEFAULT_WRITE_TIME;
    u->multiChannel.writeTime = DEFAULT_WRITE_TIME;
    pa_sink_put(u->sink);

    return u->sink;
fail:
    AUDIO_ERR_LOG("PaHdiSinkNew failed, free userdata");
    UserdataFree(u);

    return NULL;
}

static void UserdataFreeOffload(struct Userdata *u)
{
    if (u->offload.dq) {
        pa_asyncmsgq_unref(u->offload.dq);
    }

    if (u->offload.sinkAdapter) {
        u->offload.sinkAdapter->SinkAdapterStop(u->offload.sinkAdapter);
        OffloadUnlock(u);
        u->offload.sinkAdapter->SinkAdapterDeInit(u->offload.sinkAdapter);
        AUDIO_INFO_LOG("DeInited Offload HDI renderer");
        ReleaseSinkAdapter(u->offload.sinkAdapter);
        u->offload.sinkAdapter = NULL;
    }

    if (u->offload.chunk.memblock) {
        pa_memblock_unref(u->offload.chunk.memblock);
    }

    pthread_mutex_destroy(&u->offload.lockCallback);
}

static void UserdataFreeMultiChannel(struct Userdata *u)
{
    AUDIO_DEBUG_LOG("UserdataFreeMultiChannel");
    if (u->multiChannel.dq) {
        pa_asyncmsgq_unref(u->multiChannel.dq);
    }

    if (u->multiChannel.sinkAdapter) {
        u->multiChannel.sinkAdapter->SinkAdapterStop(u->multiChannel.sinkAdapter);
        u->multiChannel.sinkAdapter->SinkAdapterDeInit(u->multiChannel.sinkAdapter);
        ReleaseSinkAdapter(u->multiChannel.sinkAdapter);
        u->multiChannel.sinkAdapter = NULL;
    }

    if (u->multiChannel.chunk.memblock) {
        pa_memblock_unref(u->multiChannel.chunk.memblock);
    }
}

static void UserdataFreeThread(struct Userdata *u)
{
    if (u->thread) {
        pa_asyncmsgq_send(u->thread_mq.inq, NULL, PA_MESSAGE_SHUTDOWN, NULL, 0, NULL);
        pa_thread_free(u->thread);
    }

    if (u->offload.threadHdi) {
        pa_asyncmsgq_post(u->offload.dq, NULL, QUIT, NULL, 0, NULL, NULL);
        pa_thread_free(u->offload.threadHdi);
    }

    if (u->multiChannel.threadHdi) {
        pa_asyncmsgq_post(u->multiChannel.dq, NULL, QUIT, NULL, 0, NULL, NULL);
        pa_thread_free(u->multiChannel.threadHdi);
    }

    if (u->primary.threadHdi) {
        pa_asyncmsgq_post(u->primary.dq, NULL, QUIT, NULL, 0, NULL, NULL);
        pa_thread_free(u->primary.threadHdi);
    }

    pa_thread_mq_done(&u->thread_mq);
}

static bool FreeBufferAttr(struct Userdata *u)
{
    // free heap allocated in userdata init
    if (u->bufferAttr == NULL) {
        pa_xfree(u);
        AUDIO_DEBUG_LOG("buffer attr is null, free done");
        return false;
    }
    FreeEffectBuffer(u);

    pa_xfree(u->bufferAttr);
    u->bufferAttr = NULL;
    return true;
}

static void FreeLimiter(struct Userdata *u)
{
    if (u->isLimiterCreated == true) {
        if (LimiterManagerRelease((int32_t)u->sink->index) == SUCCESS) {
            u->isLimiterCreated = false;
        } else {
            AUDIO_ERR_LOG("LimiterManagerRelease failed");
        }
    }
}

static void UserdataFree(struct Userdata *u)
{
    AUDIO_INFO_LOG("In");
    if (u == NULL) {
        AUDIO_INFO_LOG("Userdata is null, free done");
        return;
    }

    if (u->sink) {
        pa_sink_unlink(u->sink);
    }

    UserdataFreeThread(u);

    if (u->sink) {
        pa_sink_unref(u->sink);
    }

    if (u->memchunk.memblock) {
        pa_memblock_unref(u->memchunk.memblock);
    }

    if (u->rtpoll) {
        pa_rtpoll_free(u->rtpoll);
    }

    UserdataFreeOffload(u);
    UserdataFreeMultiChannel(u);

    if (u->primary.dq) {
        pa_asyncmsgq_unref(u->primary.dq);
    }

    if (u->primary.sinkAdapter) {
        u->primary.sinkAdapter->SinkAdapterStop(u->primary.sinkAdapter);
        u->primary.sinkAdapter->SinkAdapterDeInit(u->primary.sinkAdapter);
        ReleaseSinkAdapter(u->primary.sinkAdapter);
        u->primary.sinkAdapter = NULL;
    }

    if (u->sceneToCountMap) {
        pa_hashmap_free(u->sceneToCountMap);
    }

    if (u->sceneToResamplerMap) {
        pa_hashmap_free(u->sceneToResamplerMap);
    }

    if (u->streamAvailableMap) {
        pa_hashmap_free(u->streamAvailableMap);
    }

    if (u->streamVolumeMap) {
        pa_hashmap_free(u->streamVolumeMap);
    }

    if (!FreeBufferAttr(u)) {
        return;
    }
    // free limiter buffer
    FreeLimiter(u);

    pa_xfree(u);

    AUDIO_DEBUG_LOG("UserdataFree done");
}

void PaHdiSinkFree(pa_sink *s)
{
    AUTO_CTRACE("PaHdiSinkFree");
    AUDIO_INFO_LOG("In, PaHdiSinkFree, free userdata");
    struct Userdata *u = NULL;

    pa_sink_assert_ref(s);
    pa_assert_se(u = s->userdata);

    UserdataFree(u);
}