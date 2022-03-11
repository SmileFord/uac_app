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
#ifndef SRC_MPI_MPI_CONTROL_H_
#define SRC_MPI_MPI_CONTROL_H_

#include "uac_common_def.h"
#include <rk_type.h>
#include <rk_debug.h>
#include <rk_mpi_sys.h>
#include <rk_mpi_ao.h>
#include <rk_mpi_ai.h>
#include <rk_comm_aio.h>

typedef enum _UacMpiType {
    UAC_MPI_TYPE_AI = 0,
    UAC_MPI_TYPE_AO = 1,
    UAC_MPI_TYPE_MAX
} UacMpiType;

typedef struct _UacMpiIdConfig {
    AUDIO_DEV aoDevId;
    AUDIO_DEV aiDevId;
    AO_CHN    aoChnId;
    AI_CHN    aiChnId;
} UacMpiIdConfig;

typedef struct _UacMpiStream {
    int flag;
    UacMpiIdConfig idCfg;
    UacAudioConfig config;
} UacMpiStream;

class UacMpiUtil {
 public:
    static const char* getSndCardName(UacMpiType type, int mode);
    static RK_U32 getSndCardChannels(UacMpiType type, int mode);
    static RK_U32 getSndCardSampleRate(UacMpiType type, int mode);
    static AUDIO_BIT_WIDTH_E getSndCardbitWidth(UacMpiType type, int mode);
    static AUDIO_SAMPLE_RATE_E getDataSamplerate(UacMpiType type, int mode);
    static AUDIO_BIT_WIDTH_E getDataBitwidth(UacMpiType type, int mode);
    static AUDIO_SOUND_MODE_E getDataSoundmode(UacMpiType type, int mode);
};

void mpi_set_samplerate(int type, UacMpiStream& streamCfg);
void mpi_set_volume(int type, UacMpiStream& streamCfg);
void mpi_set_ppm(int type, UacMpiStream& streamCfg);

#endif  // SRC_MPI_MPI_CONTROL_H_

