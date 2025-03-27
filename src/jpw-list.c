#include "jpw-list.h"

void list_available(char * exe);
void list_installed();

void jpw_list(int argc, char **argv) {
  if (argc != 3) {
    fprintf(stderr, "error: command 'list' requires exactly one of the following arguments: {available | installed}\n");
    exit(1);
  }

  if (strcmp(argv[2], "available") == 0) {
    list_available(argv[0]);
  } else if (strcmp(argv[2], "installed") == 0) {
    list_installed();
  } else {
    invalid_argument(argv[0], argv[1], argv[2]);
  }
}

void list_available(char * exe) {
  if (access(DBA, F_OK) != 0) {
    fprintf(stderr, "error: available package database has not been initialized, run '%s update available'\n", exe);
    exit(1);
  }

  FILE * file = fopen(DBA, "r");

  if (!file) {
    fprintf(stderr, "error: failed to read from available package database\n");
    exit(1);
  }

  char *line = NULL;
  size_t n;
  while (getline(&line, &n, file) != -1) {
    size_t len = strlen(line);
    if (len == 0 || isspace(line[0])) continue;

    if (line[len - 1] == '\n')
      printf("%s", line);
    else
      printf("%s\n", line);
  }

  free(line);
  fclose(file);
}

void list_installed() {
  fprintf(stderr, "list installed not implemented\n");
  exit(1);
}
