//Quinten Zambeck
//qaz62
//OS lab 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <malloc.h>
#include <signal.h>
#include <fcntl.h>
//https://stackoverflow.com/questions/4204915/please-explain-exec-function-and-its-family
//https://unix.stackexchange.com/questions/149741/why-is-sigint-not-propagated-to-child-process-when-sent-to-its-parent-process
//https://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file
//https://unix.stackexchange.com/questions/41421/what-is-the-file-descriptor-3-assigned-by-default



void sigint_handler(int signum){
    printf("\n");
}

void sigtstp_handler(int signum){
    printf("\n");
}





void tokenizer(char* input, char** array, size_t* size){


    char* current;
    char* tk = strtok(input, " ");
    int outfile;
    

    while (tk != NULL){
        if (strcmp(tk, ">") == 0){
            tk = strtok(NULL, " ");
            outfile = open(tk, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
            dup2(outfile, 1);
            close(outfile);
        }
        else if (strcmp(tk, "2>") == 0){
            tk = strtok(NULL, " ");
            outfile = open(tk, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
            dup2(outfile, 2);
            close(outfile);
        }
        else if (strcmp(tk, "<") == 0){
            tk = strtok(NULL, " ");
            outfile = open(tk, O_RDONLY);
            dup2(outfile, 0);
            close(outfile);
        }

        else{
            current = malloc(strlen(tk)*sizeof(char*));
            strcpy(current, tk);
            array[*size] = current;
            (*size)++;
        }

        tk = strtok(NULL, " ");
    }
    if (*size < 2000){
        array[(*size)] = NULL;
    }
}


char path[] = "/usr/bin/";

char instr[2000];
char execute[2000];
char args[2000];

char* tokens[2000];
size_t tokenSize = 0;


pid_t cpid;
int main(int argc, char * argv[]){
    signal(SIGINT, sigint_handler); 
    signal(SIGTSTP, sigtstp_handler);

    while (!feof(stdin)){
        while (tokenSize > 0){
            free(tokens[tokenSize]);
            tokenSize--;
        }
        
        printf("%s", "# ");
        fgets(instr, 2000, stdin);
        if (feof(stdin)){
            exit(0);
        }
        if ((strlen(instr) > 0) && (instr[strlen(instr) - 1] == '\n')){
            instr[strlen(instr) - 1] = '\0';
        }
        cpid = fork();
        if (cpid == 0){ 
            tokenizer(instr, tokens, &tokenSize);
            strcpy(execute, path);
            
            strcat(execute, tokens[0]);
            if (execv(execute, tokens) == -1){
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
