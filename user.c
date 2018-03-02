#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
   printf("%s: Hello from slave process %ld\n", argv[0], (long)getpid());
   return 0;
}
