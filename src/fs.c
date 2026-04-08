#include "fs.h"
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>

extern int resolve_path(const char *, char *);

static int unionfs_getattr(const char *path, struct stat *stbuf,
                          struct fuse_file_info *fi) {

    memset(stbuf, 0, sizeof(struct stat));

    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        return 0;
    }

    char resolved[PATH_MAX];
    if (resolve_path(path, resolved) != 0)
        return -ENOENT;

    return lstat(resolved, stbuf);
}

// ------- READDIR -----------
static int unionfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                          off_t offset, struct fuse_file_info *fi,
                          enum fuse_readdir_flags flags) {

    struct mini_unionfs_state *state = UNIONFS_DATA;
    DIR *dp;
    struct dirent *de;

    filler(buf, ".", NULL, 0, 0);
    filler(buf, "..", NULL, 0, 0);

    char seen[1024][256];
    int seen_count = 0;

    // -------- UPPER --------
    char upper_path[PATH_MAX];
    snprintf(upper_path, PATH_MAX, "%s%s", state->upper_dir, path);

    dp = opendir(upper_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {

            if (strncmp(de->d_name, ".wh.", 4) == 0)
                continue;

            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            filler(buf, de->d_name, NULL, 0, 0);
            strcpy(seen[seen_count++], de->d_name);
        }
        closedir(dp);
    }

    // -------- LOWER --------
    char lower_path[PATH_MAX];
    snprintf(lower_path, PATH_MAX, "%s%s", state->lower_dir, path);

    dp = opendir(lower_path);
    if (dp) {
        while ((de = readdir(dp)) != NULL) {

            if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
                continue;

            //  CORRECT WHITEOUT CHECK
            char whiteout[PATH_MAX];
            snprintf(whiteout, PATH_MAX, "%s%s/.wh.%s",
                     state->upper_dir, path, de->d_name);

            if (access(whiteout, F_OK) == 0)
                continue;

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

    return 0;
}

// REGISTER ALL OPS
extern int unionfs_read(const char *, char *, size_t, off_t, struct fuse_file_info *);
extern int unionfs_write(const char *, const char *, size_t, off_t, struct fuse_file_info *);
extern int unionfs_unlink(const char *);
extern int unionfs_utimens(const char *, const struct timespec [2],
                           struct fuse_file_info *);

struct fuse_operations unionfs_oper = {
    .getattr = unionfs_getattr,
    .readdir = unionfs_readdir,
    .read    = unionfs_read,
    .write   = unionfs_write,
    .unlink  = unionfs_unlink,
    .create  = unionfs_create, 
    .mkdir   = unionfs_mkdir,  
    .rmdir   = unionfs_rmdir,  
    .utimens = unionfs_utimens,
};