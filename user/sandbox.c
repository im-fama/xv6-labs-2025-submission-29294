#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc < 4) {
        fprintf(2, "usage: sandbox mask path command [args...]\n");
        exit(1);
    }

    int mask = atoi(argv[1]);
    char *path = argv[2];

    if (fork() == 0) {
        // Child: set restrictions
        if (interpose(mask, path) < 0) {
            fprintf(2, "sandbox: interpose failed\n");
            exit(1);
        }
        // Execute the command under sandbox
        exec(argv[3], &argv[3]);
        fprintf(2, "sandbox: exec %s failed\n", argv[3]);
        exit(1);
    } else {
        // Parent waits
        wait(0);
    }

    exit(0);
}
