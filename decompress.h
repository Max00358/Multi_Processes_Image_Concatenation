#include "stack.h"



typedef struct decompressed_png {
	unsigned length;
	char buf[6*(400*4 + 1) + 8 + 3*12 + 13];
	unsigned png_id;
	bool initialized; 
} decompressed_png;



bool decompress(STACK_RECV_BUF* p_recv_buf, decompressed_png* p_png);

