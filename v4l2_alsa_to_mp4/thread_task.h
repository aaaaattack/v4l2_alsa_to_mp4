#ifndef THREAD_TASK_H
#define THREAD_TASK_H

#include "common.h"
#include "v4l2.h"
#include "audio.h"
#include "ffmpeg_process.h"
#include "list.h"
time_t t;
struct tm *tme;
char filename[100];

int video_audio_encode(char *filename);
void *pthread_read_video_data(void *arg);
void *pthread_read_audio_data(void *arg);
#endif