#include <stdio.h>

void write_log(const char *path, const char *message) {
  FILE *log = fopen(path, "a+");
  if (log == NULL) {
    fprintf(stderr, "Unable to open  %s\n", path);
    return;
  }

  fprintf(log, message);
  fclose(log);
}
