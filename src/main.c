#include "fs.h"
#include <stdlib.h>
#include <stdio.h>

extern struct fuse_operations unionfs_oper;

int main(int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: %s <lower_dir> <upper_dir> <mount>\n", argv[0]);
        return 1;
    }

    struct mini_unionfs_state *state = malloc(sizeof(struct mini_unionfs_state));

    state->lower_dir = realpath(argv[1], NULL);
    state->upper_dir = realpath(argv[2], NULL);

    printf("Lower: %s\nUpper: %s\n", state->lower_dir, state->upper_dir);

    char *fuse_argv[] = { argv[0], argv[3], "-f" };

    return fuse_main(3, fuse_argv, &unionfs_oper, state);
}
