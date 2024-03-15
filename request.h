#ifndef __REQUEST_H__

//void requestHandle(int fd);
#define NUMBEROFWORKERTHREADS 4
#define QUEUESIZE 100
#define FALSE 0
#define TRUE 1
#define RETURNERROR -1

enum schedalg {BLOCK , DT, DH, BF, RANDOM};

pthread_mutex_t bufferMutex;

//Coniditional Variables for the buffer
pthread_cond_t masterWakeUp; 
pthread_cond_t workerWakeUp; 

//Global variavles

typedef struct request {
    int connfd;
    struct timeval arrivalTime; //To be filled out by the main thread
    struct timeval dispatchTime; //To be filled out by the worker thread

} request;

int listenfd; 
typedef struct requestsBuffer {
    struct request requests[QUEUESIZE]; // Initialize all elements to 0
    int head;
    int tail;
    int is_empty; // This is essentially a booleaen 
    int is_full;
} requestsBuffer;

requestsBuffer reqbuffer;

typedef struct workerThread {
    pthread_t thread;// For now until i add more fields
    int id;
    int count;
    int dynamicCount;
    int staticCount;
    struct timeval currentRequestArrivalTime;
    struct request currentRequest;
} workerThread;


void requestHandle(workerThread *currentThread);

#endif
