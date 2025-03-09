#include "thread_task.h"



//主线程：编码视频和音频
int video_audio_encode(char *filename)
{

    OutputStream video_st = { 0 }, audio_st = { 0 };
    AVOutputFormat *fmt;
    AVFormatContext *oc;
    AVCodec *audio_codec, *video_codec;
    int ret;
    int have_video = 0, have_audio = 0;
    int encode_video = 0, encode_audio = 0;
    AVDictionary *opt = NULL;

	//获取本地时间
	t=time(NULL);
	t=t+8*60*60; //+上8个小时
	tme=gmtime(&t);
	sprintf(filename,"%d-%d-%d-%d-%d-%d.mp4",tme->tm_year+1900,tme->tm_mon+1,tme->tm_mday,tme->tm_hour,tme->tm_min,tme->tm_sec);
	printf("视频名称:%s\n",filename);

    /* 分配输出环境 */
    avformat_alloc_output_context2(&oc,NULL,NULL,filename);
    fmt=oc->oformat;
	
     /*使用默认格式的编解码器添加音频和视频流，初始化编解码器。 */
    if(fmt->video_codec != AV_CODEC_ID_NONE)
	{
        add_stream(&video_st,oc,&video_codec,fmt->video_codec);
        have_video = 1;
        encode_video = 1;
    }
    if(fmt->audio_codec != AV_CODEC_ID_NONE)
	{
        add_stream(&audio_st, oc, &audio_codec, fmt->audio_codec);
        have_audio = 1;
        encode_audio = 1;
    }

  /*现在已经设置了所有参数，可以打开音频视频编解码器，并分配必要的编码缓冲区。 */
    if (have_video)
        open_video(oc, video_codec, &video_st, opt);
		printf("open_video\n");

    if (have_audio)
        open_audio(oc, audio_codec, &audio_st, opt);
		printf("open_audio\n");

    av_dump_format(oc, 0, filename, 1);

    /* 打开输出文件（如果需要） */
    if(!(fmt->flags & AVFMT_NOFILE)) 
	{
        ret = avio_open(&oc->pb, filename, AVIO_FLAG_WRITE);
        if (ret < 0)
		{
            fprintf(stderr, "无法打开输出文件: '%s': %s\n", filename,av_err2str(ret));
            return 1;
        }
    }

    /* 编写流头（如果有）*/
    avformat_write_header(oc,&opt);

    while(encode_video || encode_audio)
	{
        /* 选择要编码的流*/
        if(encode_video &&(!encode_audio || av_compare_ts(video_st.next_pts, video_st.enc->time_base,audio_st.next_pts, audio_st.enc->time_base) <= 0))
        {
			printf("视频编码一次----->\n");
            encode_video = !write_video_frame(oc,&video_st);
        }
		else 
		{
			printf("音频编码一次----->\n");
            encode_audio = !write_audio_frame(oc,&audio_st);
        }
    }
	
    av_write_trailer(oc);
	printf("av_write_trailer----->\n");
    if (have_video)
        close_stream(oc, &video_st);
		printf("close_stream_video----->\n");
    if (have_audio)
        close_stream(oc, &audio_st);
		printf("close_stream_audio----->\n");

    if (!(fmt->flags & AVFMT_NOFILE)){
        avio_closep(&oc->pb);
    }
        
	free(filename);
    avformat_free_context(oc);
    return 0;
}


/*
子线程函数: 采集摄像头的数据
*/
void *pthread_read_video_data(void *arg)
{
    pthread_detach(pthread_self());
    /*初始化摄像头设备*/
    int err;
	err=VideoDeviceInit("dev/video0");
	printf("VideoDeviceInit=%d\n",err);
	if(err!=0)return err;
	/*1. 循环读取摄像头采集的数据*/
	struct pollfd fds;
	fds.fd=video_fd;
	fds.events=POLLIN;

	/*2. 申请存放数据空间*/
	struct v4l2_buffer video_buffer;
    memset(&video_buffer, 0, sizeof(struct v4l2_buffer));
	while(1)
	{
        /*(1)等待摄像头采集数据*/
        poll(&fds,1,-1);
        /*(2)取出队列里采集完毕的缓冲区*/
        video_buffer.type=V4L2_BUF_TYPE_VIDEO_CAPTURE; /*视频捕获设备*/
        video_buffer.memory=V4L2_MEMORY_MMAP;
        ioctl(video_fd,VIDIOC_DQBUF,&video_buffer);
        /*(3)处理图像数据*/
        /*YUYV数据转YUV420P*/
        YUV420P_Buffer = malloc(VIDEO_WIDTH*VIDEO_HEIGHT*3/2 * sizeof(unsigned char));

        pthread_mutex_lock(&video_mutex);   /*互斥锁上锁*/
        yuyv_to_yuv420p(image_buffer[video_buffer.index],YUV420P_Buffer,VIDEO_WIDTH,VIDEO_HEIGHT);

        pthread_mutex_unlock(&video_mutex); /*互斥锁解锁*/
        pthread_cond_broadcast(&cond);/*广播方式唤醒休眠的线程*/
		 		 /*(4)将缓冲区再放入队列*/
        ioctl(video_fd,VIDIOC_QBUF,&video_buffer);

        free(YUV420P_Buffer);

	}	
}

/*
子线程函数: 采集摄像头的数据
*/
void *pthread_read_audio_data(void *arg)
{
    pthread_detach(pthread_self());
    capture_audio_data(capture_handle,buffer_frames);
}
