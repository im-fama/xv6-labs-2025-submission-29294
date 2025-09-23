#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"
#include "kernel/stat.h"

// check if a character is a separator
int issep(char c) {
  char *seps = " -\r\t\n./,";
  for (int i = 0; seps[i]; i++) {
    if (c == seps[i]) return 1;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(2, "Usage: sixfive file...\n");
    exit(1);
  }

  for (int f = 1; f < argc; f++) {
    int fd = open(argv[f], 0);
    if (fd < 0) {
      fprintf(2, "sixfive: cannot open %s\n", argv[f]);
      exit(1);
    }

    char buf[1];
    char numbuf[32]; // store digits of one number
    int nbuf = 0;

    while (read(fd, buf, 1) == 1) {
      char c = buf[0];

      if (c >= '0' && c <= '9') {
        // part of a number
        if (nbuf < sizeof(numbuf) - 1) {
          numbuf[nbuf++] = c;
        }
      } else if (issep(c)) {
        // end of a number
        if (nbuf > 0) {
          numbuf[nbuf] = '\0';
          int val = atoi(numbuf);
          if (val % 5 == 0 || val % 6 == 0) {
            printf("%d\n", val);
          }
          nbuf = 0; // reset buffer
        }
      } else {
        // non-separator non-digit (like letters in "xv6")
        nbuf = 0;
      }
    }

    // if file ends with a number, process it
    if (nbuf > 0) {
      numbuf[nbuf] = '\0';
      int val = atoi(numbuf);
      if (val % 5 == 0 || val % 6 == 0) {
        printf("%d\n", val);
      }
    }

    close(fd);
  }

  exit(0);
}

