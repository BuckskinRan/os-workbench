#include <stdio.h>
#include <string.h>
#include <stdlib.h>
void defFunction(){

}
int main(int argc, char *argv[]) {
  static char line[4096];
  while (1) {
    printf("crepl> ");
    fflush(stdout);
    if (!fgets(line, sizeof(line), stdin)) {
      break;
    }
    if (strlen(line) > 2) {
      if (strncmp(line, "int ", 3) == 0) {
        printf("try to define a function\n");
        printf("%s", line);
        char template[] = "tmp-XXXXXX";
        mkstemp(template);
        continue;
      }
    }
    printf("try to use an expression\n");
    continue;
    // printf("Got %zu chars.\n", strlen(line)); // WTF?
  }
}
