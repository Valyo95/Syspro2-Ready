//
// Created by valyo95 on 23/4/2017.
//

#ifndef UNTITLED_POOL_H
#define UNTITLED_POOL_H

#define ACTIVE 100
#define FINISHED 200
#define SUSPEND -100

#define OK 777

#include <fcntl.h>

char pool_finishing[] = "Pool has finished. Please send signal to terminate myself.\n";
char coord_answer_to_pool[] = "Go terminate yourself kid.\n";

typedef struct {
    pid_t mypid;
    int poolId;
    int readed;
    int currentJobs;
    int maxJobs;
    int *statuses;
    pid_t *pids;
    long long int * times;

    char finished;
    char *in;
    char *out;
    int fd_in;
    int fd_out;

}Pool;

typedef struct Node{
    Pool* pool;
    struct Node *next;
}Node;

typedef struct List{
    int size;
    int finishedPools;
    int readFromAllPools;
    int current_jobs;
    int max_jobs;
    Node *head;
    Node *last;
}List;

List* createList(int maxj)
{
    List *l = malloc(1*sizeof(struct List));
    l->max_jobs = maxj;
    l->head= NULL;
    l->last=NULL;
    l->size=0;
    l->finishedPools = 0;
    l->readFromAllPools = 0;
    l->current_jobs = 0;
    return l;
}

Pool *createPool(int poolId, int maxJobs)
{
    Pool *p = malloc(1*sizeof(Pool));
    p->mypid = -1;
    p->readed = -1;
    p->poolId = poolId;
    p->currentJobs = 0;
    p->maxJobs = maxJobs;
    p->statuses = malloc(maxJobs*sizeof(int));
    p->pids = malloc(maxJobs*sizeof(pid_t));
    p->times = malloc(maxJobs*sizeof(long long int));
    for (int i = 0; i < maxJobs; ++i) {
        p->statuses[i] = -1;
        p->pids[i] = -1;
        p->times[i] = -1;
    }
    p->finished = 0;
    p->fd_in = -1;
    p->fd_out = -1;
    p->in = NULL;
    p->out = NULL;
    return p;
}

void deletePool(Pool **p)
{
    free(*p);
    free((*p)->in);
    free((*p)->out);
    free((*p)->statuses);
    free((*p)->times);
    free((*p)->pids);
    *p = NULL;
}

Node * createNode(int poolId, int maxJobs)
{
    Node *n = malloc(1*sizeof(Node));
    Pool *p = createPool(poolId, maxJobs);
    n->pool = p;
    n->next = NULL;
    return n;
}

void addPool(List *l)
{
    Node *n = createNode(l->size, l->max_jobs);
    if(l->size == 0)
        l->head = n;
    else
        l->last->next = n;

    l->last = n;

    l->size++;
}

void printPool(Pool *p)
{
    if(p)
    {
        printf("poolPid: %d ",p->mypid);
        printf("poolId: %d ",p->poolId);
        printf("currJobs: %d ",p->currentJobs);
        printf("finished: %d\n", p->finished);
    }
    return;
}
void printNode(Node *n)
{
    if(n)
    {
        printPool(n->pool);
        if(n->next)
        {
            printf("-----Next---->  ");
            printNode(n->next);
        }
    }
    return;
}

void printList(List *l)
{
    if(l && l->head)
    {
        printf("--List--\npools: %d, currentJobs: %d\n", l->size, l->current_jobs);
        printNode(l->head);
    }
    return;
}

int chechJob(int poolId, int jobId,List *l)
{
    if(poolId < l->size)
    {
        Node *tmpNode = l->head;
        while(tmpNode->pool->poolId != poolId) {
            tmpNode = tmpNode->next;
        }
        if(tmpNode->pool->finished == 1)
        {
            printf("edw epistrefw\n");
            return FINISHED;
        }
        if(jobId < tmpNode->pool->currentJobs)
            return 1;
        else
            return 0;
    }
    else
        return 0;
}


#endif //UNTITLED_POOL_H
