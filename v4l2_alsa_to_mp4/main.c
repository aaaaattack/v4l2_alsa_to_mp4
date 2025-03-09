#include "thread_task.h"
#include "list.h"



int main(void)
{
	
	pthread_t video_thread_id, audio_thread_id;
	/*初始化互斥锁*/
	if (pthread_mutex_init(&video_mutex, NULL) != 0) {
		perror("Failed to initialize mutex");
	}
	/*初始化条件变量*/
	pthread_cond_init(&cond,NULL);

    /*初始化互斥锁*/
	if (pthread_mutex_init(&mutex_audio, NULL) != 0) {
		perror("Failed to initialize mutex_audio");
	}

	//创建链表头
	list_head=List_CreateHead(list_head);
	printf("**********************\nlist head 的 地址 %p\n list head next 的地址%p\n**********************\n", list_head,list_head->next);


	capture_audio_data_init("hw:1");
	printf("**********************\n    FUCKING ERR    \nlist head 的 地址 %p\n list head next 的地址%p\n**********************\n", list_head,list_head->next);

	/*创建子线程: 采集摄像头的数据*/
	pthread_create(&video_thread_id,NULL,pthread_read_video_data,NULL);
    
    /*创建子线程: 采集音频的数据*/
	pthread_create(&audio_thread_id,NULL,pthread_read_audio_data,NULL);


	while (1)
    {
		video_audio_encode(filename);
        sleep(20);
    }
    
	return 0;
}
