#include "ffmpeg_process.h"
#include "v4l2.h"
#include "audio.h"


static AVFrame *get_video_frame(OutputStream *ost);
static void fill_yuv_image(AVFrame *pict, int frame_index,int width, int height);
static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height);
static AVFrame *get_audio_frame(OutputStream *ost);
static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt);

static int write_frame(AVFormatContext *fmt_ctx, const AVRational *time_base, AVStream *st, AVPacket *pkt)
{
    /*将输出数据包时间戳值从编解码器重新调整为流时基 */
    av_packet_rescale_ts(pkt, *time_base, st->time_base);
    pkt->stream_index = st->index;
		
	/*将压缩的帧写入媒体文件*/
    return av_interleaved_write_frame(fmt_ctx, pkt);
}


/* 添加输出流。 */

void add_stream(OutputStream *ost, AVFormatContext *oc,
                       AVCodec **codec,
                       enum AVCodecID codec_id)
{
    AVCodecContext *c;


    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!(*codec)) {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        exit(1);
    }

    ost->st = avformat_new_stream(oc, NULL);
    if (!ost->st) {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    ost->st->id = oc->nb_streams-1;
    c = avcodec_alloc_context3(*codec);
    if (!c) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        exit(1);
    }
    ost->enc = c;

    switch ((*codec)->type) {
    case AVMEDIA_TYPE_AUDIO:
        c->sample_fmt  = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
        c->bit_rate    = 64000;  //设置码率
        c->sample_rate = 44100;  //音频采样率
        //c->channels= av_get_channel_layout_nb_channels(c->channel_layout);
        c->channel_layout = AV_CH_LAYOUT_STEREO;// AV_CH_LAYOUT_MONO 单声道   AV_CH_LAYOUT_STEREO 立体声
        c->channels        = av_get_channel_layout_nb_channels(c->channel_layout);
        ost->st->time_base = (AVRational){ 1, c->sample_rate };
        break;

    case AVMEDIA_TYPE_VIDEO:
        c->codec_id = codec_id;
		//码率：影响体积，与体积成正比：码率越大，体积越大；码率越小，体积越小。
        c->bit_rate = 400000; //设置码率 400kps
        /*分辨率必须是2的倍数。 */
        c->width    =VIDEO_WIDTH;
        c->height   = VIDEO_HEIGHT;
        /*时基：这是基本的时间单位（以秒为单位）
		 *表示其中的帧时间戳。 对于固定fps内容，
		 *时基应为1 / framerate，时间戳增量应为
		 *等于1。*/
        ost->st->time_base = (AVRational){1,STREAM_FRAME_RATE};
        c->time_base       = ost->st->time_base;
        c->gop_size      = 12; /* 最多每十二帧发射一帧内帧 */
        c->pix_fmt       = STREAM_PIX_FMT;
        c->max_b_frames = 0;  //不要B帧
        if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) 
		{
            c->mb_decision = 2;
        }
    break;

    default:
        break;
    }

    /* 某些格式希望流头分开。 */
    if (oc->oformat->flags & AVFMT_GLOBALHEADER)
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
}

/**************************************************************/
/* audio output */

static AVFrame *alloc_audio_frame(enum AVSampleFormat sample_fmt,
                                  uint64_t channel_layout,
                                  int sample_rate, int nb_samples)
{
    AVFrame *frame = av_frame_alloc();
    frame->format = sample_fmt;
    frame->channel_layout = channel_layout;
    frame->sample_rate = sample_rate;
    frame->nb_samples = nb_samples;
    if(nb_samples)
	{
        av_frame_get_buffer(frame, 0);
    }
    return frame;
}


void open_audio(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c;
    int nb_samples;

    AVDictionary *opt = NULL;
    c = ost->enc;
    av_dict_copy(&opt, opt_arg, 0);
    int ret = avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
	

    /*下面3行代码是为了生成虚拟的声音设置的频率参数*/
    ost->t     = 0;
    ost->tincr = 2 * M_PI * 110.0 / c->sample_rate;
    ost->tincr2 = 2 * M_PI * 110.0 / c->sample_rate / c->sample_rate;

	//AAC编码这里就固定为1024
    nb_samples = c->frame_size;

    ost->frame     = alloc_audio_frame(c->sample_fmt, c->channel_layout,
                                       c->sample_rate, nb_samples);
    ost->tmp_frame = alloc_audio_frame(AV_SAMPLE_FMT_S16, c->channel_layout,
                                       c->sample_rate, nb_samples);

    /* copy the stream parameters to the muxer */
    avcodec_parameters_from_context(ost->st->codecpar, c);

    /* create resampler context */
    ost->swr_ctx = swr_alloc();

	/* set options */
    printf("c->channels=%d\n",c->channels);
	av_opt_set_int       (ost->swr_ctx, "in_channel_count",   c->channels,       0);
	av_opt_set_int       (ost->swr_ctx, "in_sample_rate",     c->sample_rate,    0);
	av_opt_set_sample_fmt(ost->swr_ctx, "in_sample_fmt",      AV_SAMPLE_FMT_S16, 0);
	av_opt_set_int       (ost->swr_ctx, "out_channel_count",  c->channels,       0);
	av_opt_set_int       (ost->swr_ctx, "out_sample_rate",    c->sample_rate,    0);
	av_opt_set_sample_fmt(ost->swr_ctx, "out_sample_fmt",     c->sample_fmt,     0);
    
	/* initialize the resampling context */
	swr_init(ost->swr_ctx);
}

/* 毫秒级 延时 */
void Sleep(int ms)
{
	struct timeval delay;
	delay.tv_sec = 0;
	delay.tv_usec = ms * 1000; // 20 ms
	select(0, NULL, NULL, NULL, &delay);
}



/*
准备虚拟音频帧
这里可以替换成从声卡读取的PCM数据
*/


static AVFrame *get_audio_frame(OutputStream *ost)
{
    AVFrame *frame = ost->tmp_frame;
    int16_t *q = (int16_t*)frame->data[0];
    /* 检查我们是否要生成更多帧，用于判断是否结束*/
    if (av_compare_ts(ost->next_pts, ost->enc->time_base,STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

   #if 1
	//获取链表节点数量
	int cnt=0;
	while(cnt<=0)
	{
		cnt=List_GetNodeCnt(list_head);
	}
	
	pthread_mutex_lock(&mutex_audio); /*互斥锁上锁*/

	//得到节点数据
	struct AUDIO_DATA *tmp=list_head;
	unsigned char *buffer;

	tmp=tmp->next;
	if(tmp==NULL)
	{
		printf("数据为NULL.\n");
		exit(0);
	}
	buffer=tmp->audio_buffer;
	
	//1024*16*1
	memcpy(q,buffer,frame->nb_samples*sizeof(int16_t)*ost->enc->channels);//将音频数据拷贝进入frame缓冲区
	
	List_DelNode(list_head,buffer);
	free(buffer);			
    pthread_mutex_unlock(&mutex_audio); /*互斥锁解锁*/
	#endif
	
    frame->pts = ost->next_pts;
    ost->next_pts  += frame->nb_samples;
    return frame;
}


/*
 *编码一个音频帧并将其发送到多路复用器
 *编码完成后返回1，否则返回0
 */

int write_audio_frame(AVFormatContext *oc, OutputStream *ost)
{
    AVCodecContext *c;
    AVPacket pkt = { 0 };
    AVFrame *frame;
 
    int got_packet;
    int dst_nb_samples;

    av_init_packet(&pkt);
    c = ost->enc;

    frame = get_audio_frame(ost);

    if(frame)
	{
        /*使用重采样器将样本从本机格式转换为目标编解码器格式*/
		 /*计算样本的目标数量*/
		dst_nb_samples = av_rescale_rnd(swr_get_delay(ost->swr_ctx, c->sample_rate) + frame->nb_samples,
										c->sample_rate, c->sample_rate, AV_ROUND_UP);
		av_assert0(dst_nb_samples == frame->nb_samples);
        av_frame_make_writable(ost->frame);
        /*转换为目标格式 */
        swr_convert(ost->swr_ctx,
                    ost->frame->data, dst_nb_samples,
                    (const uint8_t **)frame->data, frame->nb_samples);
        frame = ost->frame;
        frame->pts = av_rescale_q(ost->samples_count, (AVRational){1, c->sample_rate}, c->time_base);
        ost->samples_count += dst_nb_samples;
    }

    avcodec_encode_audio2(c, &pkt, frame, &got_packet);

    if (got_packet) 
	{
        write_frame(oc, &c->time_base, ost->st, &pkt);
    }
    return (frame || got_packet) ? 0 : 1;
}


static AVFrame *alloc_picture(enum AVPixelFormat pix_fmt, int width, int height)
{
    AVFrame *picture;

    picture = av_frame_alloc();
    picture->format = pix_fmt;
    picture->width  = width;
    picture->height = height;

    /* allocate the buffers for the frame data */
    av_frame_get_buffer(picture, 32);
    return picture;
}


void open_video(AVFormatContext *oc, AVCodec *codec, OutputStream *ost, AVDictionary *opt_arg)
{
    AVCodecContext *c = ost->enc;
    AVDictionary *opt = NULL;
    av_dict_copy(&opt, opt_arg, 0);
    /* open the codec */
    avcodec_open2(c, codec, &opt);
    av_dict_free(&opt);
    /* allocate and init a re-usable frame */
    ost->frame = alloc_picture(c->pix_fmt, c->width, c->height);
    ost->tmp_frame = NULL;
    /* 将流参数复制到多路复用器 */
    avcodec_parameters_from_context(ost->st->codecpar, c);
}


/*
准备图像数据
YUV422占用内存空间 = w * h * 2
YUV420占用内存空间 = width*height*3/2
*/

static void fill_yuv_image(AVFrame *pict, int frame_index,int width, int height)
{
	int y_size=width*height;
	/*等待条件成立*/

    YUV420P_Buffer_temp = malloc(VIDEO_WIDTH*VIDEO_HEIGHT*3/2 * sizeof(unsigned char));

    pthread_mutex_lock(&video_mutex);
    pthread_cond_wait(&cond,&video_mutex);
    memcpy(YUV420P_Buffer_temp,YUV420P_Buffer,sizeof(YUV420P_Buffer));
    //将YUV数据拷贝到缓冲区  y_size=wXh
	memcpy(pict->data[0],YUV420P_Buffer_temp,y_size);
	memcpy(pict->data[1],YUV420P_Buffer_temp+y_size,y_size/4);
	memcpy(pict->data[2],YUV420P_Buffer_temp+y_size+y_size/4,y_size/4);
    /*互斥锁解锁*/
	pthread_mutex_unlock(&video_mutex);   
    free(YUV420P_Buffer_temp);

}


static AVFrame *get_video_frame(OutputStream *ost)
{
    AVCodecContext *c = ost->enc;

    /* 检查我们是否要生成更多帧---判断是否结束录制 */
    if(av_compare_ts(ost->next_pts, c->time_base,STREAM_DURATION, (AVRational){ 1, 1 }) >= 0)
        return NULL;

    /*当我们将帧传递给编码器时，它可能会保留对它的引用
    *内部； 确保我们在这里不覆盖它*/
    if (av_frame_make_writable(ost->frame) < 0)
        exit(1);

	//制作虚拟图像
	//DTS（解码时间戳）和PTS（显示时间戳）
    fill_yuv_image(ost->frame, ost->next_pts, c->width, c->height);
    
    ost->frame->pts = ost->next_pts++;
    return ost->frame;
}

/*
*编码一个视频帧并将其发送到多路复用器
*编码完成后返回1，否则返回0
*/

int write_video_frame(AVFormatContext *oc, OutputStream *ost)
{
    int ret;
    AVCodecContext *c;
    AVFrame *frame;
    int got_packet = 0;
    AVPacket pkt = { 0 };
    c=ost->enc;
	//获取一帧数据
    frame = get_video_frame(ost);
    av_init_packet(&pkt);

    /* 编码图像 */
    ret=avcodec_encode_video2(c, &pkt, frame, &got_packet);

    if(got_packet) 
	{
        ret=write_frame(oc, &c->time_base, ost->st, &pkt);
    }
	else
    {
        ret = 0;
    }
    return (frame || got_packet) ? 0 : 1;
}


void close_stream(AVFormatContext *oc, OutputStream *ost)
{
    avcodec_free_context(&ost->enc);
    av_frame_free(&ost->frame);
    av_frame_free(&ost->tmp_frame);
    sws_freeContext(ost->sws_ctx);
    swr_free(&ost->swr_ctx);
}




