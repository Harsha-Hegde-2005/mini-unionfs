#include "fs.h"
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

int resolve_path(const char *path, char *resolved_path) {
    struct stat st;
    char temp[PATH_MAX];
    struct mini_unionfs_state *state = UNIONFS_DATA;

    // whiteout check
    snprintf(temp, PATH_MAX, "%s/.wh.%s", state->upper_dir, path + 1);
    if (lstat(temp, &st) == 0) {
        return -ENOENT;
    }

    // upper
    snprintf(temp, PATH_MAX, "%s%s", state->upper_dir, path);
    if (lstat(temp, &st) == 0) {
        strcpy(resolved_path, temp);
        return 0;
    }

    // lower
    snprintf(temp, PATH_MAX, "%s%s", state->lower_dir, path);
    if (lstat(temp, &st) == 0) {
        strcpy(resolved_path, temp);
        return 0;
    }

    return -ENOENT;
}