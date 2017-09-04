//Quinten Zambeck
//qaz62
//OS lab 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <malloc.h>
//https://stackoverflow.com/questions/4204915/please-explain-exec-function-and-its-family
//https://unix.stackexchange.com/questions/149741/why-is-sigint-not-propagated-to-child-process-when-sent-to-its-parent-process


void tokenizer(char* input, char** array, size_t* size){

    char* current;
    char* tk = strtok(input, " ");
    
    while (tk != NULL){
        current = malloc(strlen(tk)*sizeof(char*));
        strcpy(current, tk);
        array[*size] = current;
        (*size)++;
        tk = strtok(NULL, " ");
    }
}


//char path[100]; 
char path[] = "/usr/bin/";

char instr[2000];
char execute[2000];
char args[2000];

char* tokens[2000];
size_t tokenSize = 0;

pid_t cpid;
int main(int argc, char * argv[]){
    while (1){
        printf("%s", "# ");
        //scanf("%s", instr);
        fgets(instr, 2000, stdin);
        if ((strlen(instr) > 0) && (instr[strlen(instr) - 1] == '\n')){
            instr[strlen(instr) - 1] = '\0';
        }
        //token = strtok(instr, " ");
        tokenizer(instr, tokens, &tokenSize);
        strcpy(execute, path);
        strcat(execute, tokens[0]);
        /*
        strcpy(execute, path);
        strcat(execute, token);
        token = strtok(NULL, " ");
        strcpy(args, "");
        while(token != NULL){
            printf("%s\n", token);
            strcat(args, token);
            token = strtok(NULL, " ");
        }
        */

        cpid = fork();
        if (cpid == 0){ 
            //printf("Hi");
            //if (execl(execute, "") == -1){
            if (execl(execute, "") == -1){
                printf("Error\n");
            }
            exit(0);
        }
        else {
            int status;
            waitpid(cpid, &status, 0);
        }
    }

    return 0;
}
