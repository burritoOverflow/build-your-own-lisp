#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INBUFFSIZE 2048

static char input[INBUFFSIZE];

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  printf("DIY Lispy\n");
  printf("Press C-c to Exit.\n\n");

  while (true) {
    printf("DIYLispy > ");

    if (fgets(input, INBUFFSIZE, stdin) != NULL) {
      // Ignore newline character if present
      input[strcspn(input, "\n")] = '\0';

      printf("You entered: %s\n", input);
    } else {
      // handle EOF (C-d)
      if (feof(stdin)) {
        printf("\nExiting.\n");
        exit(EXIT_SUCCESS);
      }
      perror("Error reading input from prompt");
    }
  }
}