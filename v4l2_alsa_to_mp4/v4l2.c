#include "v4l2.h"



/*
函数功能: 摄像头设备初始化
*/
int VideoDeviceInit(char *DEVICE_NAME)
{
	/*1. 打开摄像头设备*/
	video_fd=open(DEVICE_NAME,O_RDWR);
	if(video_fd<0)return -1;

    // 查询摄像头能力
    struct v4l2_capability capability;
    if(0 == ioctl(video_fd, VIDIOC_QUERYCAP, &capability)){
        if((capability.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0){
            perror("该摄像头设备不支持视频采集！");
            close(video_fd);
            return -2;
        }
        if((capability.capabilities & V4L2_MEMORY_MMAP) == 0){
            perror("该摄像头设备不支持mmap内存映射！");
            close(video_fd);
            return -3;
        }
    }

    // 枚举摄像头格式
    struct v4l2_fmtdesc fmtdesc;
    memset(&fmtdesc, 0, sizeof(fmtdesc));
    fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  //设置视频采集设备类型
    int i = 0;
    while(1){
        fmtdesc.index = i++;
        // 获取支持格式
        if(0 == ioctl(video_fd, VIDIOC_ENUM_FMT, &fmtdesc)){
            printf("支持格式：%s, %c%c%c%c\n", fmtdesc.description,
                                            fmtdesc.pixelformat & 0xff,
                                            fmtdesc.pixelformat >> 8 & 0xff,
                                            fmtdesc.pixelformat >> 16 & 0xff,
                                            fmtdesc.pixelformat >> 24 & 0xff);
            // 列出该格式下支持的分辨率             VIDIOC_ENUM_FRAMESIZES & 默认帧率 VIDIOC_G_PARM
            // 1.默认帧率
            struct v4l2_streamparm streamparm;
            streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            if(0 == ioctl(video_fd, VIDIOC_G_PARM, &streamparm))
                printf("该格式默认帧率 %d fps\n", streamparm.parm.capture.timeperframe.denominator);
            // 2.循环列出支持的分辨率
            struct v4l2_frmsizeenum frmsizeenum;
            frmsizeenum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            frmsizeenum.pixel_format = fmtdesc.pixelformat;   //设置成对应的格式
            int j = 0;
            printf("支持的分辨率有：\n");
            while(1){
                frmsizeenum.index = j++;
                if(0 == ioctl(video_fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum))
                    printf("%d x %d\n", frmsizeenum.discrete.width, frmsizeenum.discrete.height);
                else break;
            }
            printf("\n");
        }else break;
    }

    /* 5.设置摄像头类型为捕获、设置分辨率、视频采集格式 (VIDIOC_S_FMT) */
    struct v4l2_format video_formt;
    memset(&video_formt,0,sizeof(struct v4l2_format));
    video_formt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;   /* 视频采集 */
    video_formt.fmt.pix.width = VIDEO_HEIGHT;          /* 宽 */
    video_formt.fmt.pix.height = VIDEO_WIDTH;    	 /* 高 */
    video_formt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;   /* 设置输出类型：MJPG */
    if(0 > ioctl(video_fd, VIDIOC_S_FMT, &video_formt)){
        perror("设置摄像头参数失败！");
        close(video_fd);
        return -4;
    }
    printf("当前摄像头尺寸:width*height=%d*%d\n",video_formt.fmt.pix.width,video_formt.fmt.pix.height);

	
	
    /* 6.向内核申请内存 (VIDIOC_REQBUFS：个数、映射方式为mmap)
         将申请到的缓存加入内核队列 (VIDIOC_QBUF)
         将内核内存映射到用户空间 (mmap) */
    struct v4l2_requestbuffers video_requestbuffers;
    memset(&video_requestbuffers,0,sizeof(struct v4l2_requestbuffers));	
    video_requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    video_requestbuffers.count = 4;    //申请缓存个数
    video_requestbuffers.memory = V4L2_MEMORY_MMAP;     //申请为物理连续的内存空间
    if(0 == ioctl(video_fd, VIDIOC_REQBUFS, &video_requestbuffers)){
        /* 申请到内存后 */
        for(__u32 i = 0; i < video_requestbuffers.count; i++){
            /* 将申请到的缓存加入内核队列 (VIDIOC_QBUF)              */
            struct v4l2_buffer video_buffer;
            memset(&video_buffer,0,sizeof(struct v4l2_buffer));
            video_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            video_buffer.index = i;
            video_buffer.memory = V4L2_MEMORY_MMAP;
            if(0 == ioctl(video_fd, VIDIOC_QBUF, &video_buffer)){
                /* 加入内核队列成功后，将内存映射到用户空间 (mmap) */
                image_buffer[i] = (unsigned char *)mmap(NULL, video_buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, video_fd, video_buffer.m.offset);

            }
        }
    }
    else{
        perror("申请内存失败！");
        close(video_fd);
        return -5;
    }

	/*6. 启动采集队列*/
	int opt=V4L2_BUF_TYPE_VIDEO_CAPTURE; /*视频捕获设备*/
	if(ioctl(video_fd,VIDIOC_STREAMON,&opt))return -6;

	printf("VIDEO初始化成功\n");
	return 0;
}



//YUYV==YUV422
int yuyv_to_yuv420p(const unsigned char *in, unsigned char *out, unsigned int width, unsigned int height)
{
	//     // 打印 out 的地址
    // printf("Address of in: %p\n", (void *)in);

    // // 打印 out 指向的第一个字节的值
    // printf("Value at out[0]: %u\n", out[0]); // 确保 out 已分配并初始化
    unsigned char *y = out;
    unsigned char *u = out + width*height;
    unsigned char *v = out + width*height + width*height/4;
    unsigned int i,j;
    unsigned int base_h;
    unsigned int  is_u = 1;
    unsigned int y_index = 0, u_index = 0, v_index = 0;
    unsigned long yuv422_length = 2 * width * height;
    //序列为YU YV YU YV，一个yuv422帧的长度 width * height * 2 个字节
    //丢弃偶数行 u v
    for(i=0; i<yuv422_length; i+=2)
    {
        *(y+y_index) = *(in+i);
        y_index++;
		printf("yuv422_leyuv422_lengthyuv422_lengthyuv422_lengthyuv422_lengthyuv422_lengthyuv422_lengthyuv422_lengthngth++\n");
    }
    for(i=0; i<height; i+=2)
    {
        base_h = i*width*2;
        for(j=base_h+1; j<base_h+width*2; j+=2)
        {
            if(is_u)
            {
				*(u+u_index) = *(in+j);
				u_index++;
				is_u = 0;
				printf("yuv422_uindex++\n");
            }
            else
            {
                *(v+v_index) = *(in+j);
                v_index++;
                is_u = 1;
				printf("yuv422_vindex++\n");
            }
        }
    }
    return 1;
}