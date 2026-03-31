#include "fs.h"
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <limits.h>

int unionfs_read(const char *path, char *buf, size_t size, off_t offset,
                 struct fuse_file_info *fi) {
    char resolved_path[PATH_MAX];
    int fd;
    int res;

    // Resolve path (upper or lower)
    if (resolve_path(path, resolved_path) != 0) {
        return -ENOENT;
    }

    // Open file (read-only)
    fd = open(resolved_path, O_RDONLY);
    if (fd == -1) {
        return -errno;
    }

    // Read using pread
    res = pread(fd, buf, size, offset);
    if (res == -1) {
        res = -errno;
    }

    // Close file
    close(fd);

    return res;
}


int unionfs_write(const char *path, const char *buf, size_t size, off_t offset,
                  struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_open(const char *path, struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_create(const char *path, mode_t mode,
                   struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_mkdir(const char *path, mode_t mode) {
    return -ENOENT;
}


int unionfs_rmdir(const char *path) {
    return -ENOENT;
}


int unionfs_unlink(const char *path) {
    return -ENOENT;
}


int unionfs_truncate(const char *path, off_t size,
                     struct fuse_file_info *fi) {
    return -ENOENT;
}


int unionfs_utimens(const char *path, const struct timespec ts[2],
                    struct fuse_file_info *fi) {
    return -ENOENT;
}