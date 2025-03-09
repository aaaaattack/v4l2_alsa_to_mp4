#include "audio.h"



int capture_audio_data_init( char *audio_dev)
{
	int err;
	
	buffer_frames = 1024;
	unsigned int rate = AUDIO_RATE_SET;// 常用的采样频率: 44100Hz 、16000HZ、8000HZ、48000HZ、22050HZ
	snd_pcm_hw_params_t *hw_params; //此结构包含有关硬件的信息，可用于指定PCM流的配置
	
	/*注册信号捕获退出接口*/
	printf("进入audioinit\n");
	/*PCM的采样格式在pcm.h文件里有定义*/
	format=SND_PCM_FORMAT_S16_LE; // 采样位数：16bit、LE格式
	
	/*打开音频采集卡硬件，并判断硬件是否打开成功，若打开失败则打印出错误提示*/
	if ((err = snd_pcm_open (&capture_handle, audio_dev,SND_PCM_STREAM_CAPTURE,0))<0) 
	{
		printf("无法打开音频设备: %s (%s)\n",  audio_dev,snd_strerror (err));
		exit(1);
	}
	printf("音频接口打开成功.\n");
	
 
	/*分配硬件参数结构对象，并判断是否分配成功*/
	if((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) 
	{
		printf("无法分配硬件参数结构 (%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("硬件参数结构已分配成功.\n");
	
	/*按照默认设置对硬件对象进行设置，并判断是否设置成功*/
	if((err=snd_pcm_hw_params_any(capture_handle,hw_params)) < 0) 
	{
		printf("无法初始化硬件参数结构 (%s)\n", snd_strerror(err));
		exit(1);
	}
	printf("硬件参数结构初始化成功.\n");
 
	/*
		设置数据为交叉模式，并判断是否设置成功
		interleaved/non interleaved:交叉/非交叉模式。
		表示在多声道数据传输的过程中是采样交叉的模式还是非交叉的模式。
		对多声道数据，如果采样交叉模式，使用一块buffer即可，其中各声道的数据交叉传输；
		如果使用非交叉模式，需要为各声道分别分配一个buffer，各声道数据分别传输。
	*/
	if((err = snd_pcm_hw_params_set_access (capture_handle,hw_params,SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) 
	{
		printf("无法设置访问类型(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("访问类型设置成功.\n");
 
	/*设置数据编码格式，并判断是否设置成功*/
	if ((err=snd_pcm_hw_params_set_format(capture_handle, hw_params,format)) < 0) 
	{
		printf("无法设置格式 (%s)\n",snd_strerror(err));
		exit(1);
	}
	fprintf(stdout, "PCM数据格式设置成功.\n");
 
	/*设置采样频率，并判断是否设置成功*/
	if((err=snd_pcm_hw_params_set_rate_near (capture_handle,hw_params,&rate,0))<0) 
	{
		printf("无法设置采样率(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("采样率设置成功\n");
 
	/*设置声道，并判断是否设置成功*/
	if((err = snd_pcm_hw_params_set_channels(capture_handle, hw_params,AUDIO_CHANNEL_SET)) < 0) 
	{
		printf("无法设置声道数(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("声道数设置成功.\n");
 
	/*将配置写入驱动程序中，并判断是否配置成功*/
	if ((err=snd_pcm_hw_params (capture_handle,hw_params))<0) 
	{
		printf("无法向驱动程序设置参数(%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("参数设置成功.\n");
	/*使采集卡处于空闲状态*/
	snd_pcm_hw_params_free(hw_params);
 
	/*准备音频接口,并判断是否准备好*/
	if((err=snd_pcm_prepare(capture_handle))<0) 
	{
		printf("无法使用音频接口 (%s)\n",snd_strerror(err));
		exit(1);
	}
	printf("音频接口准备好.\n");
	
	return 0;
}



//音频采集线程
void capture_audio_data(snd_pcm_t *capture_handle,int buffer_frames)
{
	int err;
	//因为frame样本数固定为1024,而双通道，每个采样点2byte，所以一次要发送1024*2*2byte数据给frame->data[0];
	/*配置一个数据缓冲区用来缓冲数据*/
	//snd_pcm_format_width(format) 获取样本格式对应的大小(单位是:bit)
	int frame_byte=snd_pcm_format_width(format)/8;

	/*开始采集音频pcm数据*/
	printf("开始采集数据...\n");

	unsigned char *audio_buffer;
	while(1) 
	{
		audio_buffer=malloc(buffer_frames*frame_byte*AUDIO_CHANNEL_SET); //2048
		if(audio_buffer==NULL)
		{
			printf("缓冲区分配错误.\n");
			break;
		}
		
		/*从声卡设备读取一帧音频数据:2048字节*/
		if((err=snd_pcm_readi(capture_handle,audio_read_buff,buffer_frames))!=buffer_frames) 
		{
			  printf("从音频接口读取失败(%s)\n",snd_strerror(err));
			  exit(1);
		}
	
		pthread_mutex_lock(&mutex_audio); /*互斥锁上锁*/

		memcpy(audio_buffer,audio_read_buff,buffer_frames*frame_byte*AUDIO_CHANNEL_SET);
		printf("audio_buffer address: %p\n", (void*)audio_buffer);
        
		//添加节点
		List_AddNode(list_head,audio_buffer);
        
        /*释放数据缓冲区*/
        free(audio_buffer);
        pthread_mutex_unlock(&mutex_audio); /*互斥锁解锁*/
    
        /*关闭音频采集卡硬件*/
        snd_pcm_close(capture_handle);
		
	}
 

 


}