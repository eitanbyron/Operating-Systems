#include "segel.h"
#include "request.h"
#include "auxiliary.h"

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

void getargs(int *port, int argc, char *argv[],int* numOfThreads, int* queueSize, char* sched)
{
    if (argc < 2) {
	fprintf(stderr, "Usage: %s <port>\n", argv[0]);
	exit(1);
    }
    strcpy(sched,argv[4]);
     *queueSize=atoi (argv[3]);
    *numOfThreads=atoi (argv[2]);
     *port = atoi(argv[1]);
}


int main(int argc, char *argv[])
{
    int listenfd, connfd, port, clientlen, numOfThreads, queueSize;
    char sched[10];
    struct sockaddr_in clientaddr;
    getargs(&port, argc, argv,&numOfThreads, &queueSize, sched);
     
    WorkerPool work_pool=WorkerPoolCreate(numOfThreads,queueSize, sched);

    listenfd = Open_listenfd(port);
    while (1) {
	clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, (socklen_t *) &clientlen);
    struct timeval curr_time;
    gettimeofday (&curr_time,NULL);
    WorkerPoolAddConnection(work_pool,connfd,&curr_time);
    }

}
