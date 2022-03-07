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
#ifndef SRC_INCLUDE_UAC_CONTROL_H_
#define SRC_INCLUDE_UAC_CONTROL_H_

#include "uac_common_def.h"

enum UacApiType {
    UAC_API_MPI   = 0,
    UAC_API_GRAPH = 1,
    UAC_API_MAX
};

class UACControl {
 public:
    UACControl() {}
    virtual ~UACControl() {}

 public:
    virtual int uacStart(int type) = 0;
    virtual void uacStop(int type) = 0; 
    virtual void uacSetSampleRate(int type, int sampleRate) = 0;
    virtual void uacSetVolume(int type, int volume) = 0;
    virtual void uacSetMute(int type, int mute) = 0;
    virtual void uacSetPpm(int type, int ppm) = 0;
};

int uac_start(int type);
void uac_stop(int type);
void uac_set_sample_rate(int type, int samplerate);
void uac_set_volume(int type, int volume);
void uac_set_mute(int type, int mute);
void uac_set_ppm(int type, int ppm);

int uac_control_create(int type);
void uac_control_destory();

#endif  // SRC_INCLUDE_UAC_CONTROL_H_
