#include "segel.h"
#include "request.h"
#include <pthread.h>


#define NUMBEROFWORKERTHREADS 4
#define QUEUESIZE 100
#define FALSE 0
#define TRUE 1
#define RETURNERROR -1

pthread_mutex_t bufferMutex;

//Coniditional Variables for the buffer
pthread_cond_t masterWakeUp; 
pthread_cond_t workerWakeUp; 

//Global variavles


typedef struct requestsBuffer {
    int requests[QUEUESIZE]; // Initialize all elements to 0
    int head;
    int tail;
    int is_empty; // This is essentially a booleaen 
    int is_full;
} requestsBuffer;

requestsBuffer reqbuffer;

int getRequest()
{
    if(reqbuffer.head == reqbuffer.tail)
    {
        reqbuffer.is_empty = TRUE;
        return RETURNERROR;
    }
    else
    {
        int request = reqbuffer.requests[reqbuffer.tail];
        reqbuffer.tail = (reqbuffer.tail + 1) % QUEUESIZE; //Increment the tail to get a fifo extraction
        reqbuffer.is_full = FALSE;
        pthread_cond_signal(&masterWakeUp);
        return request;
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
        reqbuffer.requests[reqbuffer.head] = connfd;
        reqbuffer.head = (reqbuffer.head + 1) % QUEUESIZE;
        reqbuffer.is_empty = FALSE;
        pthread_cond_signal(&workerWakeUp);
        return 0; //Success
    }
}


typedef struct workerThread {
    pthread_t thread;
    int id;
};



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
void getargs(int *port, int argc, char *argv[])
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    *port = atoi(argv[1]);
}


void handleRequest(int connfd)
{
    requestHandle(connfd);
    Close(connfd);
}

void workerSingleThreadRoutine()
{
        pthread_mutex_lock(&bufferMutex);
        while(reqbuffer.is_empty)
        {
            pthread_cond_wait(&workerWakeUp, &bufferMutex);
            //Check that the buffer is not empty and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        int request = getRequest();
        pthread_mutex_unlock(&bufferMutex);
        if(request != RETURNERROR)
        {
            handleRequest(request);
        }

}
void* threadRoutine()
{
    while(1){
        //this part will probably encapsulate the second aprt
        workerSingleThreadRoutine();
    }
}




    //This routing makes the thread wait for the lock to open using the conditional variable lockStatus


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;


    getargs(&port, argc, argv);

    //Initializing the mutex


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
        return -1; 
        
        //Error has occured
    }
    //Init of the global variable
    reqbuffer.is_empty = TRUE;
    reqbuffer.is_full = FALSE;
    reqbuffer.tail = 0;
    reqbuffer.head = 0;

    //Here we need to create a queue of requests and make it empty
    //Also we need to initialize a fixed number of worker threads
    int threadIds[NUMBEROFWORKERTHREADS];
    for (int i = 0; i < NUMBEROFWORKERTHREADS; i++)
    {
        if(pthread_create(threadIds + i, NULL, threadRoutine, NULL) != 0)
        {
            perror("Routine error");
            return -1; 
        }
        
    }
    //Threads created.. Now add requests to queue..
    listenfd = Open_listenfd(port);
    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
        //Add the request to the queue
        pthread_mutex_lock(&bufferMutex);
        while(reqbuffer.is_full)
        {
            pthread_cond_wait(&masterWakeUp, &bufferMutex);
            //Check that the buffer is not full and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        addRequest(connfd);
        pthread_mutex_unlock(&bufferMutex);
    }

}


    


 
