#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <unistd.h>

#include "compress_and_concat.h"
#include "main_write_header_cb.h"
#include "stack.h"

#define MAX_IMAGES 50

void producer();
void consumer();

static int pid;

int shm_sem_not_full;
int shm_sem_not_empty;
int shm_sem_producer_check_mutex;
int shm_sem_consumer_check_mutex;
int shm_sem_mutex;


int shm_total_count_id;
int shm_count_id;
int shm_producer_count_id;
int shm_consumer_count_id;

int main(int argc, char** argv) {
	unsigned count_max = 10; //atoi(argv[1]);
	unsigned count_producers = 15; //atoi(argv[2]);
	unsigned count_consumers = 3; //atoi(argv[3]);

	shm_sem_not_full = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_not_empty = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_producer_check_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
                               IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_consumer_check_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
                               IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_sem_mutex = shmget(IPC_PRIVATE, sizeof(sem_t),
								IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
	sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_producer_check_mutex = (sem_t*)shmat(shm_sem_producer_check_mutex, NULL, 0);
	sem_t* p_sem_consumer_check_mutex = (sem_t*)shmat(shm_sem_consumer_check_mutex, NULL, 0);
	sem_t* p_sem_mutex = (sem_t*)shmat(shm_sem_mutex, NULL, 0);
	
	sem_init(p_sem_not_full, 1, count_max);
	sem_init(p_sem_not_empty, 1, 0);
	sem_init(p_sem_producer_check_mutex, 1, 1);
	sem_init(p_sem_consumer_check_mutex, 1, 1);
	sem_init(p_sem_mutex, 1, 1);

	shm_total_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
						  IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_producer_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	shm_consumer_count_id = shmget(IPC_PRIVATE, sizeof(atomic_uint),
                          IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	
	atomic_uint* p_total_count = (atomic_uint*)shmat(shm_total_count_id, NULL, 0);
	atomic_uint* p_count = (atomic_uint*)shmat(shm_count_id, NULL, 0);
	atomic_uint* p_producer_count = (atomic_uint*)shmat(shm_producer_count_id, NULL, 0);
	atomic_uint* p_consumer_count = (atomic_uint*)shmat(shm_consumer_count_id, NULL, 0);
	
	atomic_store(p_total_count, 0);
	atomic_store(p_count, 0);
	atomic_store(p_producer_count, 0);
	atomic_store(p_consumer_count, 0);

	//for (int i = 0; i < 10; i++)
	//	atomic_fetch_add(p_total_count, 1);
	int total_count = atomic_load(p_total_count);	
	printf("Total Count: %d\n", total_count);

	printf("Creating Producers\n");
	for (unsigned i = 0; i < count_producers; i++) {
		printf("\nStarting Producers\n");
		pid = fork();
		if (pid == 0) {
            producer();
            exit(0);
		} else if (pid > 0) {

		} else {
			printf("Process Creation Err\n");
			exit(1);
		}
	}

	printf("Creating Consumers\n");
	for (unsigned i = 0; i < count_consumers; i++) {
		printf("\nStarting Consumer\n");
		int pid = fork();
		if (pid == 0) {
            consumer();
            exit(0);
		} else if (pid > 0) {
		
		} else {
			printf("Process Creation Err\n");
            exit(1);
		}
	}
	
	printf("Waiting For threads to complete\n");
	//for (unsigned i = 0; i < count_producers + count_consumers; i++)
	//	wait(NULL);
	int wpid;
	while ((wpid = wait(NULL)) > 0);
	
	// cleanup	
	sem_destroy(p_sem_not_full);
	sem_destroy(p_sem_not_empty);
	sem_destroy(p_sem_producer_check_mutex);
	sem_destroy(p_sem_consumer_check_mutex);
	sem_destroy(p_sem_mutex);

	printf("Parent Exit\n");
	return 0;
}

void producer() {
	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
    sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_producer_check_mutex = (sem_t*)shmat(shm_sem_producer_check_mutex, NULL, 0);
	sem_t* p_sem_mutex = (sem_t*)shmat(shm_sem_mutex, NULL, 0);
	atomic_uint* p_total_count = (atomic_uint*)shmat(shm_total_count_id, NULL, 0);
	atomic_uint* p_count = (atomic_uint*)shmat(shm_count_id, NULL, 0);
	atomic_uint* p_producer_count = (atomic_uint*)shmat(shm_producer_count_id, NULL, 0);
	usleep(500*1000);

 	while (atomic_load(p_total_count) < MAX_IMAGES) {
		printf("Producer Wait Count: %d\n", atomic_load(p_count));
		sem_wait(p_sem_producer_check_mutex);
			printf("producer counter logic\n");
			if (atomic_load(p_producer_count) >= MAX_IMAGES) {
				sem_post(p_sem_producer_check_mutex);
				break;
			}
			atomic_fetch_add(p_producer_count, 1);
		sem_post(p_sem_producer_check_mutex);

		printf("PRO BEFORE\n");		
		sem_wait(p_sem_not_full);
		printf("PRO AFTER\n");
			sem_wait(p_sem_mutex);
				atomic_fetch_add(p_count, 1);
			sem_post(p_sem_mutex);
        sem_post(p_sem_not_empty);
		unsigned count = atomic_load(p_count);
		unsigned total_count = atomic_load(p_total_count);
		printf("Producer Post Count: %d Total: %d\n", count, total_count);
    }
}

void consumer() {
	sem_t* p_sem_not_full = (sem_t*)shmat(shm_sem_not_full, NULL, 0);
	sem_t* p_sem_not_empty = (sem_t*)shmat(shm_sem_not_empty, NULL, 0);
	sem_t* p_sem_consumer_check_mutex = (sem_t*)shmat(shm_sem_consumer_check_mutex, NULL, 0);
	sem_t* p_sem_mutex = (sem_t*)shmat(shm_sem_mutex, NULL, 0);
	atomic_uint* p_total_count = (atomic_uint*)shmat(shm_total_count_id, NULL, 0);
    atomic_uint* p_count = (atomic_uint*)shmat(shm_count_id, NULL, 0);
	atomic_uint* p_consumer_count = (atomic_uint*)shmat(shm_consumer_count_id, NULL, 0);
	usleep(500*1000);    

	while (atomic_load(p_total_count) < MAX_IMAGES) {
		printf("Consumer Wait Count: %d\n", atomic_load(p_count));
		sem_wait(p_sem_consumer_check_mutex);
			printf("producer counter logic\n");
			if (atomic_load(p_consumer_count) >= MAX_IMAGES) {
				sem_post(p_sem_consumer_check_mutex);
				break;
			}
			atomic_fetch_add(p_consumer_count, 1);
		sem_post(p_sem_consumer_check_mutex);

		sem_wait(p_sem_not_empty);
			sem_wait(p_sem_mutex);
				atomic_fetch_sub(p_count, 1);
				atomic_fetch_add(p_total_count, 1);
			sem_post(p_sem_mutex);
        sem_post(p_sem_not_full);
		unsigned count = atomic_load(p_count);
        unsigned total_count = atomic_load(p_total_count);
        printf("Consumer Post Count: %d Total: %d\n", count, total_count);
	}
}

