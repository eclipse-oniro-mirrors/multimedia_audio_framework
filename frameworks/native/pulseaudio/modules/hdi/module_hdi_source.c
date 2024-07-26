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
#undef LOG_TAG
#define LOG_TAG "ModuleHdiSource"

#include <config.h>
#include <pulsecore/log.h>
#include <pulsecore/modargs.h>
#include <pulsecore/module.h>
#include <pulsecore/source.h>
#include <stddef.h>
#include <stdbool.h>

#include <pulsecore/core.h>
#include <pulsecore/core-util.h>
#include <pulsecore/namereg.h>

#include "securec.h"
#include "audio_log.h"
#include "audio_enhance_chain_adapter.h"
#include "userdata.h"

pa_source *PaHdiSourceNew(pa_module *m, pa_modargs *ma, const char *driver);
void PaHdiSourceFree(pa_source *s);

PA_MODULE_AUTHOR("OpenHarmony");
PA_MODULE_DESCRIPTION("OpenHarmony HDI Source");
PA_MODULE_VERSION(PACKAGE_VERSION);
PA_MODULE_LOAD_ONCE(false);
PA_MODULE_USAGE(
        "source_name=<name for the source> "
        "device_class=<name for the device class> "
        "source_properties=<properties for the source> "
        "format=<sample format> "
        "rate=<sample rate> "
        "channels=<number of channels> "
        "channel_map=<channel map>"
        "buffer_size=<custom buffer size>"
        "file_path=<file path for data reading>"
        "adapter_name=<primary>"
        "open_mic_speaker<open mic>"
        "network_id<device network id>"
        "device_type<device type or port>"
        "source_type<source type or port>"
    );

static const char * const VALID_MODARGS[] = {
    "source_name",
    "device_class",
    "source_properties",
    "format",
    "rate",
    "channels",
    "channel_map",
    "buffer_size",
    "file_path",
    "adapter_name",
    "open_mic_speaker",
    "network_id",
    "device_type",
    "source_type",
    "ec_type",
    "ec_adapter",
    "ec_sampling_rate",
    "ec_format",
    "ec_channels",
    "open_mic_ref",
    "mic_ref_rate",
    "mic_ref_format",
    "mic_ref_channels",
    NULL
};

static void IncreaseScenekeyCount(pa_hashmap *sceneMap, const char *key)
{
    if (sceneMap == NULL) {
        return;
    }
    char *sceneKey;
    uint32_t *num = NULL;
    if ((num = (uint32_t *)pa_hashmap_get(sceneMap, key)) != NULL) {
        (*num)++;
    } else {
        sceneKey = strdup(key);
        num = pa_xnew0(uint32_t, 1);
        *num = 1;
        pa_hashmap_put(sceneMap, sceneKey, num);
    }
}


static bool DecreaseScenekeyCount(pa_hashmap *sceneMap, const char *key)
{
    if (sceneMap == NULL) {
        return false;
    }
    uint32_t *num = NULL;
    if ((num = (uint32_t *)pa_hashmap_get(sceneMap, key)) != NULL) {
        (*num)--;
        if (*num == 0) {
            pa_hashmap_remove_and_free(sceneMap, key);
            return true;
        }
    }
    return false;
}

static void SetResampler(pa_source_output *so, const pa_sample_spec *algoConfig,
    const char *sceneKey, pa_hashmap *resamplerMap)
{
    AUDIO_INFO_LOG("SOURCE rate = %{public}d ALGO rate = %{public}d ",
        so->source->sample_spec.rate, algoConfig->rate);
    if (!pa_sample_spec_equal(&so->source->sample_spec, algoConfig)) {
        pa_resampler *preResampler = pa_resampler_new(so->source->core->mempool,
            &so->source->sample_spec, &so->source->channel_map,
            algoConfig, &so->source->channel_map,
            so->source->core->lfe_crossover_freq,
            PA_RESAMPLER_AUTO,
            PA_RESAMPLER_VARIABLE_RATE);
        pa_hashmap_put(resamplerMap, pa_xstrdup(sceneKey), preResampler);
        pa_resampler_set_input_rate(so->thread_info.resampler, algoConfig->rate);
    }
}

static pa_hook_result_t HandleSourceOutputPut(pa_source_output *so, struct Userdata *u)
{
    const char *sceneType = pa_proplist_gets(so->proplist, "scene.type");
    uint32_t captureId = u->captureId;
    uint32_t renderId = u->renderId;
    uint32_t sceneTypeCode = 0;
    if (GetSceneTypeCode(sceneType, &sceneTypeCode) != 0) {
        AUDIO_ERR_LOG("GetSceneTypeCode failed");
        return PA_HOOK_OK;
    }
    uint32_t sceneKeyCode = 0;
    sceneKeyCode = (sceneTypeCode << SCENE_TYPE_OFFSET) + (captureId << CAPTURER_ID_OFFSET) + renderId;
    if (EnhanceChainManagerCreateCb(sceneKeyCode) != 0) {
        AUDIO_INFO_LOG("Create EnhanceChain failed, set to bypass");
        pa_proplist_sets(so->proplist, "scene.bypass", DEFAULT_SCENE_BYPASS);
        return PA_HOOK_OK;
    }
    EnhanceChainManagerInitEnhanceBuffer();
    char sceneKey[MAX_SCENE_NAME_LEN];
    if (sprintf_s(sceneKey, sizeof(sceneKey), "%u", sceneKeyCode) < 0) {
        AUDIO_ERR_LOG("sprintf from sceneKeyCode to sceneKey failed");
        return PA_HOOK_OK;
    }
    IncreaseScenekeyCount(u->sceneToCountMap, sceneKey);
    pa_sample_spec algoConfig;
    pa_sample_spec_init(&algoConfig);
    if (EnhanceChainManagerGetAlgoConfig(sceneKeyCode, &algoConfig) != 0) {
        AUDIO_ERR_LOG("Get algo config failed");
        return PA_HOOK_OK;
    }
    SetResampler(so, &algoConfig, sceneKey, u->sceneToResamplerMap);
    return PA_HOOK_OK;
}

static pa_hook_result_t HandleSourceOutputUnlink(pa_source_output *so, struct Userdata *u)
{
    const char *sceneType = pa_proplist_gets(so->proplist, "scene.type");
    uint32_t captureId = u->captureId;
    uint32_t renderId = u->renderId;
    uint32_t sceneTypeCode = 0;
    if (GetSceneTypeCode(sceneType, &sceneTypeCode) != 0) {
        AUDIO_ERR_LOG("GetSceneTypeCode failed");
        return PA_HOOK_OK;
    }
    uint32_t sceneKeyCode = 0;
    sceneKeyCode = (sceneTypeCode << SCENE_TYPE_OFFSET) + (captureId << CAPTURER_ID_OFFSET) + renderId;
    EnhanceChainManagerReleaseCb(sceneKeyCode);
    
    char sceneKey[MAX_SCENE_NAME_LEN];
    if (sprintf_s(sceneKey, sizeof(sceneKey), "%u", sceneKeyCode) < 0) {
        AUDIO_ERR_LOG("sprintf from sceneKeyCode to sceneKey failed");
        return PA_HOOK_OK;
    }
    if (DecreaseScenekeyCount(u->sceneToCountMap, sceneKey)) {
        pa_hashmap_remove_and_free(u->sceneToResamplerMap, sceneKey);
    }
    return PA_HOOK_OK;
}

static pa_hook_result_t CheckIfAvailSource(pa_source_output *so, struct Userdata *u)
{
    pa_source *soSource = so->source;
    pa_source *thisSource = u->source;
    if (soSource == NULL || thisSource == NULL) {
        return PA_HOOK_CANCEL;
    }
    if (soSource->index != thisSource->index) {
        AUDIO_INFO_LOG("NOT correspondant SOURCE %{public}s AND %{public}s.", soSource->name, thisSource->name);
        return PA_HOOK_CANCEL;
    }
    return PA_HOOK_OK;
}

static pa_hook_result_t SourceOutputPutCb(pa_core *c, pa_source_output *so, struct Userdata *u)
{
    AUDIO_INFO_LOG("Trigger SourceOutputPutCb");
    if (u == NULL) {
        AUDIO_ERR_LOG("Get Userdata failed! userdata is NULL");
        return PA_HOOK_OK;
    }
    pa_assert(c);
    if (CheckIfAvailSource(so, u) == PA_HOOK_CANCEL) {
        return PA_HOOK_OK;
    }
    return HandleSourceOutputPut(so, u);
}

static pa_hook_result_t SourceOutputUnlinkCb(pa_core *c, pa_source_output *so, struct Userdata *u)
{
    AUDIO_INFO_LOG("Trigger SourceOutputUnlinkCb");
    if (u == NULL) {
        AUDIO_ERR_LOG("Get Userdata failed! userdata is NULL");
        return PA_HOOK_OK;
    }
    pa_assert(c);
    if (CheckIfAvailSource(so, u) == PA_HOOK_CANCEL) {
        return PA_HOOK_OK;
    }
    return HandleSourceOutputUnlink(so, u);
}

static pa_hook_result_t SourceOutputMoveFinishCb(pa_core *c, pa_source_output *so, struct Userdata *u)
{
    AUDIO_INFO_LOG("Trigger SourceOutputMoveFinishCb");
    if (u == NULL) {
        AUDIO_ERR_LOG("Get Userdata failed! userdata is NULL");
        return PA_HOOK_OK;
    }
    pa_assert(c);
    if (CheckIfAvailSource(so, u) == PA_HOOK_CANCEL) {
        return PA_HOOK_OK;
    }
    return HandleSourceOutputPut(so, u);
}

int pa__init(pa_module *m)
{
    pa_modargs *ma = NULL;

    pa_assert(m);

    if (!(ma = pa_modargs_new(m->argument, VALID_MODARGS))) {
        pa_log("Failed to parse module arguments");
        goto fail;
    }

    if (!(m->userdata = PaHdiSourceNew(m, ma, __FILE__))) {
        goto fail;
    }
    pa_source *source = (pa_source *)m->userdata;

    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_PUT], PA_HOOK_LATE,
        (pa_hook_cb_t)SourceOutputPutCb, source->userdata);
    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_UNLINK], PA_HOOK_LATE,
        (pa_hook_cb_t)SourceOutputUnlinkCb, source->userdata);
    pa_module_hook_connect(m, &m->core->hooks[PA_CORE_HOOK_SOURCE_OUTPUT_MOVE_FINISH], PA_HOOK_LATE,
        (pa_hook_cb_t)SourceOutputMoveFinishCb, source->userdata);

    pa_modargs_free(ma);

    return 0;

fail:

    if (ma) {
        pa_modargs_free(ma);
    }

    pa__done(m);

    return -1;
}

int pa__get_n_used(pa_module *m)
{
    pa_source *source = NULL;

    pa_assert(m);
    pa_assert_se(source = m->userdata);

    return pa_source_linked_by(source);
}

static void ReleaseAllChains(struct Userdata *u)
{
    void *state = NULL;
    uint32_t *sceneKeyNum;
    const void *sceneKey;
    while ((sceneKeyNum = pa_hashmap_iterate(u->sceneToCountMap, &state, &sceneKey))) {
        uint32_t sceneKeyCode = (uint32_t)strtoul((char *)sceneKey, NULL, BASE_TEN);
        for (uint32_t count = 0; count < *sceneKeyNum; count++) {
            EnhanceChainManagerReleaseCb(sceneKeyCode);
        }
    }
}

void pa__done(pa_module *m)
{
    pa_source *source = NULL;

    pa_assert(m);

    if ((source = m->userdata)) {
        struct Userdata *u = (struct Userdata *)source->userdata;
        if (u != NULL) {
            AUDIO_INFO_LOG("Release all enhChains on [%{public}s]", source->name);
            ReleaseAllChains(u);
        }
        PaHdiSourceFree(source);
    }
}
