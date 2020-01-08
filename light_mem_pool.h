#ifndef LIGHT_MEM_POOL_H_INCLUDED
#define LIGHT_MEM_POOL_H_INCLUDED

#include <pthread.h>

#define MEM_TABLE_HEAD_ITEM 20
#define FRAME_ITEM_CNT  10

#define BLOCK_BUSY  1
#define BLOCK_FREE  0



struct mem_object{
	struct mem_frame * container;
	int busy;
};

struct mem_frame{
	struct mem_frame * next;
	unsigned int free_cnt;
	unsigned int total_cnt;
	unsigned int total_size;
	unsigned int item_size;	//包含头信息
	unsigned int item_buf_size;//不包含头

	unsigned char * bitmap;
	unsigned char * buf;

	struct mem_frame_head * head;
};

struct mem_frame_head{
	unsigned int frame_size;
	pthread_mutex_t mutex;
	struct mem_frame * frame_list;
};

struct mem_table{
	unsigned int head_cnt;
	struct mem_frame_head head_list[MEM_TABLE_HEAD_ITEM];
};



int init_mem_table();

void * alloc_buffer(int suggest_len,int *real_len);

int free_buffer(void * buf);

















#endif // LIGHT_MEM_POOL_H_INCLUDED
