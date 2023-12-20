#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "compress_and_concat.h"
#include "decompress.h"
//#include "main_write_header_cb.h"
//#include "stack.h"

#define MAX_IMAGES 50

/*typedef struct decompressed_png {
	unsigned length;
	char buf[6*(400*4 + 1) + 8 + 3*12 + 13];
	unsigned png_id;
	bool initialized; 
} decompressed_png;*/

void producer();
void consumer();
RECV_BUF* get_png(unsigned png_number);

static int pid;
//char IMG_URL[] = "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=00"; //remember to give mod

int shm_sem_not_full;
int shm_sem_not_empty;
int shm_sem_producer_check_mutex;
int shm_sem_consumer_check_mutex;
int shm_sem_stack_mutex;

int shm_stack_id;
int shm_decompressed_arr_id;
int shm_png_count_id;

int shm_total_count_id;
int shm_count_id;
int shm_producer_count_id;
int shm_consumer_count_id;
int shm_server_num_id;

unsigned count_sleep;
unsigned image_num;

int main(int argc, char** argv) {
    double times[2];
    struct timeval tv;

    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[0] = (tv.tv_sec) + tv.tv_usec/1000000.;
 
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	if (argc != 6) { printf("Incorrect Arg Count\n"); return -1; }
	
	unsigned count_stack_max = atoi(argv[1]);
	unsigned count_producers = atoi(argv[2]);
	unsigned count_consumers = atoi(argv[3]);
	count_sleep = atoi(argv[4]);
	image_num = atoi(argv[5]);

	// Check inputs // Ask Maran upper limit
	if (count_stack_max <= 0 || count_producers <= 0 || count_consumers <= 0 ||
				count_sleep < 0 || image_num <= 0 || image_num > 3)
	{
		printf("Incorrect Args\n");
		return -1;
	}

	// Initialize Semaphore Shared Memory
	shm_sem_not_full = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_not_empty = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_producer_check_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
                               IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_consumer_check_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
                               IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_stack_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	
	// Get Semaphore Pointers
	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
	sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_producer_check_mutex = (sem_t*)shmat(shm_sem_producer_check_mutex, NULL, 0);
	sem_t* p_sem_consumer_check_mutex = (sem_t*)shmat(shm_sem_consumer_check_mutex, NULL, 0);
	sem_t* p_sem_stack_mutex = (sem_t*)shmat(shm_sem_stack_mutex, NULL, 0);

	// Initialize Semaphores
	sem_init(p_sem_not_full, 1, count_stack_max);
	sem_init(p_sem_not_empty, 1, 0);
	sem_init(p_sem_producer_check_mutex, 1, 1);
	sem_init(p_sem_consumer_check_mutex, 1, 1);
	sem_init(p_sem_stack_mutex, 1, 1);

	// Create Stack and Array Shared Memory
	shm_stack_id = shmget(IPC_PRIVATE, 16 + count_stack_max*sizeof(STACK_RECV_BUF),
							   IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_decompressed_arr_id = shmget(IPC_PRIVATE, 50 * sizeof(decompressed_png),
									 IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	
	// Get Pointers and Initialize Stack and Array
	STACK* p_stack = (STACK*)shmat(shm_stack_id, NULL, 0);
	int init_res = init_shm_stack((STACK*)p_stack, (int)count_stack_max);
	if (init_res) { printf("init_res Failed\n"); exit(1); } // cleanup
	decompressed_png* p_decompressed =
					(decompressed_png*)shmat(shm_decompressed_arr_id, NULL, 0); 
	for (unsigned i = 0; i < 50; i++)
		((decompressed_png*)p_decompressed)[i].initialized = false;

	// Initialize Atomic Counter Variables
		//shm_total_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
		//					  IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_producer_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_consumer_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_server_num_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
			 		 	  IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	// Get Atomic Pointers
		//atomic_uint* p_total_count = (atomic_uint*)shmat(shm_total_count_id, NULL, 0);
	atomic_uint* p_count = (atomic_uint*)shmat(shm_count_id, NULL, 0);
	atomic_uint* p_producer_count = (atomic_uint*)shmat(shm_producer_count_id, NULL, 0);
	atomic_uint* p_consumer_count = (atomic_uint*)shmat(shm_consumer_count_id, NULL, 0);
	atomic_uint* p_server_num = (atomic_uint*)shmat(shm_server_num_id, NULL, 0);

	// Initialize Atomic Variables
		//atomic_store(p_total_count, 0);
	atomic_store(p_count, 0);
	atomic_store(p_producer_count, 0);
	atomic_store(p_consumer_count, 0);
	atomic_store(p_server_num, 0);

	//atomic_uint* p_png_count = (atomic_uint*)shmat(shm_png_count_id, NULL, 0);
	//atomic_store(p_png_count, 0);

	for (unsigned i = 0; i < count_producers; i++) {
		pid = fork();
		if (pid == 0) {
			producer();
			exit(0); // cleanup
		} else if (pid > 0) {
		
		} else {
			exit(1);
		}
	}

	for (unsigned i = 0; i < count_consumers; i++) {
		int pid = fork();
		if (pid == 0) {
			consumer();
			exit(0); // cleanup
		} else if (pid > 0) {
		
		} else {
            exit(1);
		}
	}
	
	while (wait(NULL) > 0);

	unsigned decompressed_len = 0;
	char* p_decompressed_data;
	for (unsigned i = 0; i < 50; i++){
		decompressed_len += p_decompressed[i].length;
	}
	p_decompressed_data	= (char*)malloc(decompressed_len*sizeof(char));

	unsigned index = 0;
	for (unsigned i = 0; i < 50; i++) {
		for (unsigned j = 0; j < p_decompressed[i].length; j++) {
			p_decompressed_data[index] = p_decompressed[i].buf[j];
			++index;
		}
	}
	
	int res = compress_and_concat(p_decompressed_data, decompressed_len);	
	if (res)
		printf("Compress and Concat Error\n");

	// cleanup	
	free(p_decompressed_data);

	// sem destroy	
	sem_destroy(p_sem_not_full);
	sem_destroy(p_sem_not_empty);
	sem_destroy(p_sem_producer_check_mutex);
	sem_destroy(p_sem_consumer_check_mutex);
	sem_destroy(p_sem_stack_mutex);

	// free shared memory
	shmctl(shm_sem_not_full, IPC_RMID, NULL);
	shmctl(shm_sem_not_empty, IPC_RMID, NULL);
	shmctl(shm_sem_producer_check_mutex, IPC_RMID, NULL);
	shmctl(shm_sem_consumer_check_mutex, IPC_RMID, NULL);
	shmctl(shm_sem_stack_mutex, IPC_RMID, NULL);

	shmctl(shm_stack_id, IPC_RMID, NULL);
	shmctl(shm_decompressed_arr_id, IPC_RMID, NULL);
	shmctl(shm_png_count_id, IPC_RMID, NULL);
	
	shmctl(shm_total_count_id, IPC_RMID, NULL);
	shmctl(shm_count_id, IPC_RMID, NULL);
	shmctl(shm_producer_count_id, IPC_RMID, NULL);
	shmctl(shm_consumer_count_id, IPC_RMID, NULL);
	shmctl(shm_server_num_id, IPC_RMID, NULL);

	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("paster2 execution time: %.6lf seconds\n", elapsed_time);

   
    if (gettimeofday(&tv, NULL) != 0) {
        perror("gettimeofday");
        abort();
    }
    times[1] = (tv.tv_sec) + tv.tv_usec/1000000.;
    printf("paster2 execution time: %.6lf seconds\n", times[1] - times[0]);
 
	return 0;
}

void producer() {
	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
    sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_producer_check_mutex = (sem_t*)shmat(shm_sem_producer_check_mutex, NULL, 0);
	sem_t* p_sem_stack_mutex = (sem_t*)shmat(shm_sem_stack_mutex, NULL, 0);
	STACK* p_stack = (STACK*)shmat(shm_stack_id, NULL, 0);  
	atomic_uint* p_producer_count = (atomic_uint*)shmat(shm_producer_count_id, NULL, 0);
    STACK_RECV_BUF item;

	while (true) {
		unsigned png_number;
		RECV_BUF* p_buf;

		sem_wait(p_sem_producer_check_mutex);
			png_number = atomic_load(p_producer_count);
			if (png_number >= MAX_IMAGES) {
				sem_post(p_sem_producer_check_mutex);
				break;
			}
			atomic_fetch_add(p_producer_count, 1);
		sem_post(p_sem_producer_check_mutex);

	    p_buf = get_png(png_number);
		
		item.size = p_buf->size;
		item.max_size = p_buf->max_size;
		item.seq = p_buf->seq;
		
		for (unsigned i = 0; i < p_buf->size; i++)
			item.buf[i] = p_buf->buf[i];

        sem_wait(p_sem_not_full);
            sem_wait(p_sem_stack_mutex);
				push(p_stack, item);
            sem_post(p_sem_stack_mutex);
        sem_post(p_sem_not_empty);

		recv_buf_cleanup(p_buf);
		free(p_buf);
    }
}

void consumer() {
   	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
	sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_consumer_check_mutex = (sem_t*)shmat(shm_sem_consumer_check_mutex, NULL, 0);
    sem_t* p_sem_stack_mutex = (sem_t*)shmat(shm_sem_stack_mutex, NULL, 0);
	STACK* p_stack = (STACK*)shmat(shm_stack_id, NULL, 0);
	decompressed_png* p_decompressed =
                    (decompressed_png*)shmat(shm_decompressed_arr_id, NULL, 0);
	
	atomic_uint* p_consumer_count = (atomic_uint*)shmat(shm_consumer_count_id, NULL, 0);
	STACK_RECV_BUF item;
    decompressed_png png;
	
	while (true) {
		bool res = false;

		sem_wait(p_sem_consumer_check_mutex);
            if (atomic_load(p_consumer_count) >= MAX_IMAGES) {             
                sem_post(p_sem_consumer_check_mutex); 
                break;                                
            }                                         
            atomic_fetch_add(p_consumer_count, 1);
	    sem_post(p_sem_consumer_check_mutex);
		
		sem_wait(p_sem_not_empty);
            sem_wait(p_sem_stack_mutex);
                pop(p_stack, &item);
            sem_post(p_sem_stack_mutex);
        sem_post(p_sem_not_full);

		int index = item.seq;

        ((decompressed_png*)p_decompressed)[index].initialized = true;
		
		usleep(1000 * count_sleep);		
		res = decompress(&item, &png);
        if (!res) {
            continue;
	    }

		p_decompressed[index].length = png.length;
		p_decompressed[index].png_id = png.png_id;		
		for (unsigned i = 0; i < png.length; i++)
			p_decompressed[index].buf[i] = png.buf[i];
	 }
}

RECV_BUF* get_png(unsigned png_number) {
	atomic_uint* p_server_num = (atomic_uint*)shmat(shm_server_num_id, NULL, 0);
	unsigned server_num = atomic_fetch_add(p_server_num, 1);
	char IMG_URL[] = "http://ece252-1.uwaterloo.ca:2530/image?img=1&part=00";
	CURL* curl_handle;
	CURLcode res;
	RECV_BUF* p_recv_buf;

__get_png_start:
//	printf("Server: %u\n", (server_num % 3) + 1);
	IMG_URL[14] = (server_num % 3) + 1 + '0';
	++server_num;
	IMG_URL[44] = image_num + '0';
	IMG_URL[51] = (png_number / 10) + '0';
	IMG_URL[52] = (png_number % 10) + '0';

	p_recv_buf = (RECV_BUF*)malloc(sizeof(RECV_BUF));
	recv_buf_init(p_recv_buf, BUF_SIZE);

	 if (!p_recv_buf) {
        goto __get_png_start;
	} else if (!(p_recv_buf->buf)) {
		free(p_recv_buf);
		goto __get_png_start;
	}
	
	curl_handle = curl_easy_init();

	if (curl_handle == NULL) {
		recv_buf_cleanup(p_recv_buf);	
		free(p_recv_buf);
	   	goto __get_png_start;
	}

	curl_easy_setopt(curl_handle, CURLOPT_URL, IMG_URL);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_cb_curl3);
	curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void*)p_recv_buf);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERFUNCTION, header_cb_curl);
	curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, (void*)p_recv_buf);
	curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
	res = curl_easy_perform(curl_handle);

	/* cleaning up */
	curl_easy_cleanup(curl_handle);

	if (res != CURLE_OK) {
		recv_buf_cleanup(p_recv_buf);
		free(p_recv_buf);
		goto __get_png_start;
	}
	if (!(p_recv_buf->buf)) {
		free(p_recv_buf);
		goto __get_png_start;
	}
		
	return p_recv_buf;	
}

