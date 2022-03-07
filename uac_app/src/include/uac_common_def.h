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
#ifndef SRC_INCLUDE_UAC_COMMON_DEF_H_
#define SRC_INCLUDE_UAC_COMMON_DEF_H_

#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <pwd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <unistd.h>
#include <sys/ioctl.h>

enum UacStreamType {
    // our device record datas from usb, pc/remote->our device
    UAC_STREAM_RECORD = 0,
    // play datas to usb, our device->pc/remote
    UAC_STREAM_PLAYBACK,
    UAC_STREAM_MAX
};

typedef struct _UacAudioConfig {
    int samplerate;
    float volume;
    int mute;
    int ppm;
} UacAudioConfig;

#endif  // SRC_INCLUDE_UAC_COMMON_DEF_H_
