//
// Created by valyo95 on 29/4/2017.
//

#ifndef UNTITLED_FUNCTIONS_H
#define UNTITLED_FUNCTIONS_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

char *my_itoa(int num, char **str)
{
    if(*str == NULL)
    {
        return NULL;
    }
    sprintf(*str, "%d", num);
    return *str;
}

int cutLine(char *line, char **word)
{
    int i = 0;
    char *tmp = strdup(line);
    char *tmp1 = tmp;
    tmp = strsep(&tmp,"\n");
    char delimit[]=" ";

    char *token;
    int k = strlen(tmp);

    while( (token = strsep(&tmp,delimit)) != NULL )
    {
        word[i] = strdup(token);
        k-=strlen(word[i])+1;
        while(k > 0 && isspace(tmp[0]))
        {
            tmp+=sizeof(char);
            k--;
        }
        i++;
    }
    word[i] = NULL;
    free(tmp1);
    return i;
}


#endif //UNTITLED_FUNCTIONS_H
