CC = gcc
CFLAGS = `pkg-config fuse3 --cflags`
LDFLAGS = `pkg-config fuse3 --libs`

SRC = src/main.c src/fs.c src/resolve.c src/file_ops.c
TARGET = build/mini_unionfs

all:
	mkdir -p build
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf build
