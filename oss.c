/*Created by: Nick Nesbit
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
#include "timer.h"

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
   sigprocmask(SIG_SETMASK, &mask, NULL);
   signal(SIGINT, handle_signal);
   signal(SIGALRM, handle_signal);

   //Shared memory
   Timer* timer;
   key_t key = ftok("shmclock", 35);
   int shmid = shmget(key, sizeof(&timer), IPC_CREAT | 0666);
   timer = shmat(shmid, NULL, 0);
   timer->secs = 0;
   timer->nanos = 0; 
 
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

   int i = 0;       //Child index number
   int count = 0;   //Total number of spawned processes
   pid_t pid;       //PID
   int status;      //

   //Main while loop
   while ( (i < 100) && (timer->secs < 2) && (term == 0) ) {
      if ( (count < numSlaves) && (count <  18) ) {
         pid = fork();
         switch(pid) {
            case -1:
               fprintf(stderr, "%s: Error: Failed to fork slave process\n", argv[0]);
               break;
            case 0:
               i++;
               count++;
               printf("%s: Attempting to exec slaves process %ld\n", argv[0], (long)getpid());
               char* args[] = {"./user", NULL};
               if (execv(args[0], args) == -1)
                  fprintf(stderr, "%s: Error: Failed to exec slave process %ld\n", argv[0], (long)getpid());
               exit(1);
            default:
               //Receive message
               //Write message to log file
               //CS() add 100 to clock
               while ( (pid = waitpid(-1, &status, WNOHANG)) > 0 ) {
                  count--;
                  printf("%s: Process %ld finished(1). %d child processes running.\n", argv[0], (long)pid, count);
               }
               break;
         }
      }
   }

   while ( (pid = wait(&status)) > 0) {
      count--;
      printf("%s: Process %ld finished(2). %d child processes running.\n", argv[0], (long)pid, count);
   }

   shmdt(timer);
   shmctl(shmid, IPC_RMID, NULL);      

   return 0;
}


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

void handle_signal(int sig) {
   printf("Process %ld caught signal: %d. Cleaning up and terminating.\n", (long)getpid(), sig);
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
