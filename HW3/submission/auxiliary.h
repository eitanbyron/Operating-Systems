#include <sys/time.h>
#include <pthread.h>


typedef struct Node_t* Node;
typedef struct Queue_t *Queue;
typedef struct WorkerPool_t *WorkerPool;
typedef struct pthread_args_t *pthread_args;


typedef void(*runHandler)(int, int, int*, int*, int*, struct timeval*, struct timeval*);

typedef enum QueueResult_t {
    QUEUE_SUCCESS,
    QUEUE_NULL_ARGUMENT,
    QUEUE_EMPTY,
    QUEUE_ADD_FAILED
} QueueResult;

struct Node_t {
    int connection_descriptor; 
    struct timeval arrival;
    struct Node_t* next;
    struct Node_t* prev;
};

typedef enum Overload_t {
    OVERLOAD_BLOCK,
    OVERLOAD_DROP_TAIL,
    OVERLOAD_DROP_HEAD,
    OVERLOAD_DROP_RAND
} Overload;

struct Queue_t { 
   struct Node_t* first;
   struct Node_t* last;
   int size;
};

struct WorkerPool_t {
    Queue pending;
    pthread_t* threads;
    pthread_cond_t queue_empty;
    pthread_cond_t queue_full;
    pthread_mutex_t lock_queue;
    int running;
    int max_queue_size;
    int numOfThreads;
    runHandler handler;
    Overload overload_handler;
};

struct pthread_args_t  {
    WorkerPool wp;
    int num_of_thread;
};



Node NodeCreate(int data, Node next, struct timeval* arrival); 
Node getNodeByIndex(Queue queue, int index);

Queue QueueCreate();
int QueueGetSize(Queue queue);
QueueResult QueueAdd (Queue queue,int element, struct timeval* arrival);
QueueResult QueueRemoveHead (Queue queue, int* head, struct timeval* arrival);
void QueueDeleteByIndex(Queue queue, int index);


WorkerPool WorkerPoolCreate(int numOfThreads, int max_queue_size, char* sched);
QueueResult WorkerPoolAddConnection(WorkerPool wp, int fd,struct timeval *arrival);
void* thread_routine(pthread_args args);
typedef void*(*thread_rout)(void*);

