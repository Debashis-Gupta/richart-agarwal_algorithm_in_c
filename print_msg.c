#include <stdio.h>      // Standard input/output functions
#include <stdlib.h>     // Standard library for memory allocation, process 
control, etc.
#include <string.h>     // String handling functions
#include <sys/types.h>  // Definitions for types like pid_t
#include <sys/ipc.h>    // IPC (Inter-process communication) key 
structures
#include <sys/shm.h>    // Shared memory functions
#include <sys/sem.h>    // Semaphore functions
#include <unistd.h>     // Provides access to the POSIX operating system 
API
#include <sys/msg.h>    // Message queue functions
#include <time.h>       // Time functions for timestamps
#include <errno.h>  
#define PRINT_BUFF_KEY 271546
#define MAXSIZE 200
struct PrintData{
    long mtype; // Message type for communication protocol
    int no_of_line; // Number of lines to print
    char mtext[MAXSIZE]; // Text of the message
};

int main()
{
    // printf("Inside print_msg.c file");
    int msgqid;
    struct PrintData rcvbuffer;
    // key_t key=ftok(".",PRINT_BUFF_KEY);
    key_t key=1234;
    while(1)
    {

        if((msgqid=msgget(key,0666))<0)
        {
            // perror("\n*****Msgget Failed*****\n");
        }
        if(msgrcv(msgqid,&rcvbuffer,sizeof(rcvbuffer),1,0)<0)
        {
            // perror("\n*****MSGRECIEVE Failed*****\n");
        }
        else
        {
            printf("%s\n",rcvbuffer.mtext);
        }
        
    }
}



