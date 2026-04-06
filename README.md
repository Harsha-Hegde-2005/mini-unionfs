# Mini-UnionFS using FUSE
A simplified Union File System implemented in userspace using FUSE.
Merges a read-only lower directory and a writable upper directory
into a single unified mount point — similar to how Docker OverlayFS works.
## Team 18 — Section D
| Name | SRN | Branch |
|---------------------|-----------------|---------------------------|
| Harsha Madev Hegde | PES2UG23CS212 | feature/core-fuse-setup |
| Gudihalli Kiran | PES2UG23CS207 | feature/path-resolution |
| Harshavardhan N | PES2UG23CS214 | feature/file-operations |
## Features- Layer stacking: upper directory takes precedence over lower- Copy-on-Write (CoW): modifications to lower files are promoted to upper- Whiteout: deletions create `.wh.<filename>` markers hiding lower files- Supports: getattr, readdir, read, write, create, unlink, mkdir, rmdir- Bonus: open() with CoW, truncate(), utimens()
## Project Structure
```
mini-unionfs/
 src/
 fs.h # Shared header — struct, macro, declarations
 main.c # FUSE entry point
 resolve.c # Path resolution logic
 fs.c # getattr and readdir
 file_ops.c # All file operations
 tests/
 test_unionfs.sh # Automated test suite
 build/ # Compiled binary (generated)
 Makefile
```
## Build
```bash
# Install FUSE development libraries first
sudo apt install libfuse3-dev fuse3
make clean && make
```
## Usage
```bash
# Create test directories
mkdir -p test/lower test/upper test/mnt
# Add a file to lower layer
echo "Hello from lower" > test/lower/file1.txt
# Mount the filesystem
./build/mini_unionfs test/lower test/upper test/mnt
```
## Testing (new terminal)
```bash
ls test/mnt # verify merged view
cat test/mnt/file1.txt # read from lower layer
echo "NEW DATA" >> test/mnt/file1.txt # trigger Copy-on-Write
cat test/upper/file1.txt # file promoted to upper
cat test/lower/file1.txt # lower unchanged
rm test/mnt/file1.txt # trigger whiteout
ls -a test/upper # see .wh.file1.txt marker
```
## Run Automated Tests
```bash
chmod +x tests/test_unionfs.sh
./tests/test_unionfs.sh
```
## Unmount
```bash
fusermount -u test/mnt
```
## Limitations- No advanced permission handling- Basic nested directory support- No caching optimisation
## References- FUSE documentation: https://libfuse.github.io/doxygen/- Docker OverlayFS: https://docs.docker.com/storage/storagedriver/overlayfs-driver/
