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
#include "mpi_control.h"
#include "uac_control_mpi.h"

#ifdef LOG_TAG
#undef LOG_TAG
#define LOG_TAG "uac_mpi"
#endif // LOG_TAG

#define UAC_MPI_ENABLE (1 << 1)

typedef enum _UacMpiAODevId {
    AO_USB_DEV,    // ao[0]
    AO_SPK_DEV,    // ao[1]
} UacMpiAODevId;

typedef enum _UacMpiAIDevId {
    AI_MIC_DEV,    // ai[0]
    AI_USB_DEV,    // ai[1]
} UacMpiAIDevId;

typedef struct _UACControlGraph {
    int mode;
    UacMpiStream stream;
} UacControlMpi;

void mpi_sys_init() {
    RK_MPI_SYS_Init();
}

void mpi_sys_destrory() {
    RK_MPI_SYS_Exit();
}

UacControlMpi* getContextMpi(void* context) {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(context);
    return ctx;
}

UACControlMpi::UACControlMpi(int mode) {
    UacControlMpi *ctx= (UacControlMpi*)calloc(1, sizeof(UacControlMpi));
    memset(ctx, 0, sizeof(UacControlMpi));

    ctx->mode = mode;
    ctx->stream.config.intVol = 100;
    ctx->stream.config.mute = 0;
    ctx->stream.config.ppm = 0;

    if (mode == UAC_STREAM_PLAYBACK) {
        ctx->stream.idCfg.aiDevId = (AUDIO_DEV)AI_MIC_DEV;
        ctx->stream.idCfg.aoDevId = (AUDIO_DEV)AO_USB_DEV;
        ctx->stream.config.samplerate = UacMpiUtil::getDataSamplerate(UAC_MPI_TYPE_AO, ctx->mode);
    } else if (mode == UAC_STREAM_RECORD) {
        ctx->stream.idCfg.aiDevId = (AUDIO_DEV)AI_USB_DEV;
        ctx->stream.idCfg.aoDevId = (AUDIO_DEV)AO_SPK_DEV;
        ctx->stream.config.samplerate = UacMpiUtil::getDataSamplerate(UAC_MPI_TYPE_AI, ctx->mode);
    }

    mCtx = reinterpret_cast<void *>(ctx);
}

UACControlMpi::~UACControlMpi() {
    UacControlMpi* ctx = getContextMpi(mCtx);
    if (ctx) {
        free(ctx);
    }

    mCtx = NULL;
}

void UACControlMpi::uacSetSampleRate(int sampleRate) {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ctx->stream.config.samplerate = sampleRate;
    ALOGD("mode = %d, sampleRate = %d\n", ctx->mode, sampleRate);
    if ((ctx->stream.flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE) {
        mpi_set_samplerate(ctx->mode, ctx->stream);
    }
}

void UACControlMpi::uacSetVolume(int volume) {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ALOGD("mode = %d, volume = %d\n", ctx->mode, volume);
    ctx->stream.config.intVol = volume;
    if ((ctx->stream.flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE) {
        mpi_set_volume(ctx->mode, ctx->stream);
    }
}

void UACControlMpi::uacSetMute(int mute) {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ALOGD("mode = %d, mute = %d\n", ctx->mode, mute);
    ctx->stream.config.mute = mute;
    if ((ctx->stream.flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE) {
        mpi_set_volume(ctx->mode, ctx->stream);
    }
}

void UACControlMpi::uacSetPpm(int ppm) {
    ALOGD("ppm = %d\n", ppm);
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ctx->stream.config.ppm = ppm;
    if ((ctx->stream.flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE) {
        mpi_set_ppm(ctx->mode, ctx->stream);
    }
}

int UACControlMpi::uacStart() {
    uacStop();
    int ret = 0;
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ret = startAi();
    if (ret != 0) {
        goto __FAILED;
    }

    ret = startAo();
    if (ret != 0) {
        goto __FAILED;
    }

    mpi_set_samplerate(ctx->mode, ctx->stream);
    mpi_set_volume(ctx->mode, ctx->stream);
    mpi_set_ppm(ctx->mode, ctx->stream);

    AiBindAo();
    ctx->stream.flag |= UAC_MPI_ENABLE;
    return 0;

__FAILED:
    return -1;
}

void UACControlMpi::uacStop() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    ALOGD("stop mode = %d, flag = %d\n", ctx->mode, ctx->stream.flag);
    if ((ctx->stream.flag &= UAC_MPI_ENABLE) == UAC_MPI_ENABLE) {
       AiUnBindAo();
       stopAi();
       stopAo();
       ctx->stream.flag &= (~UAC_MPI_ENABLE);
    }
}

int UACControlMpi::startAi() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    AUDIO_DEV aiDevId = ctx->stream.idCfg.aiDevId;
    AI_CHN aiChn = ctx->stream.idCfg.aiChnId;
    ALOGD("this:%p, startAi(dev:%d, chn:%d), mode : %d\n", this, aiDevId, aiChn, ctx->mode);
    const char *cardName = RK_NULL;
    RK_S32 result = 0;
    AUDIO_SAMPLE_RATE_E rate;
    AIO_ATTR_S aiAttr;
    memset(&aiAttr, 0, sizeof(AIO_ATTR_S));

    cardName = UacMpiUtil::getSndCardName(UAC_MPI_TYPE_AI, ctx->mode);
    snprintf(reinterpret_cast<char *>(aiAttr.u8CardName), sizeof(aiAttr.u8CardName), "%s", cardName);
    aiAttr.soundCard.channels = UacMpiUtil::getSndCardChannels(UAC_MPI_TYPE_AI, ctx->mode);
    aiAttr.soundCard.sampleRate = UacMpiUtil::getSndCardSampleRate(UAC_MPI_TYPE_AI, ctx->mode);
    aiAttr.soundCard.bitWidth = UacMpiUtil::getSndCardbitWidth(UAC_MPI_TYPE_AI, ctx->mode);

    /*
     * 1. if datas are sended from pc to uac device, the ai device is usb,
     *    we set the samplerate which uevent report.
     * 2. if datas are sended from uac device to pc, the ai device is mic,
          we use the a fix samplerate like 16000.
     */
    if (ctx->mode == UAC_STREAM_PLAYBACK) {
        rate = (AUDIO_SAMPLE_RATE_E)UacMpiUtil::getSndCardSampleRate(UAC_MPI_TYPE_AI, ctx->mode);
    } else {
        rate = (AUDIO_SAMPLE_RATE_E)ctx->stream.config.samplerate;
    }

    aiAttr.enBitwidth = UacMpiUtil::getDataBitwidth(UAC_MPI_TYPE_AI, ctx->mode);
    aiAttr.enSamplerate = rate;
    aiAttr.enSoundmode = UacMpiUtil::getDataSoundmode(UAC_MPI_TYPE_AI, ctx->mode);
    ALOGD("this:%p, startAi(dev:%d, chn:%d), enSamplerate : %d\n", this, aiDevId, aiChn, aiAttr.enSamplerate);
    aiAttr.u32FrmNum = 4;
    aiAttr.u32PtNumPerFrm = 1024;
    aiAttr.u32EXFlag = 0;
    aiAttr.u32ChnCnt = 2;
    result = RK_MPI_AI_SetPubAttr(aiDevId, &aiAttr);
    if (result != 0) {
        RK_LOGE("ai set attr(dev:%d) fail, reason = %x", aiDevId, result);
        goto __FAILED;
    }

    result = RK_MPI_AI_Enable(aiDevId);
    if (result != 0) {
        RK_LOGE("ai enable(dev:%d) fail, reason = %x", aiDevId, result);
        goto __FAILED;
    }

    result = RK_MPI_AI_EnableChn(aiDevId, aiChn);
    if (result != 0) {
        RK_LOGE("ai enable channel(dev:%d, chn:%d) fail, reason = %x", aiDevId, aiChn, result);
        return RK_FAILURE;
    }

    // disable resample in ai, this means the samplerate of output of ai is the samplerate of sound card
    result = RK_MPI_AI_DisableReSmp(aiDevId, aiChn);
    if (result != 0) {
        RK_LOGE("ai disable resample(dev:%d, chn:%d) fail, reason = %x", aiDevId, aiChn, result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

int UACControlMpi::startAo() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    AUDIO_DEV aoDevId = ctx->stream.idCfg.aoDevId;
    AO_CHN aoChn = ctx->stream.idCfg.aoChnId;
    ALOGD("this:%p, startAo(dev:%d, chn:%d), mode : %d\n", this, aoDevId, aoChn, ctx->mode);
    const char *cardName = RK_NULL;
    RK_S32 result;
    AIO_ATTR_S aoAttr;
    memset(&aoAttr, 0, sizeof(AIO_ATTR_S));

    cardName = UacMpiUtil::getSndCardName(UAC_MPI_TYPE_AO, ctx->mode);
    snprintf(reinterpret_cast<char *>(aoAttr.u8CardName), sizeof(aoAttr.u8CardName), "%s", cardName);

    /*
     * 1. if datas is from pc to uac device, the ao device is spk,
          we use the a fix samplerate like 48000.
     * 2. if datas is from uac device to pc, the ao device is usb,
     *    we set the samplerate which uevent report.
     */
    AUDIO_SAMPLE_RATE_E rate;
    if (ctx->mode == UAC_STREAM_RECORD) {
        rate = (AUDIO_SAMPLE_RATE_E)UacMpiUtil::getSndCardSampleRate(UAC_MPI_TYPE_AO, ctx->mode);
    } else {
        rate = (AUDIO_SAMPLE_RATE_E)ctx->stream.config.samplerate;
    }

    aoAttr.soundCard.channels = UacMpiUtil::getSndCardChannels(UAC_MPI_TYPE_AO, ctx->mode);
    aoAttr.soundCard.sampleRate = rate;
    aoAttr.soundCard.bitWidth = UacMpiUtil::getSndCardbitWidth(UAC_MPI_TYPE_AO, ctx->mode);

    aoAttr.enBitwidth = UacMpiUtil::getDataBitwidth(UAC_MPI_TYPE_AO, ctx->mode);
    aoAttr.enSamplerate = rate;
    aoAttr.enSoundmode = UacMpiUtil::getDataSoundmode(UAC_MPI_TYPE_AO, ctx->mode);
    ALOGD("this:%p, startAo(dev:%d, chn:%d), mode : %d, enSamplerate = %d\n", 
        this, aoDevId, aoChn, ctx->mode, aoAttr.enSamplerate);
    aoAttr.u32FrmNum = 4;
    aoAttr.u32PtNumPerFrm = 1024;
    aoAttr.u32EXFlag = 0;
    aoAttr.u32ChnCnt = 2;
    result = RK_MPI_AO_SetPubAttr(aoDevId, &aoAttr);
    if (result != 0) {
        RK_LOGE("ai set attr(dev:%d) fail, reason = %x", aoDevId, result);
        goto __FAILED;
    }

    result = RK_MPI_AO_Enable(aoDevId);
    if (result != 0) {
        RK_LOGE("ai enable(dev:%d) fail, reason = %x", aoDevId, result);
        goto __FAILED;
    }

    result = RK_MPI_AO_EnableChn(aoDevId, aoChn);
    if (result != 0) {
        RK_LOGE("ao enable channel(dev:%d, chn:%d) fail, reason = %x", aoDevId, aoChn, result);
        return RK_FAILURE;
    }

    result = RK_MPI_AO_EnableReSmp(aoDevId, aoChn, aoAttr.enSamplerate);
    if (result != 0) {
        RK_LOGE("ao enable resample(dev:%d, chn:%d) fail, reason = %x", aoDevId, aoChn, result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

void UACControlMpi::AiBindAo() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = ctx->stream.idCfg.aiDevId;
    stSrcChn.s32ChnId = ctx->stream.idCfg.aiChnId;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = ctx->stream.idCfg.aoDevId;
    stDstChn.s32ChnId = ctx->stream.idCfg.aoChnId;
    RK_MPI_SYS_Bind(&stSrcChn, &stDstChn);
}

int UACControlMpi::stopAi() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    AUDIO_DEV aiDevId = ctx->stream.idCfg.aiDevId;
    AI_CHN aiChn = ctx->stream.idCfg.aiChnId;
    ALOGD("this:%p, stopAi(dev:%d, chn:%d), mode : %d\n", this, aiDevId, aiChn, ctx->mode);
    RK_MPI_AI_DisableReSmp(aiDevId, aiChn);
    RK_S32 result = RK_MPI_AI_DisableChn(aiDevId, aiChn);
    if (result != 0) {
        RK_LOGE("ai disable channel(dev:%d, chn:%d) fail, reason = %x", aiDevId, aiChn, result);
        return RK_FAILURE;
    }

    result =  RK_MPI_AI_Disable(aiDevId);
    if (result != 0) {
        RK_LOGE("ai disable(dev:%d) fail, reason = %x", aiDevId, result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

int UACControlMpi::stopAo() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    AUDIO_DEV aoDevId = ctx->stream.idCfg.aoDevId;
    AO_CHN aoChn = ctx->stream.idCfg.aoChnId;
    ALOGD("this:%p, stopAo(dev:%d, chn:%d), mode : %d\n", this, aoDevId, aoChn, ctx->mode);
    RK_MPI_AO_DisableReSmp(aoDevId, aoChn);
    RK_S32 result = RK_MPI_AO_DisableChn(aoDevId, aoChn);
    if (result != 0) {
        RK_LOGE("ao disable channel(dev:%d, chn:%d) fail, reason = %x", aoDevId, aoChn, result);
        return RK_FAILURE;
    }

    result =  RK_MPI_AO_Disable(aoDevId);
    if (result != 0) {
        RK_LOGE("ao disable(dev:%d) fail, reason = %x", aoDevId, result);
        return RK_FAILURE;
    }

    return 0;
__FAILED:
    return -1;
}

void UACControlMpi::AiUnBindAo() {
    UacControlMpi* ctx = reinterpret_cast<UacControlMpi *>(mCtx);
    MPP_CHN_S stSrcChn, stDstChn;
    stSrcChn.enModId = RK_ID_AI;
    stSrcChn.s32DevId = ctx->stream.idCfg.aiDevId;
    stSrcChn.s32ChnId = ctx->stream.idCfg.aiChnId;

    stDstChn.enModId = RK_ID_AO;
    stDstChn.s32DevId = ctx->stream.idCfg.aoDevId;
    stDstChn.s32ChnId = ctx->stream.idCfg.aoChnId;
    RK_MPI_SYS_UnBind(&stSrcChn, &stDstChn);
}

