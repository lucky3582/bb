#include<stdio.h>
#include<pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <string.h>
#include<unistd.h>
#include<fcntl.h>

//TODO Define global data structures to be used

static int tid_count;
char *str = "kumar is a good and talent young boy from uottawa canada and andhra pradesh india china pakisatan sdavwds dvs dvs dv s bdfs dv sd vs fdbsd vbs bs fdb sd va s  r e v sfbs b sb as fdb sf bv swa er wa  db sb sa b sb sdb wa br sbv s b wa aw ba w bvaw ebw aeb wa ebvaw b wab aw ebwa ba wbv aw bvaw vwa b wb wab wab aw b wab wb awb awb wa bwa b wab awb awb wa baw b wrb rwb wb wrb wa bw b wb wrb awbr wab rwb wa b wrb wrb wrb wr bwr bw rb wrb wrb wrb rbr b rwb wrb rwb rwb r wab rwb wrb wbr rwb";
int done = 0;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;
char* ptr;
char* pt;
pthread_mutex_t mutex;
int byte_count;

const int SIZE = 1024;
const char* name;

struct buffer_details {
        char* buffer;
        int bufferSizeInBytes;
};

struct read_details {

    char *buffer;
    int *bufferSizeInBytes;

};

void at_exit()
{
pthread_mutex_destroy(&mutex);
shm_unlink(name);
pthread_cond_destroy(&c);
munmap(ptr,SIZE);
}

void process_data(char* buffer, int bufferSizeInBytes){
    printf("In Reading: %p\n",*&buffer);
    printf("In Reading: %d",bufferSizeInBytes);
    for(int i=0;i<bufferSizeInBytes-1;i++)
    {
    printf("%c", buffer[i]);
}
printf("\n");
}

void get_external_data(char *buffer, int bufferSizeInBytes)
{
memcpy(ptr, buffer, bufferSizeInBytes);
printf("In Writinh:\n %d",bufferSizeInBytes);
printf("Data written %s:\n",buffer);
ptr += bufferSizeInBytes;

memset(ptr++,' ',1);
byte_count+=bufferSizeInBytes+1;
printf("%p\n",*&ptr);
}

/**
* This thread is responsible for pulling data off of the shared data
* area and processing it using the process_data() API.
*/


void *reader_thread(void *arg) {
//TODO: Define set-up required
//while(1) {
//TODO: Define data extraction (queue) and processing
//}

pthread_mutex_lock(&mutex);
while (done == 0)
{
pthread_cond_wait(&c, &mutex);
}

struct read_details *details = (struct read_details*) arg;
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
//TODO: Define set-up required
//while(1) {
//TODO: Define data extraction (device) and storage
//}
//

pthread_mutex_lock(&mutex);

struct buffer_details *details = (struct buffer_details*) arg;
get_external_data( details->buffer , details->bufferSizeInBytes);
pthread_cond_broadcast(&c);
done =1;

pthread_mutex_unlock(&mutex);

pthread_exit(0);

}


#define M 10
#define N 20

int main(int argc, char **argv) {
int i;
int count = 0;
pthread_t tids[M+N];
int tid_count = 0;

name = "OSI";

/* shared memory faile descriptor */
int shm_fd;

/* create the shared memory object */
shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);

/* configure the size of the shared memory object */
ftruncate(shm_fd, SIZE);

/* memory map the shared memory object */
ptr = mmap(0, SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);

pt=ptr;
if(pt==MAP_FAILED)
{
printf("read failed");
}

if(ptr==MAP_FAILED)
{
printf("write failed");
}
printf("PT:%p\n", ptr);
printf("PTR:%p\n",*&ptr);

pthread_mutex_init(&mutex, NULL);

//launch N threads
for(i = 0; i < N; i++) {

struct read_details details;
details.buffer = pt;
details.bufferSizeInBytes = &byte_count;
pthread_create(&tids[tid_count], NULL, reader_thread, &details);
tid_count++;
}

//launch M threads
for(i = 0; i < M; i++) {
struct buffer_details details;
details.buffer = str;
details.bufferSizeInBytes = 130;
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
