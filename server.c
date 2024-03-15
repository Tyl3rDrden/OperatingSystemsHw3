#include "segel.h"
#include "request.h"
#include <pthread.h>
#include <string.h>
#include <sys/time.h>



#include <signal.h>




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
    struct request req;
    if(reqbuffer.head == reqbuffer.tail)
    {
        reqbuffer.is_empty = TRUE;
        req.connfd = RETURNERROR;
        //Return garbage value
        return req;
    }
    else
    {
        req = reqbuffer.requests[reqbuffer.tail];
        gettimeofday(&req.dispatchTime, NULL);
        reqbuffer.tail = (reqbuffer.tail + 1) % QUEUESIZE; //Increment the tail to get a fifo extraction
        reqbuffer.is_full = FALSE;
        pthread_cond_signal(&masterWakeUp);
        //Returns a copy of the struct request
        return req;
    }
}
int addRequest(int connfd)
{
    if((reqbuffer.head + 1) % QUEUESIZE == reqbuffer.tail)
    {
        reqbuffer.is_full = TRUE;
        return RETURNERROR;
        //We are full Return error code
    }
    else
    {
        reqbuffer.requests[reqbuffer.head].connfd = connfd;
        gettimeofday(&reqbuffer.requests[reqbuffer.head].arrivalTime, NULL);
        reqbuffer.head = (reqbuffer.head + 1) % QUEUESIZE;
        reqbuffer.is_empty = FALSE;
        pthread_cond_signal(&workerWakeUp);
        return 0; //Success
    }
}

void overRideLastElement(int connfd)
{
    if(reqbuffer.is_empty)
    {
        perror("Buffer is empty, cannot override last element");
        return;
    }
    close(reqbuffer.requests[reqbuffer.tail].connfd); //Closing the connection to the last element
    reqbuffer.requests[reqbuffer.tail].connfd = connfd;
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
    requestHandle(currentThread);
    currentThread->count++;
    Close(currentThread->currentRequest.connfd);
}

struct request workerSingleThreadRoutine()
{
        pthread_mutex_lock(&bufferMutex);
        while(reqbuffer.is_empty)
        {
            pthread_cond_wait(&workerWakeUp, &bufferMutex);
            //Check that the buffer is not empty and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        struct request req = getRequest();
        pthread_mutex_unlock(&bufferMutex);
        return req;
        

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
        currentThread->currentRequest = workerSingleThreadRoutine();
        if(currentThread->currentRequest.connfd != RETURNERROR)
        {
            handleRequest(currentThread);
        }
    }
}




    //This routing makes the thread wait for the lock to open using the conditional variable lockStatus


int main(int argc, char *argv[])
{
    int connfd, port, clientlen, numOfThreads = NUMBEROFWORKERTHREADS, queueSize = 100, schedalg = BLOCK;
    struct sockaddr_in clientaddr;

    getargs(&port, &numOfThreads, &queueSize, &schedalg, argc, argv);

    //Initializing the mutex

    signal(SIGINT, handle_sigint);



    if (pthread_mutex_init(&bufferMutex, NULL) != 0) {
        // Handle error
        //Chatgbt Loves to save my ass 
        return 1;
    }


    //initializing the condition variables
    
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


        int block  = FALSE;
        while(reqbuffer.is_full)
        {
            if(schedalg == BLOCK)
            {
                pthread_cond_wait(&masterWakeUp, &bufferMutex);
            }
            else if(schedalg == DT)
            {
                block = TRUE;
                close(connfd);
                //Dropping the request by not adding it to the queue
                //Design here is abit off
                break;
            }
            else if (schedalg == DH)
            {
                overRideLastElement(connfd);
                block = TRUE;
                break;
                // I need to drop the head of the queue
            }
            else
            {
                //BF and Random
                //TODO As bonus.. 
                perror("Not implemented yet");
                return 0;
            
            }
            
            //This is the block
        }
        if(block == FALSE)
        {
            addRequest(connfd);
        }
        
        pthread_mutex_unlock(&bufferMutex);
    }

}


    


 
