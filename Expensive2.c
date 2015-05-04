#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {

  struct timeval tv1, tv2;

  gettimeofday(&tv1, NULL);
  sleep(5);
  usleep(400000);
  gettimeofday(&tv2, NULL);

  long running_time = tv2.tv_sec - tv1.tv_sec;
  
  printf("Process running time: %lds\n", running_time);

  return 0;
}
