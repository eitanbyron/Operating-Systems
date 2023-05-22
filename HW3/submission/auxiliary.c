#include "auxiliary.h"
#include <stdio.h>
#include <stdlib.h>
#include "segel.h"
#include "request.h"
#include <stdbool.h>
#include <math.h>

thread_rout rout=&thread_routine;

void QueueDeleteByIndex(Queue queue, int index);


void handleWrapper(int fd, int thread_id, int* req_count, int* stat_count, int* dyn_count, struct timeval* arrival,   
                    struct timeval* dispatch)
{
    requestHandle(fd, thread_id, req_count, stat_count, dyn_count, arrival, dispatch);
    Close(fd);
}

pthread_args argsCreate(WorkerPool wp, int id)
{
    pthread_args args=malloc(sizeof(*args));
    args->wp=wp;
    args->num_of_thread=id;
    return args;
}


void chooseHandling(WorkerPool wp, char* sched)
{
    if(strcmp(sched,"block")==0)
    {
        wp->overload_handler=OVERLOAD_BLOCK;
    }
    else if(strcmp(sched,"dt")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_TAIL;

    }
    else if(strcmp(sched,"dh")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_HEAD;

    }
    else if(strcmp(sched,"random")==0)
    {
        wp->overload_handler=OVERLOAD_DROP_RAND;
    }
}

bool initializer(WorkerPool wp)
{
    if (pthread_mutex_init(&wp->lock_queue,NULL) != 0)
    {
        fprintf(stderr,"pthread_mutex_init() error\n");
        return false;
    }
    pthread_cond_init(&wp->queue_full,NULL);  //always success (tutroyal 8)
    pthread_cond_init(&wp->queue_empty,NULL); //always success (tutroyal 8)
    return true;
}


WorkerPool WorkerPoolCreate(int numOfThreads, int max_queue_size, char* sched)
{
    WorkerPool wp=malloc(sizeof(*wp));
    if (!initializer(wp))
        exit(1);
    wp->running=0;
    wp->numOfThreads=numOfThreads;
    wp->pending=QueueCreate();
    wp->handler=handleWrapper;
    wp->max_queue_size=max_queue_size;
    chooseHandling(wp, sched);
     wp->threads=malloc(sizeof(*wp->threads)*numOfThreads);
    for (int i=0;i<numOfThreads;i++)
    {
        pthread_args args =argsCreate(wp, i);
        pthread_create(&wp->threads[i],NULL,rout,args);
    }

    return wp;
}


QueueResult WorkerPoolDequeue(WorkerPool wp, int thread_id, int* total_req, int* stat_req, int* dyn_req)
{
    if(wp==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    }
    int fd;
    struct timeval arrival;
    struct timeval pick;
    struct timeval result;
    pthread_mutex_lock(&wp->lock_queue);
     while (QueueRemoveHead(wp->pending,&fd,&arrival) != QUEUE_SUCCESS)
    {
        pthread_cond_wait(&wp->queue_empty,&wp->lock_queue);
    }
    gettimeofday(&pick,NULL);
    timersub (&pick,&arrival,&result);
    wp->running++;
    pthread_mutex_unlock(&wp->lock_queue);
    wp->handler(fd, thread_id, total_req, stat_req, dyn_req, &arrival, &result);
    pthread_mutex_lock(&wp->lock_queue);
    wp->running--;
    pthread_cond_broadcast(&wp->queue_full);
    pthread_mutex_unlock(&wp->lock_queue);
    return QUEUE_SUCCESS;
}


QueueResult WorkerPoolEnqueue(WorkerPool wp, int element, struct timeval *arrival)
{
    if(wp==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    }
    QueueResult res;
    pthread_mutex_lock(&wp->lock_queue);
    if(wp->running+QueueGetSize(wp->pending)==wp->max_queue_size)
    {
        switch(wp->overload_handler)
        {
            case OVERLOAD_BLOCK:
                while(wp->running+QueueGetSize(wp->pending)==wp->max_queue_size)
                {
                    pthread_cond_wait(&wp->queue_full, &wp->lock_queue);
                }
                res=QueueAdd(wp->pending, element, arrival);
                pthread_cond_signal(&wp->queue_empty);
                pthread_mutex_unlock(&wp->lock_queue);
                return res;
            case OVERLOAD_DROP_TAIL:
                close(element);
                pthread_mutex_unlock(&wp->lock_queue);
                return QUEUE_SUCCESS;
            case OVERLOAD_DROP_HEAD:
                if(QueueGetSize(wp->pending)==0)
                {
                    close(element);
                    pthread_mutex_unlock(&wp->lock_queue);
                    return QUEUE_SUCCESS;
                } 
                int fd;
                struct timeval tmp;
                QueueRemoveHead (wp->pending,&fd, &tmp);
                close (fd);
                res=QueueAdd(wp->pending, element, arrival);
                pthread_cond_signal(&wp->queue_empty);
                pthread_mutex_unlock(&wp->lock_queue);
                return res;
            case OVERLOAD_DROP_RAND:
                if(QueueGetSize(wp->pending)==0)
                {
                    close(element);
                    pthread_mutex_unlock(&wp->lock_queue);
                    return QUEUE_SUCCESS;
                } 
                int random_index=-1;
                int num_to_delete=ceil(0.5*QueueGetSize(wp->pending));
                while (num_to_delete>0)
                {
                    random_index=rand()%QueueGetSize(wp->pending);
                    QueueDeleteByIndex(wp->pending, random_index);
                    num_to_delete--;
                }
                res=QueueAdd(wp->pending, element, arrival);
                pthread_cond_signal(&wp->queue_empty);
                pthread_mutex_unlock(&wp->lock_queue);
                return res;

             //option for a default case
        } 
    }
    res=QueueAdd(wp->pending,element, arrival);
    pthread_cond_signal(&wp->queue_empty);
    pthread_mutex_unlock(&wp->lock_queue);

    return res;
}

QueueResult WorkerPoolAddConnection(WorkerPool wp, int fd,struct timeval *arrival)
{
    if(wp==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    }
    return WorkerPoolEnqueue(wp,fd, arrival);
}

void* thread_routine(pthread_args args)
{
    int static_counter = 0, dynamic_counter =0 , request_counter = 0;
    while(true)
    {
        WorkerPoolDequeue(args->wp, args->num_of_thread, &request_counter, &static_counter, &dynamic_counter);
    }
}


// ******************************* Node Implementation *****************************************
Node NodeCreate(int data, Node next, struct timeval *arrival)
{
    Node new_node=malloc(sizeof(*new_node));
    if(new_node==NULL)
    {
        return NULL;
    }
    new_node->connection_descriptor=data;
    new_node->next=next;
    new_node->prev=NULL;
    if(next!=NULL)
    {
        next->prev=new_node;
    }
    new_node->arrival=*arrival;
    return new_node;
}


Node getNodeByIndex(Queue queue, int index)
{
    if(queue==NULL || index > queue->size)
    {
        return NULL;
    }
    Node iter=queue->first;
    int i;
    for( i=0 ; i < queue->size ; i++)
    {
        if(i==index)
        {
            return iter;
        }
        if(iter->next==NULL) return NULL;
        iter=iter->next;
    }
    return NULL;
}

// ******************************* Queue Implementation *****************************************

Queue QueueCreate()
{
    Queue queue=malloc(sizeof(*queue));
    if(queue==NULL)
    {
        return NULL;
    }
    queue->first=NULL;
    queue->last=NULL;
    queue->size=0;
    return queue;
}

QueueResult QueueAdd (Queue queue,int element, struct timeval *arrival)
{

    if(queue==NULL)
    {
        return QUEUE_NULL_ARGUMENT;
    };
    Node new_node=NodeCreate(element,queue->first,arrival);
    if(new_node==NULL)
    {
        return QUEUE_ADD_FAILED;
    }
    queue->first=new_node;
    if(queue->size==0)
    {
        queue->last=new_node;
    }
    queue->size++;
    return QUEUE_SUCCESS;
}

QueueResult QueueRemoveHead (Queue queue, int* fd, struct timeval* arrival)
{
    if (queue == NULL)
        return QUEUE_NULL_ARGUMENT;
    if (QueueGetSize(queue) == 0)
        return QUEUE_EMPTY;
    Node before_last =queue->last->prev;
    if (before_last != NULL)
    {
        *arrival = queue->last->arrival;
        *fd = queue->last->connection_descriptor;
        free (queue->last);
        before_last->next=NULL;
        queue->last=before_last;
    }
    else
    {
        *arrival =queue->first->arrival;
        *fd=queue->first->connection_descriptor;
        free(queue->first);
        queue->first=NULL;
        queue->last=NULL;
    }
    queue->size--;
    return QUEUE_SUCCESS;
}

void QueueDeleteByIndex(Queue queue, int index)
{
    if(queue==NULL)
    {
        return;
    }
    if(index==0)
    {
        int to_remove;
        struct timeval temp;
        QueueRemoveHead(queue,&to_remove,&temp);
        close(to_remove);
        return;
    }

    Node to_remove=getNodeByIndex(queue,index);
    if(to_remove==NULL)return;

    if(to_remove==queue->last)
    {
        queue->last=to_remove->prev;
        to_remove->prev->next=NULL;
    }else{
        to_remove->prev->next=to_remove->next;
        to_remove->next->prev=to_remove->prev;
    }

    close(to_remove->connection_descriptor);
    free(to_remove);
    queue->size--;
}

int QueueGetSize(Queue queue)
{
    if(queue==NULL)
    {
        return -1;
    }
    return queue->size;
}

