#ifndef LIST_H
#define LIST_H
#include "common.h"

//保存音频数据链表
struct AUDIO_DATA
{
	unsigned char* audio_buffer;
	struct AUDIO_DATA *next;
};

//定义一个链表头

struct AUDIO_DATA *List_CreateHead(struct AUDIO_DATA *head);
void List_AddNode(struct AUDIO_DATA *head,unsigned char* audio_buffer);
void List_DelNode(struct AUDIO_DATA *head,unsigned char* audio_buffer);
int List_GetNodeCnt(struct AUDIO_DATA *head);

#endif