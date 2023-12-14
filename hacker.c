#include <stdio.h>      // Standard input/output functions
#include <stdlib.h>     // Standard library for memory allocation, process control, etc.
#include <string.h>     // String handling functions
#include <sys/types.h>  // Definitions for types like pid_t
#include <sys/ipc.h>    // IPC (Inter-process communication) key structures
#include <sys/shm.h>    // Shared memory functions
#include <sys/sem.h>    // Semaphore functions
#include <unistd.h>     // Provides access to the POSIX operating system API
#include <sys/msg.h>    // Message queue functions
#include <time.h>       // Time functions for timestamps
#include <errno.h>  
#define PRINT_BUFF_KEY 271546
#define MAXSIZE 500
struct SendHack{
    long mtype; // Message type for communication protocol
    int no_of_line; // Number of lines to print
    char mtext[MAXSIZE]; // Text of the message
};

int main()
{
    // printf("Inside print_msg.c file");
    int msgqid;
    struct SendHack sendBuf;
    // key_t key=ftok(".",PRINT_BUFF_KEY);
    key_t key=1234;
    // sendBuf.mtype=1;
    // sprintf(sendBuf.mtext, "####### HACKER NODE HIIIIIIIHAAAAAAHAHAHAHA ######\n");
    // if((msgqid=msgget(key,IPC_CREAT|0666))==-1)
    // {
    //     if(errno == EAGAIN)
    //     {
    //         fprintf(stderr, "Message queue is full, Terminated..\n");
    //         exit(EXIT_FAILURE);
    //     }
    //     else
    //     {
    //         perror("In Parent Message Sending Failed\n");
    //         exit(1);
    //     }
    // }
    // int buflen = sizeof(sendBuf.mtext) +1;
    // if(msgsnd(msgqid,&sendBuf,buflen,IPC_NOWAIT)<0)
    // {
    //     perror("\nERROR IN SENDING MESSAGE TO SERVER\n");
    // }

    while(1)
    {
        sendBuf.mtype=1;
        sprintf(sendBuf.mtext, "#######HACKER NODE HIIIIIIIHAAAAAAHAHAHAHA######\n");
        if((msgqid=msgget(key,IPC_CREAT|0666))==-1)
        {
            if(errno == EAGAIN)
            {
                fprintf(stderr, "Message queue is full, message sending is delayed.\n");
                return -1;
            }
            else
            {
                perror("In Parent Message Sending Failed\n");
                exit(1);
            }
        }
        int buflen = sizeof(sendBuf.mtext) +1;
        if(msgsnd(msgqid,&sendBuf,buflen,IPC_NOWAIT)<0)
        {
            perror("\nERROR IN SENDING MESSAGE TO SERVER\n");
        }
        sleep(6);
    }
}
