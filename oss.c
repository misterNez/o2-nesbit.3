/* Created by: Nick Nesbit
 * Date: 3/4/2018
 * Message Passing and OS Simulator
 * CS 4760 Project 3
 * oss.c
*/

//Includes
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include "timer.h"
#include "message.h"

//Global variable to flag for termination
volatile sig_atomic_t term = 0;

//Function prototypes
void display_help(char* prog);
void handle_signal(int sig);

//Start of main program
int main(int argc, char* argv[]) {

   //Set up signal handling
   sigset_t mask;
   sigfillset(&mask);
   sigdelset(&mask, SIGINT);
   sigdelset(&mask, SIGALRM);
   sigdelset(&mask, SIGTERM);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   signal(SIGINT, handle_signal);
   signal(SIGALRM, handle_signal);

   //Shared memory
   Timer* timer;
   key_t key = ftok("/tmp", 35);
   int shmid = shmget(key, sizeof(&timer), IPC_CREAT | 0666);
   timer = shmat(shmid, NULL, 0);
   timer->secs = 0;
   timer->nanos = 0; 
 
   //Message queue
   struct msg_struc message;
   key = ftok("/tmp", 65);
   int msgid = msgget(key, IPC_CREAT | 0666);

   //Variables for getopt
   int c = 0;
   int hFlag = 0;
   int sFlag = 0;
   int lFlag = 0;
   int tFlag = 0;
   extern char *optarg;
   extern int optind, optopt, opterr;

   //Default utility variables
   char* filename = "nesbitP3.log";
   int numSlaves = 5;
   int termTime = 20;

   //Check for command line arguments
   while ((c = getopt(argc, argv, "hs:l:t:")) != -1) {
      switch(c) {
         //Help
         case 'h':
            if (hFlag == 0) {
               display_help(argv[0]);
               hFlag++;
            }
            break;
         //Set number of spawned processes
         case 's':
            if (sFlag == 0) {
               numSlaves = atoi(optarg);
               if (numSlaves > 17) numSlaves = 17;
               printf("%s: number of slaves set to: %d\n", argv[0], numSlaves);
               sFlag++;
            }
            break;
         //Set log filename
         case 'l':
            if (lFlag == 0) {
               filename = optarg;
               printf("%s: filename set to: %s\n", argv[0], filename);
               lFlag++;
            }
            break;
         //Set termination time
         case 't':
            if (tFlag == 0) {
               termTime = atoi(optarg);
               printf("%s: termination time set to: %d\n", argv[0], termTime);
               tFlag++;
            }
            break;
      }
   }

   //Start termination timer
   alarm(termTime);

   //Local variables
   int total = 0;     //Total number of spawned processes
   int count = 0;     //Current number of spawned processes
   pid_t pid;         //PID variable
   int status;        //Used for status info from child
   FILE* log;	      //File variable

   printf("%s: Computing... number of simultaneous children: %d, log filename: %s, termination time: %d\n", 
                                                                    argv[0], numSlaves, filename, termTime);

   //Main master loop
   while ( (total < 100) && (timer->secs < 2) && (term == 0) ) {

      //If resources allow another process
      if ( count < numSlaves) {
         //Fork a new child and increment counters
         pid = fork();
         total++;
         count++;
         switch(pid) {

            //Fork error
            case -1:
               fprintf(stderr, "%s: Error: Failed to fork slave process\n", argv[0]);
               count--;
               break;

            //Child process
            case 0:
               //Start of a critical section
               msgrcv(msgid, &message, sizeof(message), 1, 0);

               //Print message to file
               snprintf(message.str, sizeof(message), "Master: Creating new child pid %ld at my time %d.%d\n",
                                                                    (long)getpid(), timer->secs, timer->nanos);
               log = fopen(filename, "a");
               fprintf(log, message.str);
               fclose(log);

               //printf("%s: Attempting to exec child process %ld\n", argv[0], (long)getpid());

               //Exit critical section
               message.type = 2;
               msgsnd(msgid, &message, sizeof(message), 0);

               //Execute the user program
               char* args[] = {"./user", NULL};
               if (execv(args[0], args) == -1)
                  fprintf(stderr, "%s: Error: Failed to exec child process %ld\n", argv[0], (long)getpid());

               //Exit if exec error
               exit(1);

            //Parent process
            default:
               //Some message handling
               message.type = 1;
               msgsnd(msgid, &message, sizeof(message), 0);       //Sent to line 134
               msgrcv(msgid, &message, sizeof(message), 2, 0);    //Received from line 147
               message.type = 3;
               msgsnd(msgid, &message, sizeof(message), 0);       //Sent to the user process

               //Enter critical section if message received from user process
               if ( (msgrcv(msgid, &message, sizeof(message), (long)getpid(), IPC_NOWAIT) != -1) ) {
                  //Write message to file
                  log = fopen(filename, "a");
                  fprintf(log, message.str);
                  fclose(log);

                  //Increment the timer 100 nanos and adjust
                  timer->nanos += 100;
                  while (timer->nanos >= 1000000000) {
                     timer->nanos -= 1000000000;
                     timer->secs++;
                  }
                  
                  //Decrement the count
                  pid = wait(&status);
                  count--;
                  //printf("%s: Process %ld finished(1). %d child processes running.\n", argv[0], (long)pid, count);
               }
               break;
         }
         //End of switch pid statement
      }

      //Else If max number of children is reached
      else {
         //Receive message and write to file
         msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
         log = fopen(filename, "a");
         fprintf(log, message.str);
         fclose(log);

         //Increment the timer 100 nanos and adjust
         timer->nanos += 100;
         while (timer->nanos >= 1000000000) {
            timer->nanos -= 1000000000;
            timer->secs++;
         }

         //Decrement the count
         pid = wait(&status);
         count--;
         //printf("%s: Process %ld finished(2). %d child processes running.\n", argv[0], (long)pid, count);
      }
      //End of if/else statement
   }
   //End of master while loop

   //For any remaining messages
   int k = 0;
   for ( ; k < count; k++) {
      //Receive message and write to file
      msgrcv(msgid, &message, sizeof(message), (long)getpid(), 0);
      log = fopen(filename, "a");
      fprintf(log, message.str);
      fclose(log);
     
      //Increment the timer 100 nanos and adjust
      timer->nanos += 100;
      while (timer->nanos >= 1000000000) {
         timer->nanos -= 1000000000;
         timer->secs++;
      }
   } 

   //Wait for all children to finish
   while ( (pid = wait(&status)) > 0) {
      count--;
      //printf("%s: Process %ld finished(3). %d child processes running.\n", argv[0], (long)pid, count);
   }

   //Report cause of termination
   if (term == 1)
      printf("%s: Ended because of user interrupt\n", argv[0]);
   else if (term == 2)
      printf("%s: Ended because of timeout\n", argv[0]);
   else if (total == 100) 
      printf("%s: Ended because total = 100\n", argv[0]);
   else
      printf("%s: Ended because virtual timer reached 2 seconds\n", argv[0]);

   //Print info
   printf("%s: Program ran for %d.%d and generated %d user processes\n", argv[0], timer->secs, timer->nanos, total);

   //Free allocated memory
   shmdt(timer);
   shmctl(shmid, IPC_RMID, NULL);
   msgctl(msgid, IPC_RMID, NULL);

   //End program
   return 0;
}
//End of main program


/****************************************
* Function definitions                  *
****************************************/

//display_help
void display_help(char* prog) {
   printf("%s:\tHelp: Operating Systems Project 3\n"
           "\tMessage Queue implementation\n"
           "\tTo run: ./oss (default values set)\n"
           "\tOptions:\n"
           "\t\t-h : help menu\n"
           "\t\t-s [integer]: set number of slave processes (default is 5)\n"
           "\t\t-l [filename]: set the name of the produced log file (default is nesbitP3.log)\n"
           "\t\t-t [integer]: set timeout in seconds (default is 20)\n"
           "\tExample: ./oss -s 8 -l file.log -t 30\n", prog);
}

//handle_signal
void handle_signal(int sig) {
   printf("./oss: Parent process %ld caught signal: %d. Cleaning up and terminating.\n", (long)getpid(), sig);
   switch(sig) {
      case SIGINT:
         kill(0, SIGUSR1);
         term = 1;
         break;
      case SIGALRM:
         kill(0, SIGUSR2);
         term = 2;
         break;
   }
}
