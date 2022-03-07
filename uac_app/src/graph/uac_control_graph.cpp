/*
 * Copyright 2020 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "uac_log.h"
#include "graph_control.h"
#include "uac_control_graph.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "uac_graph"
#endif // LOG_TAG

/*
 * pc datas -> rv1109
 * usb record->xxxx process->speaker playback
 */
#define UAC_USB_RECORD_SPK_PLAY_CONFIG_FILE "/oem/usr/share/uac_app/usb_recode_speaker_playback.json"

/*
 * rv1109 datas -> pc
 * mic record->>xxxx process->usb playback
 */
#define UAC_MIC_RECORD_USB_PLAY_CONFIG_FILE "/oem/usr/share/uac_app/mic_recode_usb_playback.json"

typedef struct _UacStream {
    pthread_mutex_t mutex;
    UacAudioConfig  config;
    RTUACGraph     *uac;
} UacStream;

typedef struct _UACControlGraph {
    UacStream     stream[UAC_STREAM_MAX];
} UacControlGraph;

UacControlGraph* getContextGraph(void* context) {
    UacControlGraph* ctx = reinterpret_cast<UacControlGraph *>(context);
    return ctx;
}

UACControlGraph::UACControlGraph() {
    UacControlGraph *ctx= (UacControlGraph*)calloc(1, sizeof(UacControlGraph));
    for (int i = 0; i < UAC_STREAM_MAX; i++) {
        pthread_mutex_init(&ctx->stream[i].mutex, NULL);
        ctx->stream[i].uac = NULL;

        if (UAC_STREAM_PLAYBACK == i) {
            ctx->stream[i].config.samplerate = 48000;
        } else {
            ctx->stream[i].config.samplerate = 48000;
        }

        ctx->stream[i].config.volume = 1.0;
        ctx->stream[i].config.mute = 0;
        ctx->stream[i].config.ppm = 0;
    }

    mCtx = reinterpret_cast<void *>(ctx);
}

UACControlGraph::~UACControlGraph() {
    UacControlGraph* ctx = getContextGraph(mCtx);
    if (ctx) {
        for (int i = 0; i < UAC_STREAM_MAX; i++) {
            if (ctx->stream[i].uac != NULL) {
                delete(ctx->stream[i].uac);
                ctx->stream[i].uac = NULL;
            }
            pthread_mutex_destroy(&ctx->stream[i].mutex);
        }
        free(ctx);
        mCtx = RT_NULL;
    }
}

void UACControlGraph::uacSetSampleRate(int type, int sampleRate) {
    ALOGD("type = %d, samplerate = %d\n", type, sampleRate);
    UacControlGraph* ctx = getContextGraph(mCtx);
    pthread_mutex_lock(&ctx->stream[type].mutex);
    ctx->stream[type].config.samplerate = sampleRate;
    RTUACGraph* uac = ctx->stream[type].uac;
    if (uac != NULL) {
        graph_set_samplerate(uac, type, ctx->stream[type].config);
    }
    pthread_mutex_unlock(&ctx->stream[type].mutex);
}

void UACControlGraph::uacSetVolume(int type, int volume) {
    ALOGD("type = %d, volume = %d\n", type, volume);
    UacControlGraph* ctx = getContextGraph(mCtx);
    pthread_mutex_lock(&ctx->stream[type].mutex);
    ctx->stream[type].config.volume = ((float)volume/100.0);
    RTUACGraph* uac = ctx->stream[type].uac;
    if (uac != NULL) {
        graph_set_volume(uac, type, ctx->stream[type].config);
    }
    pthread_mutex_unlock(&ctx->stream[type].mutex);
}

void UACControlGraph::uacSetMute(int type, int mute) {
    ALOGD("type = %d, mute = %d\n", type, mute);
    UacControlGraph* ctx = getContextGraph(mCtx);
    pthread_mutex_lock(&ctx->stream[type].mutex);
    ctx->stream[type].config.mute = mute;
    RTUACGraph* uac = ctx->stream[type].uac;
    if (uac != NULL) {
        graph_set_volume(uac, type, ctx->stream[type].config);
    }
    pthread_mutex_unlock(&ctx->stream[type].mutex);
}

void UACControlGraph::uacSetPpm(int type, int ppm) {
    ALOGD("type = %d, ppm = %d\n", type, ppm);
    UacControlGraph* ctx = getContextGraph(mCtx);
    pthread_mutex_lock(&ctx->stream[type].mutex);
    ctx->stream[type].config.mute = ppm;
    RTUACGraph* uac = ctx->stream[type].uac;
    if (uac != NULL) {
        graph_set_ppm(uac, type, ctx->stream[type].config);
    }
    pthread_mutex_unlock(&ctx->stream[type].mutex);
}

int UACControlGraph::uacStart(int type) {
    UacControlGraph* ctx = getContextGraph(mCtx);

    uacStop(type);

    char* config = (char*)UAC_MIC_RECORD_USB_PLAY_CONFIG_FILE;
    char* name = (char*)"uac_playback";
    if (type == UAC_STREAM_RECORD) {
        name = (char*)"uac_record";
        config = (char*)UAC_USB_RECORD_SPK_PLAY_CONFIG_FILE;
    }
    ALOGD("config = %s\n", config);
    RTUACGraph* uac = new RTUACGraph(name);
    if (uac == NULL) {
        ALOGE("error, malloc fail\n");
        return -1;
    }

    // default configs will be readed in json file
    uac->autoBuild(config);
    uac->prepare();
    graph_set_volume(uac, type, ctx->stream[type].config);
    graph_set_samplerate(uac, type, ctx->stream[type].config);
    graph_set_ppm(uac, type, ctx->stream[type].config);
    uac->start();

    pthread_mutex_lock(&ctx->stream[type].mutex);
    ctx->stream[type].uac = uac;
    pthread_mutex_unlock(&ctx->stream[type].mutex);

    return 0;
}

void UACControlGraph::uacStop(int type) {
    UacControlGraph* ctx = getContextGraph(mCtx);
    ALOGD("type = %d\n", type); 
    pthread_mutex_lock(&ctx->stream[type].mutex);
    RTUACGraph *uac = ctx->stream[type].uac;
    ctx->stream[type].uac = NULL;
    pthread_mutex_unlock(&ctx->stream[type].mutex);

    if (uac != NULL) {
        uac->stop();
        uac->waitUntilDone();
        delete uac;
    }
}
