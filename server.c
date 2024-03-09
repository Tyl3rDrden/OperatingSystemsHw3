#include "segel.h"
#include "request.h"
#include <pthread.h>


#define NUMBEROFWORKERTHREADS 4
#define QUEUESIZE 100


pthread_mutex_t bufferMutex;

//Coniditional Variables for the buffer
bool is_empty = true; //This is a conditional variable that is used to make the worker threads sleep and not ask for the mutex lock
bool is_full = false; //This is a conditional variable that is used to make the main thread sleep and not ask for the mutex lock


typedef struct requestsBuffer {
    int requests[QUEUESIZE] = {0}; // Initialize all elements to 0
    int head = 0;
    int tail = 0;
} requestsBuffer reqBuffer;

int getRequest()
{
    if(reqbuffer.head == reqbuffer.tail)
    {
        is_empty = false;
        return -1;
    }
    else
    {
        int request = reqbuffer.requests[reqbuffer.tail];
        reqbuffer.tail = (reqbuffer.tail + 1) % QUEUESIZE; //Increment the tail to get a fifo extraction
        return request;
    }
}
int addRequest(int connfd)
{
    if((reqbuffer.head + 1) % QUEUESIZE == reqbuffer.tail)
    {
        is_full = true;
        return -1;
        //We are full Return error code
    }
    else
    {
        reqbuffer.head = (reqbuffer.head + 1) % QUEUESIZE;
        reqbuffer.requests[reqbuffer.head] = connfd;
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
        while(is_empty)
        {
            pthread_cond_wait(&is_empty, &bufferMutex);
            //Check that the buffer is not empty and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        int request = getRequest();
        is_full = false;
        pthread_mutex_unlock(&bufferMutex);
        if(request != -1)
        {
            handleRequest(request);
        }

}
void threadRoutine()
{
    while(1){
            //Check that the buffer is not full and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
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


    if (pthread_mutex_init(&mutex, NULL) != 0) {
        // Handle error
        //Chatgbt Loves to save my ass 
        return 1;
    }


    //initializing the condition variables
    
    if(!pthread_cond_init(is_empty, NULL))
    {
        perror("Condiiton variable is empty not set");
        return -1; 
        
        //Error has occured
    }

    if(!pthread_cond_init(is_full, NULL))
    {
        perror("Condiiton variable is full not set");
        return -1; 
        
        //Error has occured
    }


    //Here we need to create a queue of requests and make it empty
    //Also we need to initialize a fixed number of worker threads
    int threadIds[NUMBEROFWORKERTHREADS];
    for (int i = 0; i < NUMBEROFWORKERTHREADS; i++)
    {
        if(!pthread_create(threadIds + i, NULL, threadRoutine, NULL))
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
        while(is_full)
        {
            pthread_cond_wait(&is_full, &bufferMutex);
            //Check that the buffer is not full and wait on the conditional variable
            //Ask for resource, get the request, release the resource and startthe route later
        }
        addRequest(connfd);
        is_empty = false;
        pthread_mutex_unlock(&bufferMutex);
        pthread_cond_signal(&is_empty);
    }

}


    


 
