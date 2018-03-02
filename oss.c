#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

void displayHelp(char* prog);

int main(int argc, char* argv[]) {
   int c = 0;
   int hFlag = 0;
   int sFlag = 0;
   int lFlag = 0;
   int tFlag = 0;
   char* filename = "nesbitP3.log";
   int numSlaves = 5;
   int termTime = 20;
   extern char *optarg;
   extern int optind, optopt, opterr;

   while ((c = getopt(argc, argv, "hs:l:t:")) != -1) {
      switch(c) {
         case 'h':
            if (hFlag == 0) {
               displayHelp(argv[0]);
               hFlag++;
            }
            break;
         case 's':
            if (sFlag == 0) {
               numSlaves = atoi(optarg);
               printf("%s: number of slaves set to: %d\n", argv[0], numSlaves);
               sFlag++;
            }
            break;
         case 'l':
            if (lFlag == 0) {
               filename = optarg;
               printf("%s: filename set to: %s\n", argv[0], filename);
               lFlag++;
            }
            break;
         case 't':
            if (tFlag == 0) {
               termTime = atoi(optarg);
               printf("%s: termination time set to: %d\n", argv[0], termTime);
               tFlag++;
            }
            break;
      }
   }

   int i;
   for (i = 0; i < numSlaves; i++) {
      pid_t childpid;
      childpid = fork();
      switch(childpid) {
         case -1:
            fprintf(stderr, "%s: Error: Failed to fork slave process\n", argv[0]);
            break;
         case 0:
            printf("%s: Attempting to exec slaves process %ld\n", argv[0], (long)getpid());
            char* args[] = {"./user", NULL};
            if (execv(args[0], args) == -1)
               fprintf(stderr, "%s: Error: Failed to exec slave process %ld\n", argv[0], (long)getpid());
            break;
         default:
            //wait(NULL);
            break;
      }
   }

   wait(NULL);
   return 0;
}


/*************************************
 * DisplayHelp()                     *
 * ***********************************/
void displayHelp(char* prog) {
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
