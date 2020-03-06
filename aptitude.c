#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>

//Global data structures used

static int tid_count; //for unique thread id's
char *device = "kumar"; //dummy device for reading 

int read_access = 0; //condition variable to check read access
pthread_cond_t write_condition = PTHREAD_COND_INITIALIZER;

char* shared_buffer_ptr;
char* read_ptr;

pthread_mutex_t mutex;

int byte_count; //To track the amount of data written

const int SIZE = 1024;
const char* name;

struct buffer_details {
    char* buffer;
    int bufferSizeInBytes;
};

struct reader_buffer_details {
    char *buffer;
    int *bufferSizeInBytes;
};

#define M 10
#define N 20

/**
 * This function is called at exit making sure
 * all the opened resources are closed 
*/

void at_exit() {
    pthread_mutex_destroy(&mutex);
    shm_unlink(name);
    pthread_cond_destroy(&write_condition);
    munmap(shared_buffer_ptr,SIZE);
}

/**
 * This function is stub for processing the data in the
 * shared memory. It simply prints the data in the buffer
*/

void process_data(char* buffer, int bufferSizeInBytes) {
    printf("Reading Data: ");
    for(int i=0;i<bufferSizeInBytes-1;i++) {
        printf("%c", buffer[i]);
    }
    printf("\n");
}

/**
 * This function is stub for writing the data received from
 * the device into the shared buffer
*/

int get_external_data(char *buffer, int bufferSizeInBytes) {
   if(shared_buffer_ptr+bufferSizeInBytes < shared_buffer_ptr+SIZE)
   {
    memcpy(shared_buffer_ptr, buffer, bufferSizeInBytes);
    printf("Data written: %s\n",buffer);
    shared_buffer_ptr += bufferSizeInBytes;
    memset(shared_buffer_ptr++,' ',1);
    byte_count+=bufferSizeInBytes+1;
    return bufferSizeInBytes+1;
   }
   else
   {
       return -1;
   }
}

/**
* This thread is responsible for pulling data off of the shared data
* area and processing it using the process_data() API.
*/

void *reader_thread(void *arg) {

    pthread_mutex_lock(&mutex);
    while (read_access == 0) {
    pthread_cond_wait(&write_condition, &mutex);
    }

    struct reader_buffer_details *details = (struct reader_buffer_details*) arg;
    process_data(details->buffer,*details->bufferSizeInBytes);
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);

}

/**
* This thread is responsible for pulling data from a device using
* the get_external_data() API and placing it into a shared area
* for later processing by one of the reader threads.
*/

void *writer_thread(void *arg) {

    pthread_mutex_lock(&mutex);
    struct buffer_details *details = (struct buffer_details*) arg;
    if(get_external_data( details->buffer , details->bufferSizeInBytes) == -1) {
    at_exit();
    exit(EXIT_FAILURE);
    }
    pthread_cond_broadcast(&write_condition);
    read_access = 1;
    pthread_mutex_unlock(&mutex);
    pthread_exit(0);
}

int main(int argc, char **argv) {
    int i;
    int count = 0;
    pthread_t tids[M+N];
    int tid_count = 0;
    byte_count=0;
    name = "BB";
    int shm_fd;
    int *byte = &byte_count;
    /* create the shared memory object */
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0777);

    if( shm_fd == -1 ) {
        fprintf( stderr, "Open failed:%s\n", strerror( errno ) );
        at_exit();
        return EXIT_FAILURE;
    }

    if( ftruncate( shm_fd, SIZE) == -1 ) {
        fprintf( stderr, "ftruncate: %s\n",strerror( errno ) );
        at_exit();
        return EXIT_FAILURE;
    }


    /* configure the size of the shared memory object */
    //ftruncate(shm_fd, SIZE);

    /* memory map the shared memory object */
    shared_buffer_ptr = mmap(0, SIZE,
                                PROT_WRITE | PROT_READ,
                                MAP_SHARED, shm_fd, 0);

    if(shared_buffer_ptr==MAP_FAILED) {
        fprintf(stderr, "mmap failed: %s\n", strerror(errno));
        at_exit();
        return EXIT_FAILURE;
    }

    read_ptr=shared_buffer_ptr;

    if(pthread_mutex_init(&mutex, NULL) != 0)
    {
        fprintf(stderr, "pthread init failed: %s\n", strerror(errno));
        at_exit();
        return EXIT_FAILURE;
    };
    struct reader_buffer_details details[N];
    //launch N threads
    for(i = 0; i < N; i++) {

        details[i].buffer = read_ptr;
        details[i].bufferSizeInBytes = byte; //will be replaced in real method
        pthread_create(&tids[tid_count], NULL, reader_thread, &details[i]);
        tid_count++;
    }

    //launch M threads
    for(i = 0; i < M; i++) {
    struct buffer_details details;
    details.buffer = device; //location of device for reading
    details.bufferSizeInBytes = 5; //mock value
    pthread_create(&tids[tid_count], NULL, writer_thread, &details);
    tid_count++;

    }

    // Wait till all threads finish
    for (int i = 0; i < tid_count; i++) {
    pthread_join(tids[i], NULL);
    }

    at_exit();
    return 0;
}
