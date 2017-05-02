//
//
// Created by valyo95 on 14/4/2017.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <wait.h>
#include "pool.h"
#include "functions.h"

#define BUFFERSIZE 1024
char *printStatus(int status)
{
    char *str;
    if(status == FINISHED)
        str = strdup("FINISHED");
    else if (status == ACTIVE)
        str = strdup("ACTIVE");
    else if (status == SUSPEND)
        str = strdup("SUSPENDED");
    else
    {
        str = strdup("fuckking error in printStatus()\n\n");
        printf("%s status = %d\n", str, status);
    }
    return str;
}

char *showPools(List *l);
char *showFinished(List *l);
char *submitAnswer(char *l, List *list, Node *tmp);
char *statusAll(List *l, int wordcount, int time);
char *showActive(List *l);
char *finish(List *l);

int main(int argc, char **argv)
{

    int i = 1;
    int max_jobs = -1;
    char *jms_in = NULL;
    char *jms_out = NULL;
    char *path = NULL;
    int in_pipe=-1, out_pipe=-1;

    if(argc < 9) {
        fprintf(stderr, "Error not enough arguments to run the programm.\n");
        return 1;
    }


    while (i < argc) {
//        printf("%s ", argv[i]);
        if(strcmp(argv[i], "-w") == 0) {
            jms_in = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            strcpy(jms_in, argv[i + 1]);
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            jms_out = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            strcpy(jms_out, argv[i + 1]);
        }
        else if(strcmp(argv[i], "-l") == 0)
        {
            path = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            strcpy(path, argv[i + 1]);
        }
        else if(strcmp(argv[i], "-n") == 0)
        {
            max_jobs = atoi(argv[i+1]);
        }
        i++;
    }
/*
    printf("\n");
    printf("Max jobs for a pool = %d\n", max_jobs);
*/
    if(max_jobs==-1)
    {
        fprintf(stderr, "Error! Max jobs not specificied.\n");
        return 1;
    }
    if(jms_in == NULL || jms_out == NULL || path == NULL)
    {
        fprintf(stderr, "Error! Wrong arguments.\n");
        return 1;
    }
    if(strcmp(jms_in, jms_out) == 0) {
        fprintf(stderr, "Error! FIFO_in must be different from FIFO_out.\n");
        return 1;
    }

    in_pipe = open(jms_in, O_WRONLY | O_NONBLOCK);
    out_pipe = open(jms_out, O_RDONLY | O_NONBLOCK);

//    printf(" Coord: in pipe = %d\nout pipe = %d\n", in_pipe, out_pipe);

    if(in_pipe == -1)
    {
        perror("in_pipe: ");
        exit(EXIT_FAILURE);
    }
    if(out_pipe == -1)
    {
        perror("out_pipe: ");
        exit(EXIT_FAILURE);
    }


    List* l = createList(max_jobs);
    Node *tmpNode;


    fd_set rfds;
    FD_ZERO(&rfds);

    int out_files = out_pipe;
    int last_pool = -1;
    int files_to_read= -1;

    char line[BUFFERSIZE];								//na eimai sigouros oti den tha dwsoun kamia vlakeia megali
    char **word = malloc(10*sizeof(char *));	//To polu 10 le3eis i insert me megethos 20xaraktires to kathena

    int allPoolFinished = 0;
    int reads=0;
    int shutdown = 0;
    while(allPoolFinished == 0)
    {
        tmpNode = l->head;
        FD_ZERO(&rfds);
        int finished = 0;
        while(tmpNode != NULL)
        {
            if(tmpNode->pool->finished != 1)
                FD_SET(tmpNode->pool->fd_out,&rfds);
            else
                finished++;
            tmpNode = tmpNode->next;
        }

        if(finished == l->size && shutdown == 1)
        {
//            printf("ara eimai edw\n\n");
            char *tmpStr1 = finish(l);
            if(write(in_pipe ,tmpStr1, BUFFERSIZE) != BUFFERSIZE)
            {
                perror("coord to console: data write error");
                exit(EXIT_FAILURE);
            }
            free(tmpStr1);
            allPoolFinished = 1;
            continue;
        }

        FD_SET(out_pipe,&rfds);

/*
        printf("\n\n\n\n\n\n\n\n\nEdww\n\n\n\n\n\n\n\n\n");
        printf("\n\n\n\n\n\n\n\n\nEkeiiiiii------->\n");
*/

//        printf("bainw select\n");
        files_to_read = select(out_files+1, &rfds, NULL, NULL, NULL);
        reads++;
//        printf("vgika apo select me files to read = %d\n", files_to_read);

        if(FD_ISSET(out_pipe, &rfds) != 0)
        {
            read(out_pipe, line, BUFFERSIZE);
/*
            printf("Data is available now from console.\n\n");
            printf("Data readed from console = %s\n", line);
*/
            int wordCount = cutLine(line,word);
/*
            for (int j = 0; j < wordCount; ++j) {
                printf("word[%d] = %s (%zu)\n", j, word[j], strlen(word[j]));
            }
*/

            if(strcmp(word[0], "submit")==0)
            {
//                printf("Gonna submit now.\n");

                if(l->size == 0 || l->last->pool->currentJobs == max_jobs)
                {
//                    printConsole("Gonna create the new pool.");
                    addPool(l);
                    last_pool = l->size-1;
//                    printList(l);

                    l->last->pool->in = malloc(20*sizeof(char));
                    sprintf(l->last->pool->in, "pool%d_in", l->last->pool->poolId);
                    l->last->pool->out = malloc(20*sizeof(char));
                    sprintf(l->last->pool->out, "pool%d_out",l->last->pool->poolId);

                    if(mkfifo(l->last->pool->in ,0666) < 0)
                    {
                        perror("pool_in mkfifo: ");
                        exit(EXIT_FAILURE);
                    }
                    if(mkfifo(l->last->pool->out ,0666) < 0)
                    {
                        perror("pool_out mkfifo: ");
                        exit(EXIT_FAILURE);
                    }

                    pid_t newpid = fork();
                    if(newpid == 0)
                    {
                        char *nextPool = malloc(20*sizeof(char));
                        char *mxj = malloc(20*sizeof(char));
                        my_itoa(last_pool, &nextPool);
                        my_itoa(max_jobs, &mxj);

                        execlp("./pool", "./pool", "-i", nextPool, "-n", mxj, "-w", l->last->pool->out, "-r", l->last->pool->in ,NULL);
                        free(nextPool);
                        free(mxj);
                        perror("exec: ");
                        exit(EXIT_FAILURE);
                    }

                    l->last->pool->mypid = newpid;
                    l->last->pool->fd_in = open(l->last->pool->in, O_WRONLY );
                    l->last->pool->fd_out = open(l->last->pool->out, O_RDONLY );


                    FD_SET(l->last->pool->fd_out, &rfds);

                    out_files = l->last->pool->fd_out;
                    l->last->pool->currentJobs++;
                }
                else if (l->last->pool->currentJobs < max_jobs)
                    l->last->pool->currentJobs++;

                if(write(l->last->pool->fd_in ,line, BUFFERSIZE) != BUFFERSIZE)
                {
                    perror("coord to pool: data write error");
                    exit(EXIT_FAILURE);
                }
                l->current_jobs++;
            }
            else if(strcmp(word[0],"status") == 0)
            {
                int ret;
                int jobId = atoi(word[1]);
                int poolId = jobId / l->max_jobs;
                jobId = jobId%l->max_jobs;

//                printf("-------Status------\n\n");;
                ret = chechJob(poolId, jobId, l);

                if( ret == 1 )
                {
                    tmpNode = l->head;
                    while(tmpNode->pool->poolId != poolId) {
                        tmpNode = tmpNode->next;
                    }

                    char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
                    memset(tmpStr1,0,BUFFERSIZE);

                    if(tmpNode->pool->statuses[jobId] == FINISHED)
                        sprintf(tmpStr1, "JobId %d Status: %s\n", poolId*max_jobs+jobId, "FINISHED");
                    else if(tmpNode->pool->statuses[jobId] == ACTIVE)
                        sprintf(tmpStr1, "JobId %d Status: ACTIVE (running for %lld sec)\n", poolId*max_jobs+jobId, time(NULL) - tmpNode->pool->times[jobId]);
                    else
                        sprintf(tmpStr1, "JobId %d Status: %s\n", poolId*max_jobs+jobId, "SUSPENDED");

                    if(write(in_pipe ,tmpStr1, BUFFERSIZE) != BUFFERSIZE)
                    {
                        perror("coord to console: data write error");
                        exit(EXIT_FAILURE);
                    }
                    free(tmpStr1);

                }
                else if(ret == FINISHED)
                {
                    char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
                    memset(tmpStr1,0,BUFFERSIZE);
                    sprintf(tmpStr1, "JobId %s Status: %s\n", word[1], "FINISHED");
                    if(write(in_pipe, tmpStr1, BUFFERSIZE) != BUFFERSIZE)
                    {
                        perror("coord to console: data write error");
                        exit(EXIT_FAILURE);
                    }
                    free(tmpStr1);
                }
                else
                {
                    if(write(in_pipe, "Error. No such job.\n", BUFFERSIZE) != BUFFERSIZE)
                    {
                        perror("coord to console: data write error");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if(strcmp(word[0],"status-all") == 0 || strcmp(word[0],"show-active") == 0 || strcmp(word[0],"show-finished") == 0 || strcmp(word[0],"show-pools") == 0)
            {
                char *tmpStr1;

                if(strcmp(word[0],"status-all") == 0)
                    tmpStr1 = statusAll(l, wordCount, (wordCount == 2) ? atoi(word[1]) : -1);

                else if(strcmp(word[0],"show-active") == 0)
                    tmpStr1 = showActive(l);

                else if(strcmp(word[0],"show-finished") == 0)

                    tmpStr1 = showFinished(l);

                else if(strcmp(word[0],"show-pools") == 0)
                    tmpStr1 = showPools(l);

                if(write(in_pipe ,tmpStr1, BUFFERSIZE) != BUFFERSIZE)
                {
                    perror("coord to console: data write error");
                    exit(EXIT_FAILURE);
                }
                free(tmpStr1);
            }
            else if(strcmp(word[0],"suspend") == 0 || strcmp(word[0],"resume") == 0)
            {
                    char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
                    int ret;
                    int jobId = atoi(word[1]);
                    int poolId = jobId / l->max_jobs;
                    jobId = jobId%l->max_jobs;

                    ret = chechJob(poolId,jobId,l);

                    if(ret == 1)
                    {
                        sprintf(tmpStr1, "%s",line);
                        Node *tmpNode1 = l->head;
                        while(tmpNode1 != NULL)
                        {
                            if(poolId == tmpNode1->pool->poolId)
                            {
                                if(write(tmpNode1->pool->fd_in, tmpStr1, BUFFERSIZE) != BUFFERSIZE)
                                {
                                    perror("console to pool: write error");
                                    exit(EXIT_FAILURE);
                                }
                                break;
                            }
                            tmpNode1 = tmpNode1->next;
                        }
                    }
                    else if(ret == FINISHED)
                    {
                        sprintf(tmpStr1, "Job has finished\n");
                        if(write(in_pipe,tmpStr1,BUFFERSIZE) != BUFFERSIZE)
                        {
                            perror("console to console: write error");
                            exit(EXIT_FAILURE);
                        }
                    }
                    else if(ret == 0)
                    {
                        sprintf(tmpStr1, "No such job.\n");
                        if(write(in_pipe,tmpStr1,BUFFERSIZE) != BUFFERSIZE)
                        {
                            perror("console to console: write error");
                            exit(EXIT_FAILURE);
                        }
                    }
                    free(tmpStr1);
            }
            else if(strcmp(word[0],"shutdown") == 0)
            {
                Node *tmpNode1 = l->head;
                while(tmpNode1 != NULL)
                {
                    if(tmpNode1->pool->finished != 1)
                        kill(tmpNode1->pool->mypid, SIGTERM);
                    tmpNode1 = tmpNode1->next;
                }
                shutdown =1;
            }
            for (int j = 0; j < wordCount; ++j) {
                free(word[j]);
            }
        }
        else
        {
            int readedFiles = 0;
            while(readedFiles < files_to_read)
            {
                tmpNode = l->head;
                while(tmpNode != NULL)
                {
                    if(tmpNode->pool->finished != 1 && FD_ISSET(tmpNode->pool->fd_out,&rfds) != 0)
                    {
                        FD_CLR(tmpNode->pool->fd_out, &rfds);
                        readedFiles+=1;

                        read(tmpNode->pool->fd_out, line, BUFFERSIZE);
                        cutLine(line,word);

//                        printf("Line readed from pool: %s\n", line);
                        /*---------------Edw to pool zita na kanei exit-----------*/
                        /*--------------------------------------------------------*/
                        if(strcmp(line,pool_finishing) == 0)
                        {
                            char *tmpStr1 = strdup(coord_answer_to_pool);
                            if(write(tmpNode->pool->fd_in, tmpStr1, BUFFERSIZE) != BUFFERSIZE)
                            {
                                perror("coord to console: data write error");
                                exit(EXIT_FAILURE);
                            }
                            free(tmpStr1);
                            int st;
                            waitpid(tmpNode->pool->mypid,&st,0);
                            if(WIFEXITED(st) == 0)
                            {
                                perror("pool did not terminate normally: ");
                                exit(EXIT_FAILURE);
                            }

                            tmpNode->pool->finished = 1;

                            l->finishedPools++;

                            FD_CLR(tmpNode->pool->fd_out, &rfds);
                            close(tmpNode->pool->fd_out);
                            close(tmpNode->pool->fd_in);
                            remove(tmpNode->pool->in);
                            remove(tmpNode->pool->out);
                            for (int j = 0; j < tmpNode->pool->maxJobs; ++j) {
                                tmpNode->pool->statuses[j] = FINISHED;
                            }
                        }
                        if(strcmp(word[0],"finished") == 0)
                        {
                            tmpNode->pool->statuses[atoi(word[1])] = FINISHED;
                        }
                        else if(strcmp(word[0],"submit") == 0)
                        {
                            char *tmpStr1 = submitAnswer(line, l, tmpNode);

                            if(write(in_pipe, tmpStr1, BUFFERSIZE) != BUFFERSIZE )
                            {
                                perror("coord to console: data write error");
                                exit(EXIT_FAILURE);
                            }
                            free(tmpStr1);
                        }
                        else if(strcmp(word[0],"suspend") == 0)
                        {
                            char *firsWord = malloc(BUFFERSIZE*sizeof(char));
                            int job;
                            sscanf(line,"%s %d", firsWord, &job);
                            char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
                            if(job >= 0)
                            {
                                tmpNode->pool->statuses[job-tmpNode->pool->maxJobs*tmpNode->pool->poolId] = SUSPEND;
                                sprintf(tmpStr1, "JobId %d SUSPENDED\n", job);
                            }
                            else if(job == -1)
                                sprintf(tmpStr1, "Problem with suspending JobId\n");
                            else if(job == -2)
                                sprintf(tmpStr1, "JobId has finished\n");
                            else if(job == -3)
                                sprintf(tmpStr1, "JobId is already SUSPENDED\n");

                            if(write(in_pipe, tmpStr1, BUFFERSIZE) != BUFFERSIZE )
                            {
                                perror("coord to console: data write error");
                                exit(EXIT_FAILURE);
                            }
                            free(tmpStr1);
                        }
                        else if(strcmp(word[0],"resume") == 0)
                        {
                            char *firsWord = malloc(BUFFERSIZE*sizeof(char));
                            int job;
                            sscanf(line,"%s %d", firsWord, &job);
                            char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
                            if(job >= 0)
                            {
                                tmpNode->pool->statuses[job-tmpNode->pool->maxJobs*tmpNode->pool->poolId] = ACTIVE;
                                sprintf(tmpStr1, "JobId %d RESUMED\n", job);
                            }
                            else if(job == -1)
                                sprintf(tmpStr1, "Problem with resuming JobId\n");
                            else if(job == -2)
                                sprintf(tmpStr1, "JobId has finished\n");
                            else if(job == -3)
                                sprintf(tmpStr1, "JobId is already ACTIVE\n");

                            if(write(in_pipe, tmpStr1, BUFFERSIZE) != BUFFERSIZE )
                            {
                                perror("coord to console: data write error");
                                exit(EXIT_FAILURE);
                            }
                            free(tmpStr1);
                        }
                        else if(strcmp(word[0],"shutdown") == 0)
                        {
//                            printf("ola kala\n");
                            char *tmpStr1 = strdup(line);
                            char *tmpStr2 = tmpStr1;

                            char *firstword = malloc(BUFFERSIZE*sizeof(char));

                            sscanf(tmpStr1, "%s ", firstword);
                            tmpStr1+= (strlen(firstword)+1  )*sizeof(char);
//                            printf("line: %s\n", tmpStr1);
                            free(firstword);
                            for (int j = 0; j < tmpNode->pool->currentJobs; ++j)
                            {
                                sscanf(tmpStr1, "%d", &tmpNode->pool->statuses[j]);
                                tmpStr1+= sizeof(int)  + 0*sizeof(char);

/*
                                printf("line: %s\n", tmpStr1);
                                printf("tmpNode->pool->statuses[j] = %d\n", tmpNode->pool->statuses[j]);
*/


                            }
//                            printf("vgikaa\n\n");
                            if(write(tmpNode->pool->fd_in, tmpStr2, BUFFERSIZE) != BUFFERSIZE )
                            {
                                perror("coord to pool last msg: data write error");
                                exit(EXIT_FAILURE);
                            }
                            close(tmpNode->pool->fd_out);
                            close(tmpNode->pool->fd_in);
                            remove(tmpNode->pool->in);
                            remove(tmpNode->pool->out);
                            free(tmpStr2);
                            tmpNode->pool->finished = 1;

//                            printf("ola POLI POLI kala\n");
                        }
//                        printf("ekei?\n\n");
                    }
//                    printf("edw?\n\n");
                    tmpNode = tmpNode->next;
               }
//                printf("parapera?\n\n");

            }
//            printf("akoma pio pera\n");
        }
//        printf("poso pia???\n\n");
    }

    free(word);

    close(in_pipe);
    close(out_pipe);

    free(jms_in);
    free(jms_out);
    printf("Cooordinator exitng\n");
}


char *showFinished(List *l)
{
    Node *tmpNode = l->head;
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);

    if(tmpNode == NULL)
    {
        sprintf(ret, "No FINISHED jobs cause no jobs were submitted.\n");
        return ret;
    }

    sprintf(ret, "Finished jobs:\n");
    int k = 1;
    while(tmpNode != NULL)
    {
        for (int j = 0; j < tmpNode->pool->currentJobs; ++j)
        {
            if(tmpNode->pool->statuses[j] == FINISHED)
                sprintf(ret, "%s%d. JobId %d\n", ret,k++ , tmpNode->pool->poolId*tmpNode->pool->maxJobs + j);
        }
        tmpNode = tmpNode->next;
    }
    return ret;
}


char *showActive(List *l)
{
    Node *tmpNode = l->head;
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);

    if(tmpNode == NULL)
    {
        sprintf(ret, "No ACTIVE jobs cause no jobs were submitted.\n");
        return ret;
    }

    sprintf(ret, "Active jobs:\n");
    int k = 1;
    while(tmpNode != NULL)
    {
        for (int j = 0; j < tmpNode->pool->currentJobs; ++j)
        {
            if(tmpNode->pool->statuses[j] == ACTIVE)
                sprintf(ret, "%s%d. JobId %d\n", ret,k++ , tmpNode->pool->poolId*tmpNode->pool->maxJobs + j);
        }
        tmpNode = tmpNode->next;
    }
    return ret;
}
char *statusAll(List *l, int wordcount, int timee)
{
    Node *tmp = l->head;
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);
    if(tmp == NULL)
    {
        sprintf(ret, "No job submitted.\n");
        return ret;
    }

    char ***pool = malloc(l->size*sizeof(char**));
    for (int j = 0; j < l->size; ++j) {
        pool[j] = malloc(l->max_jobs*sizeof(char*));
    }
    int k = 1;
    int num=0;
    while(tmp != NULL)
    {
        for (int i = 0; i < tmp->pool->currentJobs; ++i) {
            if((wordcount==1) || (wordcount ==2 && time(NULL) - tmp->pool->times[i]  <= timee))
            {
                pool[tmp->pool->poolId][i] = malloc(100*sizeof(char));
                sprintf(pool[tmp->pool->poolId][i] , "%d. JobId %d Status: %s " ,k++, tmp->pool->poolId*tmp->pool->maxJobs+i, printStatus( tmp->pool->statuses[i])); // time: %lld \n",  ,tmp->pool->times[i]);
                if(tmp->pool->statuses[i] == ACTIVE)
                    sprintf(pool[tmp->pool->poolId][i], "%s(running for %lld sec)\n", pool[tmp->pool->poolId][i],  (long long int)time(NULL)-tmp->pool->times[i]);
                else
                    sprintf(pool[tmp->pool->poolId][i] , "%s\n", pool[tmp->pool->poolId][i]);

                sprintf(ret,"%s%s", ret, pool[tmp->pool->poolId][i]);
                free(pool[tmp->pool->poolId][i]);
                num++;
            }
        }
        tmp = tmp->next;
    }
    for (int j = 0; j < l->size; ++j) {
        free(pool[j]);
    }
    free(pool);
    if(num == 0)
        sprintf(ret, "No such jobs\n");
    return ret;
}

char *submitAnswer(char *l, List *list, Node *tmp)
{
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);
    int id;
    int status;
    int pid;
    long long int time;
    char *sbm =malloc(BUFFERSIZE*sizeof(char));
    sscanf(l, "%s %d %d %d %lld\n", sbm, &id, &status, &pid, &time);
    sprintf(ret, "JobId: %d, PID: %d\n", id, pid);

    tmp->pool->pids[id-tmp->pool->poolId*tmp->pool->maxJobs] = pid;
    tmp->pool->statuses[id-tmp->pool->poolId*tmp->pool->maxJobs] = status;
    tmp->pool->times[id-tmp->pool->poolId*tmp->pool->maxJobs] = time;

    free(sbm);
    return ret;
}

char *showPools(List *l)
{
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);

    Node *tmp = l->head;

    if(tmp== NULL)
    {
        sprintf(ret, "No jobs were submitted.\n");
        return ret;
    }

    sprintf(ret, "Pools & NumOfJobs\n");
    int p = 1;
    while(tmp != NULL)
    {
        int jobs = 0;
        sprintf(ret,"%s%d. ", ret, p++);
        for (int i = 0; i < tmp->pool->currentJobs; ++i) {
            if(tmp->pool->statuses[i] == ACTIVE)
                jobs++;
        }
        sprintf(ret, "%s%d %d", ret, tmp->pool->mypid, jobs);
        if(tmp->pool->finished == 1)
            sprintf(ret, "%s (pool finished)\n", ret);
        else
            sprintf(ret, "%s\n", ret);

        tmp = tmp->next;
    }
    return ret;
}

char *finish(List *l)
{
    char *ret = malloc(BUFFERSIZE*sizeof(char));
    memset(ret,0,BUFFERSIZE);

    Node *tmp = l->head;

    if(tmp== NULL)
    {
        sprintf(ret, "No jobs were submitted.\n");
        return ret;
    }

    sprintf(ret, "Pools & NumOfJobs\n");
    int p = 1;
    int jobs = 0;
    int jobsActive = 0;
    while(tmp != NULL)
    {
        sprintf(ret,"%s%d. ", ret, p++);
        for (int i = 0; i < tmp->pool->currentJobs; ++i)
        {
            jobs++;
            if(tmp->pool->statuses[i] != FINISHED)
                jobsActive++;
        }
        tmp = tmp->next;
    }
    sprintf(ret, "Served %d jobs, %d were still in progress\n", jobs, jobsActive);
    return ret;
}