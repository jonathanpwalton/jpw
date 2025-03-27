#include "jpw.h"
#include "jpw-update.h"

static const struct {
  char const * const prg;
  char const * const ver;
  char const * const dsc;
} INFO = {
  .prg = "jpw",
  .ver = "2025.03.28",
  .dsc = "a simplicity-focused package manager for GNU/Linux"
};

char const * PROGRAM = "jpw";
char const * COMMAND = NULL;

int main (int argc, char ** argv) {
  if (argc == 1) {
    fprintf(stderr, "error: command not provided, try '%s help' for more information\n", argv[0]);
    exit(1);
  }

  FILE * f = fopen("/home/jonathan/dne/test.txt", "w");
  if (!f) {
    fprintf(stderr, ":(\n");
    exit (2);
  }
  fprintf(f, "hello\n");
  fclose(f);

  if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
    fprintf(stdout, "%s %s, %s\n", INFO.prg, INFO.ver, INFO.dsc);
    fprintf(stdout, "\nusage: %s <command>\n\n", argv[0]);
    fprintf(stdout,
      "commands:\n"
      "  update {all | available | installed}   fetches updates according to the provided argument\n"
      "  help                                   display this help text and exit\n"
    );
  } else if (strcmp(argv[1], "update") == 0) {
    update(argc, argv);
  } else {
    fprintf(stderr, "error: command '%s' not recognized, try '%s help' for more information\n", argv[1], argv[0]);
  }
}
