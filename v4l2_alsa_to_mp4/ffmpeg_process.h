#ifndef FFMOEG_PROCESS_H
#define FFMOEG_PROCESS_H

#include "common.h"
#include "list.h"
#include <libavutil/avassert.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libavutil/mathematics.h>
#include <libavutil/timestamp.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>

#define STREAM_DURATION   10.0   /*录制10秒的视频，由于缓冲的原因，一般只有8秒*/
#define STREAM_FRAME_RATE 15     /* 15 images/s   avfilter_get_by_name */
#define STREAM_PIX_FMT    AV_PIX_FMT_YUV420P /* default pix_fmt */
#define SCALE_FLAGS SWS_BICUBIC


// 单个输出AVStream的包装器
typedef struct OutputStream {
    AVStream *st;
    AVCodecContext *enc;

    /* 下一帧的点数*/
    int64_t next_pts;
    int samples_count;

    AVFrame *frame;
    AVFrame *tmp_frame;

    float t, tincr, tincr2;

    struct SwsContext *sws_ctx;
    struct SwrContext *swr_ctx;
} OutputStream;


void close_stream(AVFormatContext *oc, OutputStream *ost);
int write_video_frame(AVFormatContext *oc, OutputStream *ost);
void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
int write_audio_frame(AVFormatContext *oc, OutputStream *ost);
void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg);
void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id);


void Sleep(int ms);

#endif