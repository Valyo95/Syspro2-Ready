#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include "functions.h"
#include "pool.h"

#define BUFFERSIZE 1024


int checkKeyWords(char *str);



int main(int argc, char **argv)
{
    char *max_jobs_str = "3";
    int i=1;
     int coord_pid = -1;
    char *jms_in = NULL;
    char *jms_out = NULL;
    char *op_file = NULL;

    int in_pipe=-1, out_pipe=-1;

    if(argc <7)
    {
        fprintf(stderr, "Error not enough arguments to run the programm.\n");
        return 1;
    }

    while(i<argc)
    {
        if(strcmp(argv[i],"-w") == 0)
        {
            jms_in = malloc((strlen(argv[i+1])+1)*sizeof(char));
            strcpy(jms_in, argv[i+1]);
        }
        else if(strcmp(argv[i],"-r") == 0)
        {
            jms_out = malloc((strlen(argv[i+1])+1)*sizeof(char));
            strcpy(jms_out, argv[i+1]);
        }
        else if(strcmp(argv[i],"-o") == 0)
        {
            op_file = malloc((strlen(argv[i+1])+1)*sizeof(char));
            strcpy(op_file, argv[i+1]);
        }
    i++;
	}
    if(jms_in == NULL || jms_out == NULL || op_file == NULL)
    {
        fprintf(stderr, "Error! Wrong arguments.\n");
        return 1;
    }
    if(strcmp(jms_in,jms_out) == 0)
    {
        fprintf(stderr, "Error! FIFO_in must be different from FIFO_out.\n");
        return 1;
    }
    if(strcmp(jms_in,op_file) == 0)
    {
        fprintf(stderr, "Error! FIFO_in must be different from operation_file.\n");
        return 1;
    }
    if(strcmp(op_file,jms_out) == 0)
    {
        fprintf(stderr, "Error! FIFO_out must be different from operation_file.\n");
        return 1;
    }

    if(mkfifo(jms_in,0666) < 0)
    {
        perror("jms_in mkfifo: ");
        exit(EXIT_FAILURE);
    }
    if(mkfifo(jms_out,0666) < 0)
    {
        perror("jms_out mkfifo: ");
        exit(EXIT_FAILURE);
    }

    coord_pid = fork();


    if(coord_pid == 0)
    {
        execlp("./jms_coord", "./jms_coord", "-l", "coordDir", "-n", max_jobs_str, "-w", jms_out, "-r", jms_in, NULL);
        perror("exec: ");
        exit(EXIT_FAILURE);
    }


    out_pipe = open(jms_out, O_RDONLY );
    in_pipe = open(jms_in, O_WRONLY );


//    printf("Console: in pipe = %d\nout pipe = %d\n", in_pipe, out_pipe);
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


    char line[BUFFERSIZE];								//na eimai sigouros oti den tha dwsoun kamia vlakeia megali
    for (int k = 0; k < BUFFERSIZE; ++k) {
        line[k]= '\0';
    }

    int finish = 0;
    char **word = malloc(20*sizeof(char *));	//To polu 10 le3eis i insert me megethos 20xaraktires to kathena

        while(finish == 0 && fgets(line, sizeof(line), stdin))
        {
            int wordCount = cutLine(line,word);
            if(wordCount > 0)
            {

/*
                for (int j = 0; j < wordCount; ++j) {
                    printf("word[%d] = %s (%zu)\n", j, word[j], strlen(word[j]));
                }
*/


                if(checkKeyWords(word[0]) == 0)
                {
                    if (write(in_pipe, line, BUFFERSIZE) != BUFFERSIZE) {
                        perror("console: data write error");
                        exit(EXIT_FAILURE);
                    }
                    long long int n;

                    while((n = read(out_pipe,line,BUFFERSIZE)) == 0)
                    {}
                    printf("%s\n",  line);
                    if(strcmp(word[0],"shutdown") == 0)
                        finish =1;

                }
                for (int j = 0; j < wordCount; ++j) {
                    free(word[j]);
                }

            }

            for (int k = 0; k < BUFFERSIZE; ++k) {
                line[k]= '\0';
            }


    /*
            printf("in\n");
            printf("out\n");
    */
        }

    int status;
    wait(&status);

    close(in_pipe);
    close(out_pipe);

    remove(jms_in);
    remove(jms_out);

    free(word);
    free(jms_in);
    free(jms_out);
    free(op_file);

    printf("Exiting console.\n");
    return 0;
}


int checkKeyWords(char *str)
{
    if( strcmp(str,"submit") == 0 ||  strcmp(str,"status") == 0 ||  strcmp(str,"status-all") == 0 ||  strcmp(str,"status-all\n") == 0 ||  strcmp(str,"show-active") == 0 ||  strcmp(str,"show-pools") == 0 || strcmp(str,"show-finished") == 0 ||  strcmp(str,"suspend") == 0 ||  strcmp(str,"resume") == 0 || strcmp(str,"shutdown") == 0 )
        return 0;
    else
    {
        fprintf(stderr, "Error. No such command. Please try again.\n");
        return 1;
    }
}

