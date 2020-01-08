#include <stdio.h>
#include <stdlib.h>
#include "light_mem_pool.h"
#include <sys/time.h>
#include <string.h>


int main()
{
    printf("Hello world!\n");

    init_mem_table();

    struct timeval tv_start,tv_end;
    memset(&tv_start,0,sizeof(struct timeval));
    memset(&tv_end,0,sizeof(struct timeval));

    gettimeofday(&tv_start,NULL);
    for(int i = 0; i < 10000000; ++i){
        int suggest_len = rand() % 65535;
        int alloc_size = 0;

        void * buf = alloc_buffer(suggest_len,&alloc_size);

        free_buffer(buf);

    }
    gettimeofday(&tv_end,NULL);

    printf("alloc cost %ld ms\n",((tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000));


    memset(&tv_start,0,sizeof(struct timeval));
    memset(&tv_end,0,sizeof(struct timeval));

    gettimeofday(&tv_start,NULL);
    for(int i = 0; i < 10000000; ++i){
        int suggest_len = rand() % 65535;

        void * buf = malloc(suggest_len);

        free(buf);

    }
    gettimeofday(&tv_end,NULL);

    printf("malloc cost %ld ms\n",((tv_end.tv_sec - tv_start.tv_sec) * 1000 + (tv_end.tv_usec - tv_start.tv_usec) / 1000));

    return 0;
}
