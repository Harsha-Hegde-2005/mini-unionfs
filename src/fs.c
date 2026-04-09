#include "fs.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>

/* Forward declarations for all operations defined in file_ops.c */
extern int resolve_path(const char *, char *);
extern int unionfs_open(const char *, struct fuse_file_info *);
extern int unionfs_read(const char *, char *, size_t, off_t,
                        struct fuse_file_info *);
extern int unionfs_write(const char *, const char *, size_t, off_t,
                         struct fuse_file_info *);
extern int unionfs_truncate(const char *, off_t, struct fuse_file_info *);
extern int unionfs_unlink(const char *);
extern int unionfs_create(const char *, mode_t, struct fuse_file_info *);
extern int unionfs_mkdir(const char *, mode_t);
extern int unionfs_rmdir(const char *);
extern int unionfs_utimens(const char *, const struct timespec [2],
                           struct fuse_file_info *);

/* ============================================================
 * unionfs_getattr - Return file / directory attributes.
 *
 * Called by the kernel for every stat(), ls, open, access, etc.
 * Returns -ENOENT if a whiteout exists or the file is in neither layer.
 * ============================================================ */
static int unionfs_getattr(const char *path, struct stat *stbuf,
                           struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));

    /* Root directory is always present */
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode  = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }

    char resolved[PATH_MAX];
    int ret = resolve_path(path, resolved);
    if (ret != 0)
        return ret;

    return lstat(resolved, stbuf);
}

/* ============================================================
 * unionfs_readdir - List directory as a merged view of upper + lower.
 *
 * Algorithm:
 *   1. List upper dir — add all non-whiteout entries, track in seen[]
 *   2. List lower dir — skip entries that:
 *        a. Have a whiteout marker in upper  (logically deleted)
 *        b. Already appear in seen[]          (upper version takes priority)
 *
 * Uses dynamic allocation for seen[] to avoid fixed-array overflow.
 * ============================================================ */
static int unionfs_readdir(const char *path, void *buf,
                           fuse_fill_dir_t filler, off_t offset,
                           struct fuse_file_info *fi,
                           enum fuse_readdir_flags flags) {

    struct mini_unionfs_state *state = UNIONFS_DATA;
    DIR *dp;
    struct dirent *de;

    filler(buf, ".",  NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    /* Dynamic seen[] — grows as needed, avoids stack overflow */
    int    seen_cap   = 256;
    int    seen_count = 0;
    char **seen = malloc(seen_cap * sizeof(char *));
    if (!seen) return -ENOMEM;

    /* ---------- Upper layer ---------- */
    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    dp = opendir(upper_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            /* Never expose .wh.* whiteout markers to the user */
            if (strncmp(de->d_name, ".wh.", 4) == 0)
                continue;

            filler(buf, de->d_name, NULL, 0, 0);

            /* Track entry in seen[] — grow dynamically if needed */
            if (seen_count == seen_cap) {
                seen_cap *= 2;
                seen = realloc(seen, seen_cap * sizeof(char *));
                if (!seen) return -ENOMEM;
            }
            seen[seen_count++] = strdup(de->d_name);
        }
        closedir(dp);
    }

    /* ---------- Lower layer ---------- */
    char lower_path[PATH_MAX];
    snprintf(lower_path, PATH_MAX, "%s%s", state->lower_dir, path);

    dp = opendir(lower_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {
            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            /* Check whiteout for this entry in the correct subdirectory.
             * Path: upper_dir/<current_dir>/.wh.<filename>            */
            char whiteout[PATH_MAX];
            snprintf(whiteout, PATH_MAX, "%s%s/.wh.%s",
                     state->upper_dir, path, de->d_name);
            if (access(whiteout, F_OK) == 0)
                continue;   /* Whiteout found — skip this entry */

            /* Skip if already listed from upper layer */
            int found = 0;
            for (int i = 0; i < seen_count; i++) {
                if (strcmp(seen[i], de->d_name) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found)
                filler(buf, de->d_name, NULL, 0, 0);
        }
        closedir(dp);
    }

    for (int i = 0; i < seen_count; i++) free(seen[i]);
    free(seen);
    return 0;
}

/* ============================================================
 * Register all FUSE operation handlers.
 * Operations implemented here: getattr, readdir.
 * All others are implemented in file_ops.c.
 * ============================================================ */
struct fuse_operations unionfs_oper = {
    .getattr  = unionfs_getattr,
    .readdir  = unionfs_readdir,
    .open     = unionfs_open,
    .read     = unionfs_read,
    .write    = unionfs_write,
    .truncate = unionfs_truncate,
    .unlink   = unionfs_unlink,
    .create   = unionfs_create,
    .mkdir    = unionfs_mkdir,
    .rmdir    = unionfs_rmdir,
    .utimens  = unionfs_utimens,
};
