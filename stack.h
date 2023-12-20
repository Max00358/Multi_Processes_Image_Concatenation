
/* The code is 
 * Copyright(c) 2018-2019 Yiqing Huang, <yqhuang@uwaterloo.ca>.
 *
 * This software may be freely redistributed under the terms of the X11 License.
 */
/**
 * @brief  stack to push/pop integer(s), API header  
 * @author yqhuang@uwaterloo.ca
 */
#include <stdio.h>
#define MAX_PNG_SNIPPET_SIZE 6*(400*4 + 1) + 8 + 3*12 + 13

/*typedef struct png_snippet {
	unsigned img_id;
	unsigned size;
    char data[MAX_PNG_SNIPPET_SIZE];
} png_snippet;*/

typedef struct stack_recv_buf {
	char buf[MAX_PNG_SNIPPET_SIZE];
	size_t size;
	size_t max_size;
	int seq;
} STACK_RECV_BUF;

typedef struct stack {
    int size;               /* the max capacity of the stack */
    int pos;                /* position of last item pushed onto the stack */
    STACK_RECV_BUF* items;             /* stack of stored integers */
} STACK; 

int sizeof_shm_stack(int size);
int init_shm_stack(struct stack* p, int stack_size);
struct stack* create_stack(int size);
void destroy_stack(struct stack* p);
int is_full(struct stack* p);
int is_empty(struct stack* p);
int push(struct stack* p, STACK_RECV_BUF item);
int pop(struct stack* p, STACK_RECV_BUF* p_item);
