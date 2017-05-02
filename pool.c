//
// Created by valyo95 on 23/4/2017.
//
#include <stdio.h>
#include <printf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include "pool.h"
#include "functions.h"

#define BUFFERSIZE 1024

int sigterm;

static void hdl (int sig, siginfo_t *siginfo, void *context)
{
    sigterm = 1;
//    printf("bika ston handler\n");
    return;
}

int mkJobDir(int poolId, int id, pid_t pid, char *err, char *out);

int main(int argc, char **argv)
{
    sigterm = 0;
    struct sigaction act;
    memset (&act, '\0', sizeof(act));
    act.sa_sigaction = &hdl;
    act.sa_flags = SA_SIGINFO;

    int saction = sigaction(SIGTERM, &act, NULL);

    if ( saction< 0) {
        perror ("sigaction");
        return 1;
    }
    if(sigterm == 1)
    {
//            printf("Sigeterm == 1 \n\n");
    }


    int i = 1;
    int poolId = -1;
    int maxJobs = 0;
    int in_pipe=-1, out_pipe=-1;
    char *jms_in = NULL;
    char *jms_out = NULL;

    fd_set rfds;
    FD_ZERO(&rfds);

    pid_t * pids;
    long long int * times;

    int current_jobs = 0;
    while (i < argc) {
        if(strcmp(argv[i], "-i") == 0)
        {
            poolId = atoi(argv[i+1]);       //-i for the logical poolId
        }
        else if(strcmp(argv[i], "-r") == 0)
        {
            jms_out = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            strcpy(jms_out, argv[i + 1]);
        }
        else if(strcmp(argv[i], "-w") == 0)
        {
            jms_in = malloc((strlen(argv[i + 1]) + 1) * sizeof(char));
            strcpy(jms_in, argv[i + 1]);
        }
        else if(strcmp(argv[i], "-n") == 0)
        {
            maxJobs = atoi(argv[i+1]);
        }
        i++;
    }

    if(poolId==-1)
    {
        fprintf(stderr, "Error. Wrong poolId as argument. Exiting pool.\n");
        exit(1);
    }
    if(maxJobs <= 0)
    {
        fprintf(stderr, "Error. MaxJobs in pool must be greater tha 0. Exiting pool.\n");
        exit(1);
    }

    pids = malloc(maxJobs*sizeof(pid_t));
    times = malloc(maxJobs*sizeof(long long int));
    int *statuses = malloc(maxJobs*sizeof(int));

    for (int j = 0; j < maxJobs; ++j) {
        pids[j] = -1;
        statuses[j] = -1;
    }

//    printf("jms_in = %s   jms_out = %s\n\n", jms_in, jms_out);


    out_pipe = open(jms_out, O_RDONLY );
    in_pipe = open(jms_in, O_WRONLY );

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


    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    char line[2000];								//na eimai sigouros oti den tha dwsoun kamia vlakeia megali
    char **word = malloc(10*sizeof(char *));	//To polu 10 le3eis i insert me megethos 20xaraktires to kathena

    int jobsFinished = 0;
    int returnSelect = 0;
    while(sigterm == 0)
    {

        int st;
        if(jobsFinished < maxJobs)
        {
            for (int i = 0; i < maxJobs; ++i) {
                pid_t  wait_exit =  waitpid(-1, &st, WNOHANG);

//            printf("waitExit = %d\n", wait_exit);
                if(wait_exit > 0)
                {
                    for (int j = 0; j < maxJobs; ++j)
                    {
/*
                if(WIFEXITED(st) == 1)
                    printf("child terimanted normally \n");
*/

//                    printf("pids[%d] = %d\n", j, pids[j]);
                        if((int)wait_exit == (int) pids[j])
                        {
                            jobsFinished++;
/*
                        printf("job %d with pid: %d has finished.\n\n\n\n\n", poolId+j, pids[j]);
                        printf("jobsFinished = %d\n", jobsFinished);
*/
                            statuses[j] = FINISHED;
                            char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
                            sprintf(tmpStr, "finished");
                            sprintf(tmpStr, "%s %d %d %lld\n",tmpStr, j, statuses[j], times[j]);

                            if(write(in_pipe, tmpStr, BUFFERSIZE) != BUFFERSIZE) {
                                perror("Pool sending OK for submited proccess: data write error");
                                exit(EXIT_FAILURE);
                            }
                            free(tmpStr);

                        }
                    }
                    if(jobsFinished == maxJobs)
                    {
                        char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
                        sprintf(tmpStr, "%s", pool_finishing);
                        if(write(in_pipe, tmpStr, BUFFERSIZE) != BUFFERSIZE)
                        {
                            perror("Pool sending OK for submited proccess: data write error");
                            exit(EXIT_FAILURE);
                        }
                        free(tmpStr);
                        continue;
                    }
                }
            }
        }
        FD_ZERO(&rfds);
        FD_SET(out_pipe, &rfds);

        returnSelect = select(out_pipe+1,&rfds,NULL,NULL, &tv);
        if(returnSelect == 0)
            continue;

        returnSelect--;

        int ret =read(out_pipe,line,BUFFERSIZE);

        cutLine(line,word);

        if(ret != 0) {
            if(strcmp(line, coord_answer_to_pool) == 0)
            {
                sigterm=1;
                continue;
            }
            if(strcmp(word[0], "submit") == 0)
            {
                pids[current_jobs] = fork();

                if(pids[current_jobs] == 0) {
                    char *err = malloc(BUFFERSIZE*sizeof(char));
                    char *out = malloc(BUFFERSIZE*sizeof(char));
                    mkJobDir(poolId,current_jobs,getpid(), err, out);

                    int fd_out;
                    int fd_err;

                    fd_out = open(out, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
                    fd_err = open(err, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

                    dup2(fd_out,1);
                    dup2(fd_err,2);

                    close(fd_out);
                    close(fd_err);

                    free(out);
                    free(err);



                    execvp(word[1], &word[1]);
                    perror("exec: ");
                    exit(EXIT_FAILURE);
                }
                else
                {
                    statuses[current_jobs] = ACTIVE;
                    times[current_jobs] = time(NULL);

                    char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
                    sprintf(tmpStr, "submit");
                    sprintf(tmpStr, "%s %d %d %d %lld\n",tmpStr, poolId*maxJobs+current_jobs, statuses[current_jobs], pids[current_jobs], times[current_jobs]);
                    if(write(in_pipe, tmpStr, BUFFERSIZE) != BUFFERSIZE)
                    {
                        perror("Pool sending OK for submited proccess: data write error");
                        exit(EXIT_FAILURE);
                    }
                    free(tmpStr);
                }
                current_jobs++;
            }
            else if(strcmp(word[0], "suspend") == 0)
            {
                char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
                int jobId = atoi(word[1]);
                jobId = jobId%maxJobs;
                if(statuses[jobId] == FINISHED)
                    sprintf(tmpStr, "suspend %d\n", -2);
                else if(statuses[jobId] == ACTIVE)
                {
                    int ret = kill(pids[jobId], SIGSTOP);
                    if(ret == 0) {
                        statuses[jobId] = SUSPEND;
                        sprintf(tmpStr, "suspend %d\n", jobId+poolId*maxJobs);
                    }
                    else
                        sprintf(tmpStr, "suspend %d\n", -1);
                }
                else if(statuses[jobId] == SUSPEND)
                {
                    sprintf(tmpStr, "suspend %d\n", -3);
                }

                if(write(in_pipe, tmpStr, BUFFERSIZE) != BUFFERSIZE)
                {
                    perror("Pool sending OK for submited proccess: data write error");
                    exit(EXIT_FAILURE);
                }
                free(tmpStr);
            }
            else if(strcmp(word[0], "resume") == 0)
            {
                char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
                int jobId = atoi(word[1]);
                jobId = jobId%maxJobs;
                if(statuses[jobId] == FINISHED)
                    sprintf(tmpStr, "resume %d\n", -2);
                else if(statuses[jobId] == SUSPEND)
                {
                    int ret = kill(pids[jobId], SIGSTOP);
                    if(ret == 0) {
                        statuses[jobId] = ACTIVE;
                        sprintf(tmpStr, "resume %d\n", jobId+poolId*maxJobs);
                    }
                    else
                        sprintf(tmpStr, "resume %d\n", -1);
                }
                else if(statuses[jobId] == ACTIVE)
                    sprintf(tmpStr, "resume %d\n", -3);

                if(write(in_pipe, tmpStr, BUFFERSIZE) != BUFFERSIZE)
                {
                    perror("Pool sending OK for submited proccess: data write error");
                    exit(EXIT_FAILURE);
                }
                free(tmpStr);
            }
        }
    }
  //  printf("vgika apo while POOL\n\n");

    free(word);

    char *tmpStr1 = malloc(BUFFERSIZE*sizeof(char));
    memset(tmpStr1,0,BUFFERSIZE);
    tmpStr1[0]='\0';

    sprintf(tmpStr1, "shutdown");
    for (int k = 0; k < current_jobs; ++k) {
        if(statuses[k] != FINISHED)
        {
            if(kill(pids[k], SIGTERM) == -1)
                statuses[k] = FINISHED;
        }
        sprintf(tmpStr1,"%s %d", tmpStr1, statuses[k]);
    }
    if(jobsFinished != maxJobs && write(in_pipe, tmpStr1, BUFFERSIZE) != BUFFERSIZE)
    {
        perror("Pool sending OK for submited proccess: data write error");
        exit(EXIT_FAILURE);
    }
    FD_ZERO(&rfds);
    FD_SET(out_pipe, &rfds);
    returnSelect = select(out_pipe+1,&rfds,NULL,NULL, NULL);
//    printf("retSelect = %d\n", returnSelect);
    if(returnSelect == 1)
        if(read(out_pipe,tmpStr1,BUFFERSIZE) <=0)
        {
            perror("pool reading last msg: ");
            exit(EXIT_FAILURE);
        }


    free(tmpStr1);

    free(times);
    free(pids);
    free(statuses);
    close(in_pipe);
    close(out_pipe);

    free(jms_in);
    free(jms_out);
    printf("Exiting pool with id = %d.\n", poolId);

}


int mkJobDir(int poolId, int id, pid_t pid, char *err, char *out)
{
    char *tmpStr = malloc(BUFFERSIZE*sizeof(char));
    if(poolId == 0)
    {
        mkdir("outDir", S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP);
    }
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    sprintf(tmpStr, "outDir/jms_sdi1400049_%d_%d_%.4d%.2d%.2d_%.2d%.2d%.2d", id, pid, tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    mkdir(tmpStr, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP);

    sprintf(err, "%s/stderr_%d", tmpStr, id);
    sprintf(out, "%s/stdout_%d", tmpStr, id);

    free(tmpStr);
    return 0;
}