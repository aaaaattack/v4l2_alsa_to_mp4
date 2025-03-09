#ifndef V4L2_H
#define V4L2_H

#include "common.h"

/*一些摄像头需要使用的全局变量*/
const unsigned char * image_buffer[4];
int video_fd;
pthread_mutex_t video_mutex;
pthread_cond_t cond;

//固定摄像头输出画面的尺寸
#define VIDEO_WIDTH  640
#define VIDEO_HEIGHT 480
//存放从摄像头读出转换之后的数据
unsigned char *YUV420P_Buffer;
unsigned char *YUV420P_Buffer_temp;

int VideoDeviceInit(char *DEVICE_NAME);
int yuyv_to_yuv420p(const unsigned char *in, unsigned char *out, unsigned int width, unsigned int height);

#endif