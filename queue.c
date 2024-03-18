#include "queue.h"
struct Queue_t
{
    int max_queue_size;
    int cur_queue_size;
    Request head_queue;
    Request tail_queue;
};
//-----------------------------------------------------------------
struct Request_t
{
    int descriptor;
    struct timeval arrival_time;
    Request next;
};

Request newRequest(int descriptor, struct timeval arrival_time)
{
    Request new_request = (Request)malloc(sizeof(*new_request));
    new_request->descriptor = descriptor;
    new_request->arrival_time = arrival_time;
    new_request->next = NULL;
    return new_request;
}
Queue createQueue(int max_queue_size)
{
    Queue new_queue = (Queue)malloc(sizeof(*new_queue));
    new_queue->head_queue = NULL;
    new_queue->tail_queue = NULL;
    new_queue->cur_queue_size = 0;
    new_queue->max_queue_size = max_queue_size;
    return new_queue;
}

void enqueue(Queue queue, int descriptor, struct timeval arrival_time)
{
    Request new_request = newRequest(descriptor, arrival_time);
    if (queue->cur_queue_size == queue->max_queue_size)
    {
        return;
    }
    if (queueIsEmpty(queue) != true)
    {
        queue->tail_queue->next = new_request;
        queue->tail_queue = new_request;
        queue->cur_queue_size++;
    }
    else
    {
        queue->head_queue = new_request;
        queue->tail_queue = new_request;
        queue->cur_queue_size++;
    }
}

int dequeue(Queue queue)
{
    int head_descriptor = queue->head_queue->descriptor;
    Request tmp = queue->head_queue->next;
    free(queue->head_queue);
    if (tmp == NULL)
    {
        queue->tail_queue = NULL;
        queue->head_queue = NULL;
        queue->cur_queue_size--;
    }
    else
    {
        queue->head_queue = tmp;
        queue->cur_queue_size--;
    }
    return head_descriptor;
}

int specificDequeueIndex(Queue queue, int num)
{
    // assume element num exists in the queue
    if (num >= getCurQueueSize(queue) || num < 0)
    {
        return -1;
    }
    if (num == 0)
    {
        return dequeue(queue);
    }
    Request prev_wanted_request = NULL;
    Request wanted_request = queue->head_queue;
    for (int i = 0; i < num; i++)
    {
        prev_wanted_request = wanted_request;
        wanted_request = wanted_request->next;
    }

    int wanted_descriptor = wanted_request->descriptor;
    prev_wanted_request->next = wanted_request->next;
    free(wanted_request);
    if (num == getCurQueueSize(queue) - 1)
    {
        queue->tail_queue = prev_wanted_request;
    }
    queue->cur_queue_size--;
    return wanted_descriptor;
}

int specificDequeueFd(Queue queue, int fd)
{
    Request wanted_request = queue->head_queue;
    int i = 0;
    while (wanted_request != NULL)
    {
        if (fd == wanted_request->descriptor)
        {
            return specificDequeueIndex(queue, i);
        }
        i++;
        wanted_request = wanted_request->next;
    }

    return -1;
}
//-------------------------------------------
int getCurQueueSize(Queue queue)
{
    return queue->cur_queue_size;
}
//-------------------------------------------
int queueIsEmpty(Queue queue)
{
    if (queue->cur_queue_size == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
//-------------------------------------------
struct timeval getHeadArrivalTime(Queue queue)
{
    if (queueIsEmpty(queue) != true)
    {
        return queue->head_queue->arrival_time;
    }
    else
    {
        return (struct timeval){0};
    }
}
