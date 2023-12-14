#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>
#include <sys/msg.h>
#include <errno.h>
#include <time.h>

#define PRINT_SERVER_KEY 271546

#define SYNC_MSG_KEY 3141678 //2905
#define PRINT_BUFF_KEY 4821

#define NO_OF_NODE 2

// Define the shared structure
typedef struct {
    
    int request_number;
    int highest_request_number;
    int outstanding_reply;
    int reply_deferred[100]; // This will be a flexible array member
    int CS_Request; 
    
} SharedData;
typedef struct{
    long mtype; 
    int from;
    int req_value;
    int to;
}Sync_msg;

typedef struct{
    long mtype; 
    char mtext[200]; 
}ServerData;

// ######### GLOBAL VARIABLES STARTS ########

int N=NO_OF_NODE;
int k;
int me;
int mutex_id;
int wait_sem_id;
int sync_msg_queue_id;
int shm_id;
int MUTEX_KEY;
int WAIT_SEM_KEY;
// ######### GLOBAL VARIABLES ENDS ########

// Creating P & V

void P(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = -1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}
void V(int sem_id) {
    struct sembuf sem_op;
    sem_op.sem_num = 0;
    sem_op.sem_op = 1;
    sem_op.sem_flg = 0;
    semop(sem_id, &sem_op, 1);
}

// Ending P & V

SharedData *shared_data;



int main(int argc, char *argv[]) {
    srand(time(0)); // random number intialization

    
    me = atoi(argv[1]);
    printf("Me is %d\n", me);


    // Setting up the environment
    char demo[200];
    char dir_path[100];
    int msg_queue_id;
    if (getcwd(dir_path, sizeof(dir_path)) == NULL) {
        perror("getcwd");
        exit(1);
    }
   
    // int MUTEX_KEY = 121+me;
    // int WAIT_SEM_KEY=345+me;
    MUTEX_KEY=275+me;
    WAIT_SEM_KEY=594+me;
    // printf("MUTEX KEY %d\n", MUTEX_KEY);
    // printf("WAITSEM KEY %d\n", WAIT_SEM_KEY);
    key_t sync_msg_key = ftok(dir_path,SYNC_MSG_KEY);
    sync_msg_queue_id = msgget(sync_msg_key,0666);
    // ######### CREATING MESSAGE SYNC KEY #########
    if(sync_msg_queue_id==-1)
    {
        if(errno== ENOENT)
        {
            sync_msg_queue_id = msgget(sync_msg_key,0666 | IPC_CREAT);
            if(sync_msg_queue_id==-1)
            {
                perror("msgget");
                exit(1);
            }
        }
    }
    MUTEX_KEY = 121+me;
    // printf("MUTEX KEY %d\n", MUTEX_KEY);
    //  ######## CREATING MUTEX KEY #######
    // key_t m_key = ftok(demo,MUTEX_KEY);
    key_t m_key = MUTEX_KEY;
    // printf("m_key = %d\n",m_key);
    mutex_id = semget(m_key,1,0666 | IPC_CREAT | IPC_EXCL);
    if(mutex_id == -1)
    {
        if(errno == EEXIST)
        {
            mutex_id = semget(m_key,1,0666);
        }
        else
        {
            perror("semget");
            exit(1);
        }
    }
    semctl(mutex_id,0,SETVAL,1);

      //  ######## CREATING SEMAPHORE KEY #######
    WAIT_SEM_KEY=345+me;
    // int WAIT_SEM_KEY=345+me;
    // key_t w_sem_key = ftok(demo,WAIT_SEM_KEY);
    key_t w_sem_key = WAIT_SEM_KEY;
    // printf("w_sem_key = %d\n",w_sem_key);
    wait_sem_id = semget(w_sem_key,1,0666 | IPC_CREAT | IPC_EXCL);

    if(wait_sem_id == -1)
    {
        if(errno==EEXIST)
        {
            wait_sem_id = semget(w_sem_key,1,0666);
        }
        else
        {
            perror("semget");
            exit(1);
        }
    }
    semctl(wait_sem_id,0,SETVAL,1);


   //  ######## ENDING SEMAPHORE KEY #######
    // printf("\nSETTING SUCCESSFUL\n");
    //  Setting Done

    key_t shm_key = ftok("./",231+me);
    
    shm_id = shmget(shm_key, sizeof(SharedData), 0666|IPC_CREAT);
    if (shm_id == -1) {
        perror("shmget failed");
        exit(1);
    }
       printf("Shared memory segment created with ID %d\n", shm_id);


    shared_data = (SharedData*) shmat(shm_id, (void*)0, 0);
    if (shared_data == (SharedData*) -1) {
        perror("shmat failed");
        exit(1);
    }
    int size = sizeof(SharedData);
    
    printf("Size of shared memory segment %d\n", size);
    // ######### ASSIGNING VALUES TO SHARED MEMORY
    // shared_data->outstanding_reply ;
    memset(shared_data->reply_deferred, 0, sizeof(shared_data->reply_deferred));
    shared_data->CS_Request = 0;
    shared_data->highest_request_number=0;
    shared_data->request_number=0;

    // int size1=100;
    // int *array = malloc(size1 * sizeof(int));
    // if (array == NULL) {
    //     fprintf(stderr, "Memory allocation failed!\n");
    //     return 1;
    // }
    // else
    // {
    //     printf("@@@@@ Array is successfully created @@@@@ \n");
    // }
    // array[1] = 1;
    // array[2] = 2;
    // array[3] = 3;
    // array[4] = 4;
    // int N = 4;
    // ######### ENDING ASSIGNING VALUES TO SHARED MEMORY

    // ########## CREATING CHILD #######
    pid_t child = fork();

    if(child == -1)
    {
        perror("ERROR IN CREATING CHILD\n");
    }
    // child block starts --- RECEIVE REQUEST AND REPLY SECTION #########
    if(child == 0)   
    {
         // ************* Sending A Request ****************
        while(1)
        {   

            printf("############  SENDING REQUEST  ############\n");
            P(mutex_id);
            shared_data->CS_Request=1;
            shared_data->request_number = shared_data->highest_request_number+1;
            // V(mutex_id);
            // printf("\nParent -> Shared request number in node %d is %d\n",me,shared_data->request_number);
            shared_data->outstanding_reply = N-1;
            // printf("\nParent-> Shared outstanding_reply in node %d is %d\n",me,shared_data->outstanding_reply);
            V(mutex_id);
            for(int i=1;i<=N;i++)
            {
                if(i!=me)
                {
                    Sync_msg send_msg;
                    send_msg.mtype = i;
                    send_msg.from = me;
                    send_msg.to = i;
                    send_msg.req_value=shared_data->request_number;

                    if(msgsnd(sync_msg_queue_id,&send_msg,sizeof(send_msg),IPC_NOWAIT)==-1)
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
                    else
                    {
                        printf("Message send to node %d successfully\n",send_msg.to);
                    }

                }
            }
            // sleep(2);
            while(shared_data->outstanding_reply!=0)
            {
                P(wait_sem_id);
            }
            // printf("Node %d gets hold on P() or not?\n",me);
            // ##### STARTING CRITICAL SECTION #######
            printf("NODE %d ENTERTING INTO THE CRITICAL SECTION\n",me);
            ServerData send_msg;

            char dir_path[100];
            int msg_queue_id;
            if (getcwd(dir_path, sizeof(dir_path)) == NULL) {
                perror("getcwd");
                exit(1);
            }
            key_t print_key = ftok(dir_path, PRINT_SERVER_KEY);
            // key_t print_key = PRINT_SERVER_KEY;
            int msgqid;
            //###############################
            send_msg.mtype=9999;
            //###############################
            // snprintf(send_msg.mtext, sizeof(send_msg.mtext) ,"####### START OUTPUT FOR NODE : %d ######\n", me);
            sprintf(send_msg.mtext, "####### START OUTPUT FOR NODE : %d ######\n", me);
            if((msgqid=msgget(print_key,IPC_CREAT|0666))<0)
            {
                perror("\nERROR IN SENDING MESSAGE QID \n");
            }
            int buflen = sizeof(send_msg.mtext) +1;
            if(msgsnd(msgqid,&send_msg,buflen,IPC_NOWAIT)==-1)
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
            else
            {
                printf("\nMessage Send Successfully\n");
            }
            // printf("###################### Starting by NODE %d #####################\n",me);
            send_msg.mtype=9999;
            for(int i=1;i<=5;i++)
            {
                sleep(1);
                sprintf(send_msg.mtext, "####### ---- Node %d saying hello %d time(s)--- ######\n", me,i);
                // printf("Node %d saying hello %d time(s)\n",node_id,i);
                if(msgsnd(msgqid,&send_msg,buflen,IPC_NOWAIT)==-1)
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
            }
            send_msg.mtype=9999;
            // snprintf(send_msg.mtext,sizeof(send_msg.mtext), "-----END OUTPUT FOR NODE:%d ----\n", me);
            sprintf(send_msg.mtext, "####### END OUTPUT FOR NODE : %d ######\n", me);
            if((msgqid=msgget(print_key,IPC_CREAT|0666))<0)
            {
                perror("\nERROR IN SENDING MESSAGE QID \n");
            }
            buflen = sizeof(send_msg.mtext) +1;
            if(msgsnd(msgqid,&send_msg,buflen,IPC_NOWAIT)==-1)
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

            
            // ##### ENDING CRITICAL SECTION #######
            printf("NODE %d EXITING THE CRITICAL SECTION\n",me);
            sleep(rand()%3+me);
            P(mutex_id);
            shared_data->CS_Request=0;
            V(mutex_id);
            // printf("######## CHECKING REPLY DEFERRED BUFFER ARRAY (IN-PARENT) ######\n");
            // for(int i=1;i<=N;i++){
            //     if(i!=me)
            //     {
            //         printf("Node : %d Value : %d\n",i,shared_data->reply_deferred[i]);
            //     }
            // }
            for(int i=1;i<=N;i++)
            {
                if(i!=me)
                {
                    // printf("Reply_deferred Node is : %d and value is %d\n",i,shared_data->reply_deferred[i]);
                    if(shared_data->reply_deferred[i])
                    {
                        shared_data->reply_deferred[i]=0;
                        Sync_msg response_msg;
                        response_msg.from = me;
                        response_msg.to =i;
                        response_msg.mtype = i;
                        response_msg.req_value=-1;

                        // printf("################# PARENT SENDING ############\n");
                        // printf("############## RESPONSE SENDING ##############\n");
                        // printf("Response Message From %d \n",response_msg.from);
                        // printf("Response Message To %d\n",response_msg.to);
                        // printf("Response Message Mtype %d\n ",response_msg.mtype);
                        // printf("Response Message Req_Value %d\n ",response_msg.req_value);
                        // printf("############## RESPONSE SENDING ENDING ##############\n");



                        if(msgsnd(sync_msg_queue_id,&response_msg,sizeof(response_msg),IPC_NOWAIT)==-1)
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
                        else
                        {
                            printf("RESPONSE SEND TO NODE %d SUCCESSFULLY\n",response_msg.to);
                        }
                    }
                }
            }
            sleep(rand()%50+1);
        }

    }
    else
    {

        while(1)
        {

            // printf("\nHi From CHILD...\n");
            Sync_msg msg_req;
            // P(mutex_id);
            // printf("$$$$$$ Checking if there is NEW NODE $$$$$$$\n");
            if(msgrcv(sync_msg_queue_id,&msg_req,sizeof(msg_req),me,0)==-1)
            {
                perror("MESSAGE RECEIVE FAILED\n");
            }
            else
            {
                printf("\nMessage Received successfully from NODE=%d\n",msg_req.from);
                // printf("Look It Works...\n");
            }
            // P(mutex_id);
            // printf("#### CHECKING IF THERE IS NEW NODE #### \n");
            // if (isValuePresent(array, size1, msg_req.from)) 
            // {
            //      N=N+1;
            //      array[N]=N;
            //     //  shared_data->outstanding_reply= shared_data->outstanding_reply+1;
            //      printf("####### NEW NODE ARRIVED ########\n");
            //      printf("#### NOW outstanding_reply = %d ###\n", shared_data->outstanding_reply);
            // } 
            // else
            // {
            //     printf("#### NO New NODE ARRIVED - Currently #N = %d #####\n",N);
            // }
            // printf("####### STARTING PRINTING THE ARRAY #########\n");
            // for (int j =1;j<=N;j++)
            // {
            //     printf("array[%d] = %d\n",j,array[j]);
            // }
            // printf("####### ENDING PRINTING THE ARRAY #########\n");
            // V(mutex_id);
            int defer_it;
            
            
            
            k = msg_req.req_value;   // This will tell me if a node is replying my response or requesting me for accessing CS....
            
            if(k==-1)
            {
                // while(1)
                // {
                    // Sync_msg msg_req;
                    // if(msgrcv(sync_msg_queue_id,&msg_req,sizeof(msg_req),me,0)==-1)
                    // {
                    //     perror("MESSAGE RECEIVE FAILED\n");
                    // }
                    printf("************** ACKNOWLEDGMENT RECEIVED from NODE %d *************\n",msg_req.from);
                    P(mutex_id);
                    
                    shared_data->outstanding_reply= shared_data->outstanding_reply-1;
                    printf("********* For NODE %d OUTSTANDING REPLY is %d ************\n",me, shared_data->outstanding_reply);
                    V(mutex_id);

                    V(wait_sem_id);
                     

                    
                // }
            }
            else if(k>-1)
            {
                // printf("Received message request value k=%d\n", k);
            
                // // printf("REQUEST RECEIVED Section\n");
                // printf("##########  REQUEST Section Starts  ##############\n");
                // printf("Message requested from node =%d\n",msg_req.from);  
                // printf("Message requested request number =%d\n",msg_req.req_value); 
                // printf("Message requested to node = %d\n",msg_req.to); 
                // printf("Message requested mtype = %d\n",msg_req.mtype); 
                // printf("**** HIGHEST REQUEST NUMBER IS =%d *****\n",shared_data->highest_request_number);
                // printf("######### REQUEST RECEIVED Section ENDS #########\n");
                // printf("Child -> K value is %d for node %d\n", k,me);
                P(mutex_id);
                if(k > shared_data->highest_request_number)
                {
                    shared_data->highest_request_number=k;
                    // printf("%%%% shared_data->highest_request_number =%d {inside k>highest if-block} %%%%%\n",shared_data->highest_request_number);
                }
                V(mutex_id);
                // printf("##########  REQUEST   HERE ##############\n");
                P(mutex_id);
                // printf("shared_data->CS_Request : %d\n",shared_data->CS_Request);
                // printf("k = %d\n",k);
                // printf("shared_data->request_number: %d\n",shared_data->request_number);
                // printf("Message Request from : %d\n",msg_req.from);
                defer_it = (shared_data->CS_Request) && ( (k>shared_data->request_number) || (k==shared_data->request_number && msg_req.from > me )  );
                V(mutex_id);
                // printf("##########  REQUEST   HERE 2##############\n");
                // printf("Node %d -> Defer value is %d for message requested node %d\n",me,defer_it,msg_req.from);
                if(defer_it)
                    {
                        P(mutex_id);
                        shared_data->reply_deferred[msg_req.from]=1;
                        // printf("##### INSIDE DEFER_IT BLOCK #####\n");
                        // printf("Node Number is %d and reply_deferred[%d]=%d\n",msg_req.from,msg_req.from,shared_data->reply_deferred[msg_req.from]);
                        V(mutex_id);
                      

                    }
                else
                    {
                       
                        // printf("######## CHECKING REPLY DEFERRED BUFFER ARRAY (IN CHILD WHERE Defer !=1) ######\n");
                        // for(int i=1;i<=N;i++){
                        //     if(i!=me)
                        //     {
                        //         printf("Node : %d Value : %d\n",i,shared_data->reply_deferred[i]);
                        //     }
                        // }
                        Sync_msg msg_response;
                        msg_response.to = msg_req.from;
                        msg_response.from = me;
                        msg_response.req_value = -1;
                        msg_response.mtype = msg_req.from;
                        // printf("############  RESPONSE STARTS ############\n");
                        // printf("Message responsed from node =%d\n",msg_response.from); // 1 (as assumed my node is 1)
                        // // printf("Message response request number =%d\n",msg_response.req_value); // -1 as I am granting permission for accessing CS
                        // printf("Message response to node = %d\n",msg_response.to); // 2,3, or 4
                        // // printf("Message response mtype = %d\n",msg_response.mtype); //2,3, or 4
                        

                        if(msgsnd(sync_msg_queue_id,&msg_response,sizeof(msg_response),IPC_NOWAIT)==-1)
                        {
                            perror("MESSAGE REPLY FAILED IN CHILD PROCESS\n");
                        }
                        else
                        {
                            // printf("Message replied from %d to %d successfully\n",me,msg_response.to);
                            // printf("########### RESPONSE ENDS #############\n");
                        }
                    }
                
                // printf("######## CHECKING REPLY DEFERRED BUFFER ARRAY (IN-CHILD) ######\n");
                // P(mutex_id);
                // for(int i=1;i<=N;i++){
                //     if(i!=me)
                //     {
                //         printf("Node : %d Value : %d\n",i,shared_data->reply_deferred[i]);
                //     }
                // }  
                // V(mutex_id);
            }
            else
            {

            }
            
            //    sleep(rand()%5+me);
        }
       
        // while loop ends here ....
    }
    // parent block ends


    return 0;
}
