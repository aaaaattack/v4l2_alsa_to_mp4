#include "list.h"
/*
函数功能： 创建链表头
*/
struct AUDIO_DATA *List_CreateHead(struct AUDIO_DATA *head)
{
	if(head==NULL)
	{
		head=malloc(sizeof(struct AUDIO_DATA));
		head->next=NULL;
	}
	return head;
}

/*
函数功能: 插入新的节点
*/
void List_AddNode(struct AUDIO_DATA *head,unsigned char* audio_buffer)
{
    if (head == NULL || audio_buffer == NULL) {
        fprintf(stderr, "Invalid head or audio_buffer\n");
        return; // 或者其他错误处理
    }
        printf("AAAAAAAAAAAAA\n");
	struct AUDIO_DATA *tmp=head;
	struct AUDIO_DATA *new_node;

    new_node = malloc(sizeof(struct AUDIO_DATA));
    if (new_node == NULL) {
        printf("new node内存分配失败\n");
        return;
    }
    printf("BBBBBBBBBBBBB\n");
    new_node->audio_buffer = audio_buffer;
    new_node->next = NULL;
    printf("CCCCCCCCCCCC\n");

    if (tmp == NULL) {
        // 如果链表为空，将新节点作为头节点
        printf("链表为空，准备将新节点作为头节点\n");
        head = new_node;
        printf("链表为空，已经将新节点作为头节点\n");

    } else {
        // 找到链表尾部
        printf("链表不为空\n");
        while (tmp->next) {
            printf("非法指针\n");
            tmp = tmp->next;
            printf("找到链表尾部\n");
        }
        // 插入新的节点
        tmp->next = new_node;
        printf("DDDDDDDDDDD\n");
    }
}

/*
函数功能:删除节点
*/
void List_DelNode(struct AUDIO_DATA *head,unsigned char* audio_buffer)
{
	struct AUDIO_DATA *tmp=head;
	struct AUDIO_DATA *p;
	/*找到链表中要删除的节点*/
	while(tmp->next)
	{
		p=tmp;
		tmp=tmp->next;
		if(tmp->audio_buffer==audio_buffer)
		{
			p->next=tmp->next;
			free(tmp);
		}
	}
}

/*

*/


/*
函数功能:遍历链表，得到节点总数量
*/
int List_GetNodeCnt(struct AUDIO_DATA *head)
{
	int cnt=0;
	struct AUDIO_DATA *tmp=head;
	while(tmp->next)
	{
		tmp=tmp->next;
		cnt++;
	}
	return cnt;
}


