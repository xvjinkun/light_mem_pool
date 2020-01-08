# light_mem_pool
轻量级定长内存池

C语言练手

原理：
创建一个20层的数组，每个数组代表2^N长度block list的链表，每个链表有包含不定长度的frame，每个frame初始化制定个数的block，每个frame通过一个位图控制每个block的busy/free，
