#include "fs.h"
#include <stdlib.h>
#include <stdio.h>

/* Forward declaration of the operations struct defined in fs.c */
extern struct fuse_operations unionfs_oper;

int main(int argc, char *argv[]) {

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <lower_dir> <upper_dir> <mount_point>\n",
                argv[0]);
        return 1;
    }

    /* Allocate and populate global state */
    struct mini_unionfs_state *state =
        malloc(sizeof(struct mini_unionfs_state));
    if (!state) {
        fprintf(stderr, "Error: failed to allocate state\n");
        return 1;
    }

    state->lower_dir = realpath(argv[1], NULL);
    state->upper_dir = realpath(argv[2], NULL);

    if (!state->lower_dir || !state->upper_dir) {
        fprintf(stderr, "Error: invalid lower or upper directory path\n");
        free(state);
        return 1;
    }

    printf("Lower: %s\nUpper: %s\n", state->lower_dir, state->upper_dir);

    /* Pass only the mount point to FUSE; run in foreground (-f) */
    char *fuse_argv[] = { argv[0], argv[3], "-f" };
    return fuse_main(3, fuse_argv, &unionfs_oper, state);
}
