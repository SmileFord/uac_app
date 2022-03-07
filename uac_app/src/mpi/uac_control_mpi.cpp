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

#include <rockit/rt_header.h>
#include <rockit/mpi/rk_debug.h>
#include <rockit/mpi/rk_mpi_sys.h>
#include <rockit/mpi/rk_mpi_ao.h>
#include <rockit/mpi/rk_mpi_ai.h>
#include <rockit/mpi/rk_comm_aio.h>

#include "uac_log.h"
#include "uac_control_mpi.h"
#include "uac_common_def.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "uac_mpi"
#endif // LOG_TAG

#define UAC_MPI_ENABLE (1 << 1)

typedef struct _UacControlMpi {
    int type;
    int flag;
    UacAudioConfig  config;
    pthread_mutex_t mutex;
} UacControlMpi;

UacControlMpi* getContextMpi(void* context) {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(context);
    return ctx;
}

UACControlMpi::UACControlMpi() {
    UacControlMpi *ctx= (UacControlMpi*)calloc(1, sizeof(UacControlMpi));
    memset(ctx, 0, sizeof(UacControlMpi));
    ALOGD("UACControlMpi-----------------\n");
    ctx->config.samplerate = 48000;
    ctx->config.volume = 1.0;
    RK_MPI_SYS_Init();
    mCtx = reinterpret_cast<void *>(ctx);
}

UACControlMpi::~UACControlMpi() {
    UacControlMpi* ctx = getContextMpi(mCtx);
    if (ctx) {
        free(ctx);
    }
    RK_MPI_SYS_Exit();
    mCtx = RT_NULL;
}

void UACControlMpi::uacSetSampleRate(int type, int sampleRate) {
    ALOGD("type = %d, sampleRate = %d\n", type, sampleRate);
    UacControlMpi* ctx = getContextMpi(mCtx);
    ctx->config.samplerate = sampleRate;
}

void UACControlMpi::uacSetVolume(int type, int volume) {
    ALOGD("type = %d, volume = %d\n", type, volume);
    UacControlMpi* ctx = getContextMpi(mCtx);
    ctx->config.volume = ((float)volume/100.0);
}

void UACControlMpi::uacSetMute(int type, int mute) {
    ALOGD("type = %d, mute = %d\n", type, mute);
    UacControlMpi* ctx = getContextMpi(mCtx);
    ctx->config.mute = mute;
}

void UACControlMpi::uacSetPpm(int type, int ppm) {
    ALOGD("type = %d, ppm = %d\n", type, ppm);
    UacControlMpi* ctx = getContextMpi(mCtx);
    ctx->config.ppm = ppm;
}

int UACControlMpi::uacStart(int type) {
    uac_stop(type);
    ALOGD("type = %d\n", type);
    int ret = 0;
    UacControlMpi* ctx = getContextMpi(mCtx);
    ctx->type = type;

    ret = startAi();
    if (ret != 0) {
        goto __FAILED;
    }

    ret = startAo();
    if (ret != 0) {
        goto __FAILED;
    }

    AiBindAo();
    ctx->flag |= UAC_MPI_ENABLE;
    return 0;

__FAILED:
    return -1;
}

void UACControlMpi::uacStop(int type) {
    ALOGD("type = %d\n", type);
    UacControlMpi* ctx = getContextMpi(mCtx);
    if ((ctx->flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE){
       AiUnBindAo();
       stopAi();
       stopAo();
       ctx->flag &= (~UAC_MPI_ENABLE);
    }
}

int UACControlMpi::startAi() {
    AUDIO_DEV aiDevId = 0;
    AI_CHN AiChn = 0;
    RK_S32 result;
    AIO_ATTR_S aiAttr;
    memset(&aiAttr, 0, sizeof(AIO_ATTR_S));
    ALOGD("startAi\n");
    snprintf(reinterpret_cast<char *>(aiAttr.u8CardName),
                 sizeof(aiAttr.u8CardName), "%s", "default:CARD=rockchiprk809co");
    aiAttr.soundCard.channels = 2;
    aiAttr.soundCard.sampleRate = 44100;
    aiAttr.soundCard.bitWidth = AUDIO_BIT_WIDTH_16;

    aiAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    aiAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)44100;
    aiAttr.enSoundmode = AUDIO_SOUND_MODE_STEREO;

    aiAttr.u32FrmNum = 4;
    aiAttr.u32PtNumPerFrm = 1024;
    aiAttr.u32EXFlag = 0;
    aiAttr.u32ChnCnt = 2;
    result = RK_MPI_AI_SetPubAttr(aiDevId, &aiAttr);
    if (result != 0) {
        RK_LOGE("ai set attr fail, reason = %d", result);
        goto __FAILED;
    }

    result = RK_MPI_AI_Enable(aiDevId);
    if (result != 0) {
        RK_LOGE("ai enable fail, reason = %d", result);
        goto __FAILED;
    }

    result = RK_MPI_AI_EnableChn(aiDevId, AiChn);
    if (result != 0) {
        RK_LOGE("ai enable channel fail, Chn = %d, reason = %x", AiChn, result);
        return RK_FAILURE;
    }

    result = RK_MPI_AI_EnableReSmp(aiDevId, AiChn, (AUDIO_SAMPLE_RATE_E)44100);
    if (result != 0) {
        RK_LOGE("ai enable channel fail, Chn = %d, reason = %x", AiChn, result);
        return RK_FAILURE;
    }

#if 0
    AI_CHN_PARAM_S params;
    memset(&params, 0, sizeof(AI_CHN_PARAM_S));
    params.u32Ppm = -2;
    RK_MPI_AI_SetChnParam(aiDevId, AiChn, &params);
#endif
    return 0;
__FAILED:
    return -1;
}

int UACControlMpi::startAo() {
    AUDIO_DEV aoDevId = 0;
    AO_CHN AoChn = 0;
    RK_S32 result;
    AIO_ATTR_S aoAttr;
    memset(&aoAttr, 0, sizeof(AIO_ATTR_S));
    ALOGD("startAo\n");
    snprintf(reinterpret_cast<char *>(aoAttr.u8CardName),
                 sizeof(aoAttr.u8CardName), "%s", "hw:1,0");
    aoAttr.soundCard.channels = 2;
    aoAttr.soundCard.sampleRate = 44100;
    aoAttr.soundCard.bitWidth = AUDIO_BIT_WIDTH_16;

    aoAttr.enBitwidth = AUDIO_BIT_WIDTH_16;
    aoAttr.enSamplerate = (AUDIO_SAMPLE_RATE_E)44100;
    aoAttr.enSoundmode = AUDIO_SOUND_MODE_STEREO;

    aoAttr.u32FrmNum = 4;
    aoAttr.u32PtNumPerFrm = 1024;
    aoAttr.u32EXFlag = 0;
    aoAttr.u32ChnCnt = 2;
    result = RK_MPI_AO_SetPubAttr(aoDevId, &aoAttr);
    if (result != 0) {
        RK_LOGE("ai set attr fail, reason = %d", result);
        goto __FAILED;
    }

    result = RK_MPI_AO_Enable(aoDevId);
    if (result != 0) {
        RK_LOGE("ai enable fail, reason = %d", result);
        goto __FAILED;
    }

    result = RK_MPI_AO_EnableChn(aoDevId, AoChn);
    if (result != 0) {
        RK_LOGE("ai enable channel fail, Chn = %d, reason = %x", AoChn, result);
        return RK_FAILURE;
    }

    result = RK_MPI_AO_EnableReSmp(aoDevId, AoChn, (AUDIO_SAMPLE_RATE_E)44100);
    if (result != 0) {
        RK_LOGE("ai enable channel fail, Chn = %d, reason = %x", AoChn, result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

void UACControlMpi::AiBindAo() {
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = 0;
    RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
}

int UACControlMpi::stopAi() {
    AUDIO_DEV aiDevId = 0;
    AI_CHN AiChn = 0;

    RK_MPI_AI_DisableReSmp(aiDevId, AiChn);
    RK_S32 result = RK_MPI_AI_DisableChn(aiDevId, AiChn);
    if (result != 0) {
        RK_LOGE("ai disable channel fail, reason = %d", result);
        return RK_FAILURE;
    }

    result =  RK_MPI_AI_Disable(aiDevId);
    if (result != 0) {
        RK_LOGE("ai disable fail, reason = %d", result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

int UACControlMpi::stopAo() {
    AUDIO_DEV aoDevId = 0;
    AO_CHN AiChn = 0;

    RK_MPI_AO_DisableReSmp(aoDevId, AiChn);
    RK_S32 result = RK_MPI_AO_DisableChn(aoDevId, AiChn);
    if (result != 0) {
        RK_LOGE("ao disable channel fail, reason = %d", result);
        return RK_FAILURE;
    }

    result =  RK_MPI_AO_Disable(aoDevId);
    if (result != 0) {
        RK_LOGE("ao disable fail, reason = %d", result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

void UACControlMpi::AiUnBindAo() {
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = 0;
    stSrcChn.s32ChnId = 0;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = 0;
    stDstChn.s32ChnId = 0;
    RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
}

