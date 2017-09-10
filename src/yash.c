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


typedef int bool;
#define true 1
#define false 0

typedef enum Status {
    STOPPED,
    RUNNING,
    DONE
} pstatus;

char STOPPED_STR[] = "Stopped",
     RUNNING_STR[] = "Running",
     DONE_STR[]    = "Done";
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
    pid_t group;
    pstatus status; 
    int processCount;
    struct job* next;
} job;

int job_num = 0;
job* new_job(job* current, char* name, pid_t group, int processCount){
    job* new = (job*) malloc(sizeof(job));     
    new->name = strdup(name);
    new->group = group;
    new->status = STOPPED;
    new->processCount = processCount;
    new->next = NULL;

    if (current == NULL){
        job_num = 0;
    }
    job_num++;

    if (current != NULL){
        job* temp = current;
        while (temp->next != NULL){
            temp = temp->next;
        }
        temp->next = new;
    }
    else {
        current = new;
    }
    

    new->number = job_num;
    return current;
}

job* get_foreground(job* current){
    if (current == NULL)
        return NULL;
    
    while (current->next != NULL){
        current = current->next;
    }
    return current;
}


typedef struct maybeJob {
    job* value;
    bool exists;
} maybeJob;

maybeJob get_background(job* current){
    if (current == NULL){
        maybeJob maybe;
        maybe.value = NULL;
        maybe.exists = false;
        return maybe;
    }
    bool exists = false;
    job* ret;
    while (current != NULL){
        if (current->status == STOPPED){
            exists = true;
            ret = current;
        }
        else {
            if (exists == false){
                ret = current;
            }
        }
        current = current->next;
    }
    maybeJob maybe;
    maybe.value = ret;
    maybe.exists = exists;
    return maybe;
}

job* remove_job(job* current, pid_t id){
    job* ret = current->next;
    free(current);
    return ret;
}

