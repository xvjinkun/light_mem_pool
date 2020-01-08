#include "light_mem_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


static struct mem_table * g_mem_table = NULL;

static struct mem_object * get_frame_object(struct mem_frame * frame,int index){
    return (struct mem_object*)(frame->buf + frame->item_size * index);
}

static int get_frame_object_index(struct mem_frame * frame,struct mem_object * block){

    unsigned char * p = (unsigned char *)block;
    int offset = p - frame->buf;
    return offset / frame->item_size;
}

static void init_frame_buf(struct mem_frame * frame){
	for(int i = 0; i < frame->total_cnt; ++i){
		struct mem_object * mo = get_frame_object(frame,i);
		mo->container = frame;
		mo->busy = BLOCK_FREE;
	}
}

static struct mem_frame * alloc_frame(struct mem_frame_head * head,int item_cnt,unsigned int item_size){
	struct mem_frame * frame = (struct mem_frame*)malloc(sizeof(struct mem_frame));
	frame->next = NULL;
	frame->free_cnt = item_cnt;
	frame->total_cnt = item_cnt;
	frame->item_buf_size = item_size;
	frame->item_size = frame->item_buf_size + sizeof(struct mem_object);
	frame->total_size = frame->item_size * item_cnt;
	frame->bitmap = (unsigned char *)malloc(frame->total_cnt);

	memset(frame->bitmap,0,frame->total_cnt);
	frame->buf = (unsigned char *)malloc(frame->total_size);

	frame->head = head;

	init_frame_buf(frame);

	return frame;
}



int init_mem_table(){
	if(NULL != g_mem_table){
		return 0;
	}

	g_mem_table = (struct mem_table*)malloc(sizeof(struct mem_table));
	memset(g_mem_table,0,sizeof(struct mem_table));

	g_mem_table->head_cnt = MEM_TABLE_HEAD_ITEM;
	for(int i = 0; i < MEM_TABLE_HEAD_ITEM; ++i){
		g_mem_table->head_list[i].frame_size = pow(2,i);
		g_mem_table->head_list[i].frame_list = NULL;

		pthread_mutex_init(&g_mem_table->head_list[i].mutex,NULL);
	}

	return 0;
}

static struct mem_frame_head * get_suggest_head(struct mem_table * tbl,int len){
	struct mem_frame_head * head = NULL;
	for(int i = 0; i < tbl->head_cnt; ++i){
		if(len <= tbl->head_list[i].frame_size){
			head = &(tbl->head_list[i]);
			break;
		}
	}

	if(NULL == head){
		head = &(tbl->head_list[tbl->head_cnt - 1]);
	}

	return head;
}

static struct mem_frame * get_free_frame(struct mem_frame_head * head){
	struct mem_frame * free_frame = NULL;
	struct mem_frame * start = head->frame_list;
	while(NULL != start){
		if(start->free_cnt > 0){
			free_frame = start;
			break;
		}
	}

	return free_frame;
}

static struct mem_frame * attach_head_frame(struct mem_frame_head * head,struct mem_frame * frame){
    if(NULL == head->frame_list){
        head->frame_list = frame;
        frame->next = NULL;

        return frame;
    }

    struct mem_frame * tail = head->frame_list;
    while(tail->next){
        tail = tail->next;
    }

    tail->next = frame;
    frame->next = NULL;

    return frame;
}

static struct mem_frame * pull_head_frame(struct mem_frame_head * head){
	struct mem_frame * free_frame = get_free_frame(head);
	if(NULL == free_frame){
		struct mem_frame * new_frame = alloc_frame(head,FRAME_ITEM_CNT,head->frame_size);
		free_frame = attach_head_frame(head,new_frame);
	}

	return free_frame;
}

static int get_frame_free_bit(struct mem_frame * frame){
    int ret = -1;
    for(int i = 0; i < frame->total_cnt; ++i){
        if(frame->bitmap[i] == BLOCK_FREE){
            ret = i;
            break;
        }
    }

    return ret;
}

static void mark_frame_bit(struct mem_frame * frame,int index,int state){
    frame->bitmap[index] = state;
}

static void offset_frame_free_cnt(struct mem_frame * frame,int cnt){
    frame->free_cnt += cnt;
}


static struct mem_object * request_frame_block(struct mem_frame * frame){
    if(frame->free_cnt <= 0){
        return NULL;
    }

    int free_bit = get_frame_free_bit(frame);
    if(free_bit < 0){
        return NULL;
    }

    mark_frame_bit(frame,free_bit,BLOCK_BUSY);
    offset_frame_free_cnt(frame,-1);

    struct mem_object * mo = get_frame_object(frame,free_bit);
    mo->busy = BLOCK_BUSY;
    return mo;
}

void * alloc_buffer(int suggest_len,int *real_len){
	struct mem_frame_head * head = get_suggest_head(g_mem_table,suggest_len);

	pthread_mutex_lock(&head->mutex);
	*real_len = suggest_len > head->frame_size ? head->frame_size : suggest_len;

	struct mem_frame * dst_frame = pull_head_frame(head);

	struct mem_object * block = request_frame_block(dst_frame);

	pthread_mutex_unlock(&head->mutex);

	unsigned char * buf = (unsigned char*)block;
	return buf + sizeof(struct mem_object);
}

int free_buffer(void * buf){
    unsigned char * tmp = (unsigned char*)buf;
    tmp -= sizeof(struct mem_object);
    struct mem_object * block = (struct mem_object*)tmp;
    struct mem_frame * frame = block->container;
    struct mem_frame_head * head = frame->head;

    //pthread_mutex_lock(&head->mutex);

    int block_index = get_frame_object_index(frame,block);
    mark_frame_bit(frame,block_index,BLOCK_FREE);
    offset_frame_free_cnt(frame,1);
    block->busy = BLOCK_FREE;

    //pthread_mutex_unlock(&head->mutex);

    return 0;
}
