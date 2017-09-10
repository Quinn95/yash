//Quinten Zambeck
//qaz62
//OS lab 1

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
//https://stackoverflow.com/questions/4204915/please-explain-exec-function-and-its-family
//https://unix.stackexchange.com/questions/149741/why-is-sigint-not-propagated-to-child-process-when-sent-to-its-parent-process
//https://stackoverflow.com/questions/2605130/redirecting-exec-output-to-a-buffer-or-file
//https://unix.stackexchange.com/questions/41421/what-is-the-file-descriptor-3-assigned-by-default
//https://stackoverflow.com/questions/33884291/pipes-dup2-and-exec
//https://stackoverflow.com/questions/17630247/coding-multiple-pipe-in-c


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

job* remove_job(job* current){
    job* ret = current->next;
    free(current);
    return ret;
}



char instr[2000];
char instr_cpy[2000];
char* tokens[2000];
size_t tokenSize = 0;
job* jobs = NULL;
pid_t cpid;
pid_t cpid2;

static void sigint_handler(int signum){
    kill(-cpid,SIGINT);
}

static void sigtstp_handler(int signum){
    //printf("Hi\n");
    if (instr[0] != '\0'){
        //printf("%i suspended\n", cpid);
        jobs = new_job(jobs, instr, cpid);
        kill(-cpid,SIGTSTP);    
        tcsetpgrp(STDIN_FILENO, getpgid(0));
    }
}

static void sigchld_handler(int signum){
    printf("sigchld\n");
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
        printf("[%i] %c %s %s\n", number, fg, status, name);

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

char* outfile;
char* infile;
char* errorfile;

void fileHelper(){
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

}


int main(int argc, char* argv[]){
    int pipeIndex = 0;

    signal(SIGINT, sigint_handler); 
    //signal(SIGTSTP, sigtstp_handler);
    signal(SIGTTOU, SIG_IGN);
//    signal(SIGCHLD, sigchld_handler);

    int fd[2];
 
    int pid = 0;
    while (1){
        pipeIndex = -1;
        outfile = NULL;
        infile = NULL;
        errorfile = NULL;
        instr[0] = '\0';
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
        
        strcpy(instr_cpy, instr);
        if (strcmp(instr, "fg") == 0){
            printf("%s\n", jobs->name);
            tcsetpgrp(STDIN_FILENO, cpid);
            kill(-cpid, SIGCONT);
            //kill(cpid2, SIGCONT);
            int status;
            waitpid(-cpid, &status, WUNTRACED);
            waitpid(-cpid, &status, WUNTRACED);
            tcsetpgrp(STDIN_FILENO, getpgid(0));
            if (WIFSTOPPED(status)){
                printf("ctrl-z\n");
            }
             //waitpid(cpid, &status, 0);
        }
        else if (strcmp(instr, "jobs") == 0){
            print_jobs(jobs);
        }
        else { //do stuff
            if (strlen(instr) > 0){
                tokenizer(instr, tokens, &tokenSize, &outfile, &infile, &errorfile, &pipeIndex);
                if (pipeIndex > 0){
                    pipe(fd);
                }
                cpid = fork();
                if (cpid == 0){ 
                    //child 1
                    //setsid(); // new session with group id == cpid
                    setpgid(0, 0);
                    fileHelper();
                    if (pipeIndex > 0){
                        close(fd[0]);
                        dup2(fd[1], 1);
                        close(fd[1]);
                    }
                    if (execvp(tokens[0], tokens) == -1){
                        printf("yash: %s: command not found\n", tokens[0]);
                        fflush(stdout);
                        _exit(0);
                    }
                }
                else {
                    if (pipeIndex > 0){
                        cpid2 = fork();
                        if (cpid2 == 0){
                            // CHILD 2
                            //printf("cpg: %i -- cpid: %i -- getpgid:%i\n", getsid(cpid), cpid, getpgid(0));
                            int errori;
                            errori = setpgid(0, cpid);
                            if (errori == -1){
                                int errsv = errno;
                                if (errsv == EINVAL){
                                    printf("einval");
                                }
                                if (errsv == EACCES){
                                    printf("eacces\n");
                                }
                                if (errsv == ENOSYS){
                                    printf("enosys\n");
                                }
                                if (errsv == EPERM){
                                    printf("eperm\n");
                                }
                            }
                            //printf("cpg: %i -- cpid: %i -- getpgid:%i\n", getsid(cpid), cpid, getpgid(0));
                            /*
                            if (errori != 0){
                                if (EACCESS(errori)){
                                    print("eaccess\n");
                                }
                                else if (EINVAL(errori)){
                                    printf("einval\n");
                                }
                                else if (ENOSYS(errori)){
                                    printf("enosys\n");
                                }
                                else if (EPERM(errori)){
                                    printf("eperm\n");
                                }
                                else if (ESRCH(errori)){
                                    printf("esrch\n");
                                }
                                printf("setpgid error %i\n", errori);
                            }
                            */ 
                            fileHelper();

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
//                        printf("%i - %i - %i\n", getpid(), cpid, cpid2);
//                        fflush(stdout);
                        setpgid(cpid, cpid);
                        setpgid(cpid2, cpid);
                        jobs = new_job(jobs, instr_cpy, cpid);
                        tcsetpgrp(STDIN_FILENO, cpid);
                        pid = waitpid(- jobs->session, &status, WUNTRACED | WCONTINUED);
                        pid = waitpid(- jobs->session, &status, WUNTRACED | WCONTINUED);
                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                        if (WIFSTOPPED(status)){
                            printf("\nKilled: %s\n", jobs->name);
                        }
                    }
                    else{
                        // PARENT
                        setpgid(cpid, cpid);
                        tcsetpgrp(STDIN_FILENO, cpid);
                        int status = 0;
                        //pid = waitpid(cpid, &status, WUNTRACED | WCONTINUED);
//                        printf("%i - %i - %i\n", getpid(), cpid, getpgid(cpid));
                        pid = waitpid(-cpid, &status, WUNTRACED | WCONTINUED);

                        tcsetpgrp(STDIN_FILENO, getpgid(0));
                        if (WIFSTOPPED(status)){
                            printf("ctrl-z\n");
                        }
                    }
                }
            } 
        }
    }
    return 0;
}
