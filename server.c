#include "segel.h"
#include "request.h"
#include <pthread.h>
#include <string.h>
#include <sys/time.h>



#include <signal.h>


#include <stdarg.h>



void clearLogFile() {
    FILE* logFile = fopen("server.log", "w");
    if (logFile == NULL) {
        perror("Error opening log file");
        return;
    }
    fclose(logFile);
}

void logEvent(const char* format, ...) {
    FILE* logFile = fopen("server.log", "a");
    if (logFile == NULL) {
        perror("Error opening log file");
        return;
    }

    va_list args;
    va_start(args, format);
    vfprintf(logFile, format, args);
    va_end(args);

    fprintf(logFile, "\n");
    fclose(logFile);
}


int queueSize;

void handle_sigint(int sig) 
{
    // Close the listening socket
    close(listenfd);

    // Exit the program
    //printf("Closed the listening socket\n");
    exit(0);
}

struct request getRequest()
{
    struct request req = { 0, {0, 0}, {0, 0}};
    
    if(reqbuffer.head == reqbuffer.tail)
    {
        logEvent("Worker: Queue is empty, cannot get request ");
        reqbuffer.is_empty = TRUE;
        reqbuffer.is_full = FALSE;
        req.connfd = RETURNERROR;
        pthread_cond_signal(&masterWakeUp);
        //Return garbage value
        return req;
    }
    else
    {
        //printf("Getting request from the queue %d is the tail and %d is the head \n\n",reqbuffer.tail +1, reqbuffer.head);
        
        req = reqbuffer.requests[reqbuffer.tail];
        gettimeofday(&req.dispatchTime, NULL);
        req.dispatchTime.tv_sec -= req.arrivalTime.tv_sec;
        req.dispatchTime.tv_usec -= req.arrivalTime.tv_usec; 
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        logEvent("Dispatch time is %lu.%06lu , current time is %lu.%lu", req.dispatchTime.tv_sec, req.dispatchTime.tv_usec, currentTime.tv_sec, currentTime.tv_usec);
        reqbuffer.tail = (reqbuffer.tail + 1); //Increment the tail to get a fifo extraction
        reqbuffer.is_full = FALSE;
        //pthread_cond_broadcast(&masterWakeUp);
        logEvent("Worker: Getting request tail %d from the queue ", reqbuffer.tail-1);
        //Returns a copy of the struct request
        return req;
    }
}
int addRequest(int connfd)
{
    //printf("Adding request to the queue %d is the tail and %d is the head \n\n",reqbuffer.tail, reqbuffer.head +1 );
    if((reqbuffer.head + 1) == reqbuffer.tail)
    {
        logEvent("Buffer is full, cannot add request");
        reqbuffer.is_full = TRUE;
        reqbuffer.is_empty = FALSE;
        return RETURNERROR;
        //We are full Return error code
    }
    else
    {
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);
        logEvent("Adding request to the queue %d is the tail and %d is the head , currenttime %lu.%lu \n\n",reqbuffer.tail, reqbuffer.head +1, currentTime.tv_sec, currentTime.tv_usec);
        reqbuffer.requests[reqbuffer.head].connfd = connfd;
        gettimeofday(&reqbuffer.requests[reqbuffer.head].arrivalTime, NULL);
        reqbuffer.head = (reqbuffer.head + 1);
        reqbuffer.is_empty = FALSE;
        pthread_cond_signal(&workerWakeUp);
        //Can be broadcast
        return 0; //Success
    }
}

void overRideLastElement(int connfd)
{
    //The queue has to be full since we obtain the lock then checked that it is full 
    /*
    if(reqbuffer.is_empty)
    {
        logEvent("Master: Buffer is empty, cannot override last element\n");
        pthread_cond_signal(&master);
        perror("Buffer is empty, cannot override last element");
        return;
    }*/
    logEvent("Master: Overriding last element in the queue");
    close(reqbuffer.requests[reqbuffer.tail].connfd); //Closing the connection to the last element
    reqbuffer.requests[reqbuffer.head].connfd = connfd;
    gettimeofday(&reqbuffer.requests[reqbuffer.head].arrivalTime, NULL);
}


// 
// server.c: A very, very simple web server
//
// To run:
//  ./server <portnum (above 2000)>
//
// Repeatedly handles HTTP requests sent to this port number.
// Most of the work is done within routines written in request.c
//

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
/*
struct request workerSingleThreadRoutine()
{
        
        

}*/


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
        while(reqbuffer.is_empty)
        {
            pthread_cond_wait(&workerWakeUp, &bufferMutex);
            logEvent("Thread Wit id %d, is alive\n", currentThread->id);
            //Check that the buffer is not empty and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        currentThread->currentRequest = getRequest();
        logEvent("Not dead yet");
        pthread_cond_signal(&masterWakeUp);
        pthread_mutex_unlock(&bufferMutex);
        if(currentThread->currentRequest.connfd != RETURNERROR)
        {
            //logEvent("Worker: Handling request fd is %d, %d", currentThread->currentRequest.connfd);
            handleRequest(currentThread);
            
        }
    }
}




    //This routing makes the thread wait for the lock to open using the conditional variable lockStatus


//This is a very dumb function that i 
void deleteRandomHalf() {
    int originalSize = (reqbuffer.head - reqbuffer.tail + queueSize) % queueSize;
    int newSize = originalSize / 2;

    for (int i = 0; i < originalSize; i++) {
        double randNum = (double)rand() / (double)RAND_MAX;

        if (randNum < 0.5) {
            close(reqbuffer.requests[reqbuffer.tail].connfd);
            reqbuffer.tail = (reqbuffer.tail + 1) % queueSize;
            newSize--;

            // If we've deleted enough elements, stop
            if (newSize == 0) {
                break;
            }
        }
    }
}


int main(int argc, char *argv[])
{
    clearLogFile();
    int connfd, port, clientlen, numOfThreads = NUMBEROFWORKERTHREADS, schedalg = BLOCK;
    struct sockaddr_in clientaddr;
    queueSize = 100;
    getargs(&port, &numOfThreads, &queueSize, &schedalg, argc, argv);
    
    //For debugging purposes
    logEvent("Server Started with arguments: port: %d, numOfThreads: %d, queueSize: %d, schedalg: %d", port, numOfThreads, queueSize, schedalg);

    //queueSize = 100 ;
    //Initializing the mutex
    signal(SIGINT, handle_sigint);
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

    //Here we need to create a queue of requests and make it empty
    //Also we need to initialize a fixed number of worker threads
    workerThread* workerThreads = malloc(sizeof(workerThread)* numOfThreads);

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
    //Threads created.. Now add requests to queue..

    listenfd = Open_listenfd(port);
    while (TRUE) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        //Add the request to the queue
        pthread_mutex_lock(&bufferMutex);

        logEvent("++++++++++Master attained lock, adding request to the queue++++++++++ ");

        int block  = FALSE;
        while(reqbuffer.is_full)
        {
            logEvent("Queue is full");
            if(schedalg == BLOCK)
            {
                logEvent("Master: Blocking the request and waiting for the the wakup signal from the worker threads");
                pthread_cond_signal(&workerWakeUp);
                pthread_cond_wait(&masterWakeUp, &bufferMutex);
            }
            else if(schedalg == DT)
            {
                //Log the dropping of the request 
                logEvent("Master: Dropping the request");
                block = TRUE;
                Close(connfd);
                //Dropping the request by not adding it to the queue
                //Design here is abit off
            }
            else if (schedalg == DH)
            {
                logEvent("Master: Dropping the head of the queue");
                overRideLastElement(connfd);
                block = TRUE;
                // I need to drop the head of the queue
            }
            else if (schedalg == BF)
            {
                logEvent("Master: Blocking the request wainting for queue to be empty");
                //BF and Random
                //TODO As bonus.. 
                pthread_cond_wait(&masterWakeUpWaitUntilQueueIsEmpty, &bufferMutex);
            }
            else
            {
                //DROP RANDOMLY HALF THE REQUESTS
                logEvent("Master: Dropping randomly half the requests");
                deleteRandomHalf();
                break;
                //Delete randomly half the reuests

            }
            
            //This is the block
        }
        if(block == FALSE)
        {
            logEvent("Master: Adding request to the queue");
            addRequest(connfd);
        }
        
        pthread_mutex_unlock(&bufferMutex);
    }

}


    


 
