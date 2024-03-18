#include "segel.h"
#include "request.h"
#include <pthread.h>
#include <string.h>
#include <sys/time.h>



#include <signal.h>
#include "request.h"
#include "queue.h"
#include <stdarg.h>

int queueSize;
Queue waiting_requests;
Queue handeled_requests;


// HW3: Parse the new arguments too
void getargs(int *port, int *numOfThreads, int * queueSize,int *schedalg, int argc, char *argv[])
{
    if (argc < 5) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
    *numOfThreads = atoi(argv[2]);
    *queueSize = atoi(argv[3]);
    if(strcmp(argv[4], "block") == 0)
    {
        *schedalg = BLOCK;
    }
    else if(strcmp(argv[4], "dt") == 0)
    {
        *schedalg = DT;
    }
    else if(strcmp(argv[4], "dh") == 0)
    {
        *schedalg = DH;
    }
    else if(strcmp(argv[4], "bf") == 0)
    {
        *schedalg = BF;
    }
    else if(strcmp(argv[4], "random") == 0)
    {
        *schedalg = RANDOM;
    }
    else
    {
        perror("Invalid scheduling algorithm");
        exit(1);
    }

}


void handleRequest(workerThread *currentThread)
{
    currentThread->count++;
    requestHandle(currentThread);
    //logEvent("Not crashed yet");
    //logEvent("Worker: Request handled, closing connection");
    
    close(currentThread->currentRequest.connfd);
}


void* threadRoutine(void *arg)
{
    
    workerThread *currentThread = (workerThread*) arg;
    currentThread->dynamicCount = 0;
    currentThread->staticCount = 0;
    currentThread->count = 0;  
    //This is the main routine of the worker thread
    //I want the scope of currenThread to be here in order to handle the request with the statistics and id ...


    //This is used fro debugging
    //printf("Thread %d is alive\n", currentThread->id);

    while(1){
        //this part will probably encapsulate the second aprt
        pthread_mutex_lock(&bufferMutex);
        while(queueIsEmpty(waiting_requests))
        {
            pthread_cond_wait(&workerWakeUp, &bufferMutex);
        }
        struct timeval curr_arrival_time = getHeadArrivalTime(waiting_requests);
        int connfd = dequeue(waiting_requests);
        enqueue(handeled_requests, connfd, curr_arrival_time);
        struct timeval curr_dispatch_time;
        struct timeval curr_time;
        gettimeofday(&curr_time, NULL);
        timersub(&curr_time, &curr_arrival_time, &curr_dispatch_time);
        pthread_mutex_unlock(&bufferMutex);
        
        currentThread->currentRequest.connfd = connfd;
        currentThread->currentRequest.arrivalTime = curr_arrival_time;
        currentThread->currentRequest.dispatchTime = curr_dispatch_time;
        handleRequest(currentThread);
        pthread_mutex_lock(&bufferMutex);
        specificDequeueFd(handeled_requests, connfd);
        pthread_cond_signal(&masterWakeUp);
        if (queueIsEmpty(waiting_requests) && queueIsEmpty(handeled_requests))
        {
            pthread_cond_signal(&masterWakeUpWaitUntilQueueIsEmpty);
        }
        pthread_mutex_unlock(&bufferMutex);


    }
    return NULL;
}


void createThreads(int numofThreads)
{
    
}

    //This routing makes the thread wait for the lock to open using the conditional variable lockStatus


//This is a very dumb function that i 


int main(int argc, char *argv[])
{
    int connfd, port, clientlen, numOfThreads = NUMBEROFWORKERTHREADS, schedalg = BLOCK;
    struct sockaddr_in clientaddr;
    getargs(&port, &numOfThreads, &queueSize, &schedalg, argc, argv);
    
    //queueSize = 100 ;
    //Initializing the mutex
    if (pthread_mutex_init(&bufferMutex, NULL) != 0) {
        // Handle error
        //Chatgbt Loves to save my ass 
        return 1;
    }
    //initializing the condition variables
    if(pthread_cond_init(&masterWakeUpWaitUntilQueueIsEmpty, NULL) != 0)
    {
        perror("Condiiton variable is masterWakeUpWaitUntilQueueIsEmpty not set");
        return -1; 
        //Error has occured
    }
    if(pthread_cond_init(&masterWakeUp, NULL) != 0)
    {
        perror("Condiiton variable is empty not set");
        return -1; 
        //Error has occured
    }
    if(pthread_cond_init(&workerWakeUp, NULL) != 0)
    {
        perror("Condiiton variable is full not set");
        return RETURNERROR; 
        //Error has occured
    }
    //Init of the global variable
    reqbuffer.requests = malloc(sizeof(request)*queueSize);
    reqbuffer.is_empty = TRUE;
    reqbuffer.is_full = FALSE;
    reqbuffer.tail = 0;
    reqbuffer.head = 0;
    waiting_requests = createQueue(queueSize);
    handeled_requests = createQueue(queueSize);
    int continueListen=TRUE;
    //Here we need to create a queue of requests and make it empty
    //Also we need to initialize a fixed number of worker threads
    workerThread* workerThreads = malloc(sizeof(workerThread)* numOfThreads);
    pthread_mutex_lock(&bufferMutex); 
    
    for (int i = 0; i < numOfThreads; i++)
    {
        workerThreads[i].id = i;
        printf("Creating thread %d\n", i);
        if(pthread_create(&workerThreads[i].thread, NULL, threadRoutine, &workerThreads[i]) != 0)
        {
            perror("Failure creating a Thread for some reason!");
            return RETURNERROR; 
        }
        
    }
    pthread_mutex_unlock(&bufferMutex);
    //Threads created.. Now add requests to queue..

    listenfd = Open_listenfd(port);
    while (TRUE) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        //Add the request to the queue
        pthread_mutex_lock(&bufferMutex);
        struct timeval arrival_time;
        gettimeofday(&arrival_time, NULL);
    
        int block  = FALSE;
        while(getCurQueueSize(handeled_requests) + getCurQueueSize(waiting_requests) >= queueSize)
        {
        
            if(schedalg == BLOCK)
            {
                continueListen = false;
            }
            else if(schedalg == DT)
            {
                //Log the dropping of the request 
            
                continueListen = true;
                Close(connfd);
                //Dropping the request by not adding it to the queue
                //Design here is abit off
            }
            else if (schedalg == DH)
            {
            
                if (queueIsEmpty(waiting_requests))
                {
                    Close(connfd);
                    continueListen = true;
                }
                else
                {
                    int connection_to_close = dequeue(waiting_requests);
                    Close(connection_to_close);
                }
                break;
                // I need to drop the head of the queue
            }
            else if (schedalg == BF)
            {
                //BF and Random
                //TODO As bonus.. 
                pthread_cond_wait(&masterWakeUpWaitUntilQueueIsEmpty, &bufferMutex);
                Close(connfd);
                continueListen = true;
            }
            else
            {
                //DROP RANDOMLY HALF THE REQUESTS
                int num_to_drop = (getCurQueueSize(waiting_requests) + 1) / 2;
                int drop_index;
                int drop_connfd;
                if (getCurQueueSize(waiting_requests) == 0)
                {
                    Close(connfd);
                    continueListen = true;
                    break;
                }
                for (int i = 0; i < num_to_drop; i++)
                {
                    drop_index = rand() % getCurQueueSize(waiting_requests);
                    drop_connfd = specificDequeueIndex(waiting_requests, drop_index);
                    Close(drop_connfd);
                }
                break;
                //Delete randomly half the reuests

            }
            
            //This is the block
        }

        
        if (continueListen)
        {
            pthread_mutex_unlock(&bufferMutex);
            continue;
        }
        enqueue(waiting_requests, connfd, arrival_time);
        pthread_cond_signal(&workerWakeUp);
        pthread_mutex_unlock(&bufferMutex);
    }

}


    


 
