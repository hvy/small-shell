#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[], char *envp[]) {
  int i, j, k;
  int sum = 0;
  for (i = 0; i < 1000; i++) {
    for (j = 0; j < 1000; j++) {
      for (k = 0; k < 1000; k++) {
        sum = sum + i % 100 + j % 200 + k % 300;
      }
    }
  }
  printf("%d\n", sum);
  return EXIT_SUCCESS;
}
