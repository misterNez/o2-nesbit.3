#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include "timer.h"

volatile sig_atomic_t term = 0;

void handle_signal(int sig);

int main(int argc, char* argv[]) {
   sigset_t mask;
   sigfillset(&mask);
   sigdelset(&mask, SIGUSR1);
   sigdelset(&mask, SIGUSR2);
   sigprocmask(SIG_SETMASK, &mask, NULL);
   signal(SIGUSR1, handle_signal);
   signal(SIGUSR2, handle_signal);

   Timer* timer;
   key_t key = ftok("shmclock", 35); 
   int shmid = shmget(key, sizeof(&timer), 0666);
   timer = shmat(shmid, NULL, 0);

   printf("%s: Hello from slave process %ld. Seconds: %d, Nanoseconds: %d\n", 
                     argv[0], (long)getpid(), timer->secs, timer->nanos);
   
   while (term = 0)
      sleep(3);
   
   shmdt(timer);
   
   return 0;
}

void handle_signal(int sig) {
   printf("Process %ld caught signal: %d. Terminating...\n", (long)getpid(), sig);
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
