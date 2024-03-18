#ifndef QUEUE_H
#define QUEUE_H

#include "segel.h"
// typedefining so things are readable
typedef struct Queue_t *Queue;
typedef struct Request_t *Request;

// Converting our code to bool since the staff don't want us including bool
#define true 1
#define false 0
//-------------------------------------------
Request newRequest(int descriptor, struct timeval arrival_time);

Queue createQueue(int max_queue_size);

void enqueue(Queue queue, int descriptor, struct timeval arrival_time);
int dequeue(Queue queue);
int specificDequeueIndex(Queue queue, int num);
int specificDequeueFd(Queue queue, int fd);

int getCurQueueSize(Queue queue);
int queueIsEmpty(Queue queue);

struct timeval getHeadArrivalTime(Queue queue);

// -------------- Our Structs ---------------
struct thread_stats
{
    int thread_index;
    int count;
    int static_count;
    int dynamic_count;
};

struct request_stats
{
    struct timeval arrival_time;
    struct timeval dispatch_time;
};
//-------------------------------------------

#endif // QUEUE_H
