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
#include <sys/stat.h>
#include <fcntl.h>
//https://stackoverflow.com/questions/4204915/please-explain-exec-function-and-its-family
//https://unix.stackexchange.com/questions/149741/why-is-sigint-not-propagated-to-child-process-when-sent-to-its-parent-process
//https://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file
//https://unix.stackexchange.com/questions/41421/what-is-the-file-descriptor-3-assigned-by-default
//https://stackoverflow.com/questions/33884291/pipes-dup2-and-exec
//https://stackoverflow.com/questions/17630247/coding-multiple-pipe-in-c


void sigint_handler(int signum){
    printf("\n");
}

void sigtstp_handler(int signum){
    printf("\n");
}

typedef enum Status {
    STOPPED,
    RUNNING,
    DONE
} pstatus;

char STOPPED_STR[] = "STOPPED",
     RUNNING_STR[] = "RUNNING",
     DONE_STR[]    = "DONE";
char* status_string(pstatus status){
    switch(status){
        case STOPPED:
            return STOPPED_STR;
        case RUNNING:
            return RUNNING_STR;
        case DONE:
            return DONE_STR;
        default:
                return NULL;
    }
}


//FIFO for process groups. Single processes are treated as a group
typedef struct job {
    char* name;
    int number;
    pid_t session;
    pstatus status; 
    struct job* next;
} job;

int job_num = 0;
job* new_job(job* current, char* name, pid_t session){
    job* ret = (job*) malloc(sizeof(job));     
    ret->name = strdup(name);
    ret->session = session;
    ret->status = RUNNING;
    ret->next = current;

    if (current == NULL){
        job_num = 0;
    }
    job_num++;

    ret->number = job_num;
    return ret;
}

void print_jobs(job* jobs){
    char* name;
    int number;
    char fg;
    char* status;

    
    while(jobs != NULL){
        name = jobs->name;
        number = jobs->number;
        fg = '+';
        status = status_string(jobs->status); 
        printf("[%i] %c %s %s", number, fg, status, name);

        jobs = jobs->next;
    }

}


void tokenizer(char* input, char** array, size_t* size, char** outfile, char** infile, char** errorfile, int* pipeIndex){
    char* current;
    char* tk = strtok(input, " ");
    

    while (tk != NULL){
        if (strcmp(tk, ">") == 0){
            tk = strtok(NULL, " ");
            *outfile = (char*) malloc(strlen(tk)*sizeof(char*));
            strcpy(*outfile, tk);
        }
        else if (strcmp(tk, "2>") == 0){
            tk = strtok(NULL, " ");
            *errorfile = (char*) malloc(strlen(tk)*sizeof(char*));
            strcpy(*errorfile, tk);
        }
        else if (strcmp(tk, "<") == 0){
            tk = strtok(NULL, " ");
            *infile = (char*) malloc(strlen(tk)*sizeof(char*));
            strcpy(*infile, tk);
        }
        else if (strcmp(tk, "|") == 0){
            array[*size] = NULL;
            *pipeIndex = *size + 1;
            (*size)++;
        }
        else{
            current = (char*) malloc(strlen(tk)*sizeof(char*));
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



char instr[2000];

char* tokens[2000];
size_t tokenSize = 0;

job* jobs = NULL;

pid_t cpid;
pid_t cpid2;
int main(int argc, char* argv[]){
    int pipeIndex;

    signal(SIGINT, sigint_handler); 
    signal(SIGTSTP, sigtstp_handler);

    char* outfile;
    char* infile;
    char* errorfile;

    int fd[2];
 
    while (1){
        pipeIndex = -1;
        outfile = NULL;
        infile = NULL;
        errorfile = NULL;
        while (tokenSize > 0){
            free(tokens[tokenSize]);
            tokenSize--;
        }
        
        printf("%s", "# ");
        fflush(stdout);
        if (fgets(instr, 2000, stdin) == NULL){
            exit(0);
        }
        //if (feof(stdin)){
        //    exit(0);
       // }
        if ((strlen(instr) > 0) && (instr[strlen(instr) - 1] == '\n')){
            instr[strlen(instr) - 1] = '\0';
        }
        
        if (strcmp(instr, "fg") == 0){
            kill(cpid, SIGCONT);
            //kill(-cpid, SIGCONT);
            int status;
            waitpid(cpid, &status, 0);
        }
        else if (strcmp(instr, "jobs") == 0){
            print_jobs(jobs);
        }
        else {

        if (strlen(instr) > 0){
            tokenizer(instr, tokens, &tokenSize, &outfile, &infile, &errorfile, &pipeIndex);
            if (pipeIndex > 0){
                pipe(fd);
            }
            cpid = fork();
            if (cpid == 0){ 
                
                if (outfile != NULL){
                    int outfilenum = open(outfile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
                    dup2(outfilenum, 1);
                    close(outfilenum);
                }
                if (errorfile != NULL){
                    int errorfilenum = open(errorfile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
                    dup2(errorfilenum, 2);
                    close(errorfilenum);
                }
                if (infile != NULL){
                    int infilenum = open(infile, O_RDONLY);
                    if (infilenum < 0){
                        printf("yash: %s: No such file or directory\n", infile);
                        fflush(stdout);
                        _exit(0);
                    }
                    dup2(infilenum, 0);
                    close(infilenum);
                }
                if (pipeIndex > 0){
                    close(fd[0]);
                    dup2(fd[1], 1);
                    close(fd[1]);
                }
                if (execvp(tokens[0], tokens) == -1){
                    printf("yash: %s: command not found\n", tokens[0]);
                    _exit(0);
                }
            }
            else {
                if (pipeIndex > 0){
                    cpid2 = fork();
                    if (cpid2 == 0){
                        if (outfile != NULL){
                            int outfilenum = open(outfile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
                            dup2(outfilenum, 1);
                            close(outfilenum);
                        }
                        if (errorfile != NULL){
                            int errorfilenum = open(errorfile, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
                            dup2(errorfilenum, 2);
                            close(errorfilenum);
                        }
                        if (infile != NULL){
                            int infilenum = open(infile, O_RDONLY);
                            if (infilenum < 0){
                                _exit(0);
                            }
                            dup2(infilenum, 0);
                            close(infilenum);
                        }
                        close(fd[1]);
                        dup2(fd[0], 0); 
                        close(fd[0]);
                        if (execvp(tokens[pipeIndex], &tokens[pipeIndex]) == -1){
                            printf("yash: %s: command not found\n", tokens[pipeIndex]);
                            _exit(0);
                        }
                    }
                    close(fd[0]);
                    close(fd[1]);
                    int status;
                    waitpid(cpid2, &status, 0);
                }
                else{
                    int status;
                    waitpid(-1, &status, WUNTRACED | WCONTINUED);
                }
            }
        } 
        }
    }
    return 0;
}
