#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//#include "./main_write_header_cb.h"
#include "./crc.h"
#include "./decompress.h"
#include "./zutil.h"

bool decompress(STACK_RECV_BUF* p_recv_buf, decompressed_png* p_png);

bool check_PNG_header(char* buf, long len);
bool write_PNG_header(char* buf, long len);
bool write_PNG_data_IHDR(char* buf, long len);
bool write_PNG_width(char* buf, unsigned width, long len);
bool write_PNG_height(char* buf, unsigned height, long len);
int get_PNG_width(char* buf, long len);
int get_PNG_height(char* buf, long len);

bool check_CRC_error(char* buf, long len);
unsigned get_chunk_data_length(char* buf, char* chunk, long len);
char* get_chunk_pointer(char*buf, long len, unsigned chunk_type);
unsigned get_CRC(char* buf, char* chunk, long len);

bool decompress(STACK_RECV_BUF* p_recv_buf, decompressed_png* p_png) {
	char* buffer;
	long file_length = p_recv_buf->size;

	bool is_PNG;
	unsigned width_PNG;
	unsigned height_PNG;

	buffer = (char*) malloc((p_recv_buf->size + 1) * sizeof(char));
	if (buffer == NULL) { printf("Malloc Error\n"); return -1; }
	buffer[file_length] = '\0';
	
	memcpy(buffer, p_recv_buf->buf, file_length);

	is_PNG = check_PNG_header(buffer, file_length);
	if (!is_PNG) {
		free(buffer);
		return false;
	}
	
	width_PNG = get_PNG_width(buffer, file_length);
	height_PNG = get_PNG_height(buffer, file_length);

	char* p_IDAT = get_chunk_pointer(buffer, file_length, 1);
	if(!p_IDAT) {
		free(buffer);
		return false;
	}
	
	U64 IDAT_data_len = get_chunk_data_length(buffer, p_IDAT, file_length);
	char* p_decompressed_data = (char*)malloc((height_PNG*(width_PNG*4 + 1))*sizeof(char));
	U64 decompressed_data_len = 0;
	int decompress_res = mem_inf((U8*)p_decompressed_data, &decompressed_data_len,
								 (U8*)(p_IDAT+8), IDAT_data_len);
	
	if (decompress_res != Z_OK) {
		free(p_decompressed_data);
		free(buffer);
		return false;
	}

	p_png->png_id = p_recv_buf->seq;
	p_png->length = (int)decompressed_data_len;
	for (unsigned i = 0; i < p_png->length; i++)
		p_png->buf[i] = p_decompressed_data[i];

	// cleanup
	free(p_decompressed_data);
	free(buffer);

	return true;
}


bool check_PNG_header(char* buf, long int len) {
    if (len < 8) { return false; }
    const char png_check[] = "PNG";
    for (int i = 0; i < 3; i++) {
        if (buf[i+1] != png_check[i])
            return false;
    }
    return true;
}

unsigned get_chunk_data_length(char* buf, char* chunk,  long len) {
    if (chunk - buf + 4 >= len || chunk == NULL || buf == NULL) {
        return -1;
    }
    return htonl(*(unsigned*)chunk);
}

int get_PNG_height(char* buf, long len) {
    int offset = 20;
    if (len < 24 || buf == NULL) { return -1; }
    return htonl(*(unsigned*)&buf[0 + offset]);
}

int get_PNG_width(char* buf, long len) {
    int offset = 16;
    if (len < 20 || buf == NULL) { return -1; }
    return htonl(*(unsigned*)&buf[0 + offset]);
}

char* get_chunk_pointer(char*buf, long len, unsigned chunk_type){
    if (chunk_type >= 3) { printf("Chunk type 0, 1, 2\n"); return NULL; }

    long int current_pos = 0;
    int png_data_len = 0;

    current_pos += 8;

    if (chunk_type == 0) {   // reach IHDR
        if (current_pos >= len) { printf("IHDR Exceeds len\n"); return NULL; }
        return &buf[current_pos];
    }
    current_pos += 4 + 4 + 13 + 4;

    if (chunk_type == 1) {  //reach IDAT
        if (current_pos >= len) { printf("IDAT Exceeds len\n"); return NULL; }
        return (char*)buf + current_pos;
    }

    png_data_len = get_chunk_data_length(buf, (char*)(buf + current_pos), len);
    if (png_data_len < 0) { printf("Chunk Exceeds len\n"); return NULL; }

    current_pos += 4 + 4 + png_data_len + 4;

    if (current_pos >= len) { printf("IEND Exceeds len\n"); return NULL; }
    return &buf[current_pos]; //reach IEND
}

unsigned get_CRC(char* buf, char* chunk, long len) {
    long offset = (long)( (char*)chunk - (char*)buf );
    if (len < offset || chunk < buf || buf == NULL || chunk == NULL)
        return -1;

    unsigned length = get_chunk_data_length(buf, chunk, len);
    char* crc_pointer = &chunk[4 + 4 + length];
    if ((crc_pointer + 3) > &buf[len-1]) { return -1; }

    return htonl(*(unsigned*)crc_pointer);
}



