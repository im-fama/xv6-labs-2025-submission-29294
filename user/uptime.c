#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int ticks = uptime();   // call uptime syscall
  printf("%d ticks\n", ticks);
  exit(0);

}
