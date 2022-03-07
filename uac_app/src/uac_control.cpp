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
#include "uac_control_mpi.h"
#include "uac_control_graph.h"
#include "uac_control.h"

typedef struct _UacControl {
    UACControl *uacControl;
} UacControl;

static UacControl *gUAControl = NULL;
int uac_control_create(int type) {
    if (!gUAControl) {
        uac_control_destory();
    }

    gUAControl = (UacControl*)calloc(1, sizeof(UacControl));
    if (!gUAControl) {
        ALOGE("fail to malloc memory!\n");
        return -1;
    }

    memset(gUAControl, 0, sizeof(UacControl));
    if (type == UAC_API_MPI) {
        gUAControl->uacControl = new UACControlMpi();
    } else if (type == UAC_API_GRAPH) {
        gUAControl->uacControl = new UACControlGraph();
    }

    return 0;
}

void uac_control_destory() {
    if (gUAControl == NULL)
        return;

    if (gUAControl->uacControl) {
        delete gUAControl->uacControl;
    }
    gUAControl = NULL;
}

UACControl* getControlContext() {
    return gUAControl->uacControl;
}

int uac_start(int type) {
    UACControl *uacControl = getControlContext();
    return uacControl->uacStart(type);
}

void uac_stop(int type) {
    UACControl *uacControl = getControlContext();
    uacControl->uacStop(type);
}

void uac_set_sample_rate(int type, int samplerate) {
    UACControl *uacControl = getControlContext();
    uacControl->uacSetSampleRate(type, samplerate);
}

void uac_set_volume(int type, int volume) {
    UACControl *uacControl = getControlContext();
    uacControl->uacSetVolume(type, volume);
}

void uac_set_mute(int type, int mute) {
    UACControl *uacControl = getControlContext();
    uacControl->uacSetMute(type, mute);
}

void uac_set_ppm(int type, int ppm) {
    UACControl *uacControl = getControlContext();
    uacControl->uacSetPpm(type, ppm);
}
