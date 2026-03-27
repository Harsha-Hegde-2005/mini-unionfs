#ifndef FS_H
#define FS_H

#define FUSE_USE_VERSION 31

#include <fuse.h>
#include <limits.h>

// ================= STATE =================

struct mini_unionfs_state {
    char *lower_dir;
    char *upper_dir;
};

#define UNIONFS_DATA ((struct mini_unionfs_state *) fuse_get_context()->private_data)

// ================= FUNCTIONS =================

// path resolution
int resolve_path(const char *path, char *resolved_path);

// file operations (IMPORTANT)
int unionfs_read(const char *path, char *buf, size_t size,
                 off_t offset, struct fuse_file_info *fi);

int unionfs_write(const char *path, const char *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi);

int unionfs_unlink(const char *path);

int unionfs_create(const char *path, mode_t mode,
                   struct fuse_file_info *fi);

int unionfs_mkdir(const char *path, mode_t mode);

int unionfs_rmdir(const char *path);

int unionfs_utimens(const char *, const struct timespec [2],
                    struct fuse_file_info *);

#endif
