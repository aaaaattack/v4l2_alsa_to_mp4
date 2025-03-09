#ifndef AUDIO_H
#define AUDIO_H


#include "list.h"

unsigned char audio_read_buff[2048];

/*一些audio需要使用的全局变量*/
pthread_mutex_t mutex_audio;

#define AudioFormat         SND_PCM_FORMAT_S16_LE  //指定音频的格式,其他常用格式：SND_PCM_FORMAT_U24_LE、SND_PCM_FORMAT_U32_LE
#define AUDIO_CHANNEL_SET   2  			  //1单声道   2立体声
#define AUDIO_RATE_SET      44100   //音频采样率,常用的采样频率: 44100Hz 、16000HZ、8000HZ、48000HZ、22050HZ


struct AUDIO_DATA *list_head;
int buffer_frames;
snd_pcm_t *capture_handle;
snd_pcm_format_t format;
int capture_audio_data_init( char *audio_dev);
void capture_audio_data(snd_pcm_t *capture_handle,int buffer_frames);

#endif