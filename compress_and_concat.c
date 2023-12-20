#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

//#include "./main_write_header_cb.h"
#include "./crc.h"
#include "./compress_and_concat.h"
#include "./decompress.h"
#include "./zutil.h"


bool write_PNG_header(char* buf, long len);
bool write_PNG_width(char* buf, unsigned width, long len);
bool write_PNG_height(char* buf, unsigned height, long len);
bool write_PNG_data_IHDR(char* buf, long len);


int compress_and_concat(char* p_decompressed_data, unsigned decompressed_len) {
	if (!p_decompressed_data) { return -1; }

	unsigned all_height = 300;
	unsigned all_width = 400;
    U64 all_len = 0;
    char* all_buf;

	U64 compressed_len = 0;
	char* p_compressed_data = NULL;

	p_compressed_data = (char*)malloc(decompressed_len * sizeof(char));
	int compressed_status = mem_def((U8*)p_compressed_data, &compressed_len,
									(U8*)p_decompressed_data, decompressed_len, 6);
	if (compressed_status != Z_OK) {
		printf("Compression Failed: %d\n", compressed_status);
		return -1;
	}
	
	all_len = 8 + 3*4 + 13 + 3*4 + compressed_len + 3*4;
	all_buf = (char*)malloc(all_len*sizeof(char) + 1);
	if (!all_buf) { printf("Malloc Err\n"); return -1; }
	all_buf[all_len] = '\0';
	
	char* p_IHDR = (char*)all_buf + 8;
	char* p_IDAT = (char*)all_buf + 33;
	char* p_IEND = (char*)all_buf + 37 + compressed_len + 4 + 4;
	char PNG_type_IHDR[5] = "IHDR";
	char PNG_type_IDAT[5] = "IDAT";
	char PNG_type_IEND[5] = "IEND";
	unsigned PNG_length_IHDR = 13; 
	unsigned PNG_length_IDAT = compressed_len;
	unsigned PNG_length_IEND = 0;
	unsigned PNG_CRC_IHDR;
	unsigned PNG_CRC_IDAT;
	unsigned PNG_CRC_IEND;
	
	write_PNG_header(all_buf, all_len);
	write_PNG_width(all_buf, all_width, all_len);
	write_PNG_height(all_buf, all_height, all_len);
	write_PNG_data_IHDR(all_buf, all_len);

	*(unsigned*)((char*)all_buf + 12) = *(unsigned*)PNG_type_IHDR;
	*(unsigned*)((char*)all_buf + 37) = *(unsigned*)PNG_type_IDAT;
	*(unsigned*)((char*)all_buf + all_len - 8)
				= *(unsigned*)PNG_type_IEND;

	for (U64 i = 0; i < compressed_len; i++)
			all_buf[i + 41] = p_compressed_data[i];

	*(unsigned*)((char*)all_buf + 8) = htonl(PNG_length_IHDR);
	*(unsigned*)((char*)all_buf + 33) = htonl(PNG_length_IDAT);
	*(unsigned*)((char*)all_buf + all_len - 12)
				= htonl(PNG_length_IEND);

	PNG_CRC_IHDR = htonl(crc((unsigned char*)(p_IHDR + 4), PNG_length_IHDR + 4));
	PNG_CRC_IDAT = htonl(crc((unsigned char*)(p_IDAT + 4), PNG_length_IDAT + 4));
	PNG_CRC_IEND = htonl(crc((unsigned char*)(p_IEND + 4), PNG_length_IEND + 4));

	*(unsigned*)((char*)all_buf + 29) = PNG_CRC_IHDR;
    *(unsigned*)((char*)all_buf + 41 + compressed_len) = PNG_CRC_IDAT;
    *(unsigned*)((char*)all_buf + all_len - 4) = PNG_CRC_IEND;

	FILE *new_file = fopen("all.png", "wb");
	if (!new_file) { printf("New file creation Err\n"); return -1; }

	for (unsigned i = 0; i < all_len; i++)
			fputc(all_buf[i], new_file);
	fclose(new_file);


	// clean up
	free(p_compressed_data);
	free(all_buf);

	return 0;
}

bool write_PNG_header(char* buf, long len) {
	if (len < 8) { return false; }
	unsigned char PNG_header[8] = { 0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A };
	for ( int i = 0; i < 8; i++)
		buf[i] = PNG_header[i];
	return true;
}

bool write_PNG_width(char* buf, unsigned width, long len) {
	if (len < 20) { return false; }
	char* pointer = buf + 16;
	*(unsigned*)pointer = htonl(width);
	return true;
}

bool write_PNG_height(char* buf, unsigned height, long len) {
	if (len < 24) { return false; }
	char* pointer = buf + 20;
	*(unsigned*)pointer = htonl(height);
	return true;
}

bool write_PNG_data_IHDR(char* buf, long len) {
	if (len < 29) { return false; }
	char PNG_data_IHDR[13] = { 0,0,0,0, 0,0,0,0, 8, 6, 0, 0, 0 };
	for (int i = 0; i < 5; i++)
		buf[i + 24] = PNG_data_IHDR[i + 8];
	return true;
}


