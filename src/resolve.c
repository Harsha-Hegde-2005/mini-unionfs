#include "fs.h"
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

/*
 * resolve_path - Determine the real filesystem path for a virtual path.
 *
 * Resolution order (highest to lowest priority):
 *   1. Whiteout check : if upper/subdir/.wh.<name> exists -> ENOENT (deleted)
 *   2. Upper layer    : if upper/<path> exists            -> return upper path
 *   3. Lower layer    : if lower/<path> exists            -> return lower path
 *   4. Not found      : return -ENOENT
 *
 * Edge cases handled:
 *   - Root path "/" is handled directly in getattr, not passed here
 *   - Whiteout files (.wh.*) are never exposed to the user
 *   - Upper layer always shadows same-named file in lower
 *   - Subdirectory paths like /subdir/file handled correctly
 */
int resolve_path(const char *path, char *resolved_path) {
    struct stat st;
    char temp[PATH_MAX];
    struct mini_unionfs_state *state = UNIONFS_DATA;

    /* Extract basename and directory part from path.
     * e.g. "/subdir/file.txt"  ->  dir_part="/subdir"  basename="file.txt"
     *      "/file.txt"         ->  dir_part=""          basename="file.txt"  */
    const char *basename = strrchr(path, '/');
    basename = basename ? basename + 1 : path + 1;

    char dir_part[PATH_MAX];
    strncpy(dir_part, path, PATH_MAX);
    char *last_slash = strrchr(dir_part, '/');
    if (last_slash && last_slash != dir_part)
        *last_slash = '\0';   /* keep "/subdir" */
    else
        strcpy(dir_part, ""); /* root-level file — no dir part */

    /* Step 1: Check for whiteout marker in upper directory.
     * Correct format: upper_dir/subdir/.wh.filename
     * This correctly handles both root-level and nested paths. */
    snprintf(temp, PATH_MAX, "%s%s/.wh.%s",
             state->upper_dir, dir_part, basename);
    if (lstat(temp, &st) == 0)
        return -ENOENT;   /* Whiteout exists — treat file as deleted */

    /* Step 2: Check upper layer */
    snprintf(temp, PATH_MAX, "%s%s", state->upper_dir, path);
    if (lstat(temp, &st) == 0) {
        strncpy(resolved_path, temp, PATH_MAX);
        return 0;
    }

    /* Step 3: Check lower layer */
    snprintf(temp, PATH_MAX, "%s%s", state->lower_dir, path);
    if (lstat(temp, &st) == 0) {
        strncpy(resolved_path, temp, PATH_MAX);
        return 0;
    }

    return -ENOENT;
}
