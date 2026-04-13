#!/bin/bash
# Automated test suite for Mini-UnionFS
# As specified in Appendix B of the project manual

FUSE_BINARY="./build/mini_unionfs"
TEST_DIR="./unionfs_test_env"
LOWER_DIR="$TEST_DIR/lower"
UPPER_DIR="$TEST_DIR/upper"
MOUNT_DIR="$TEST_DIR/mnt"

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

PASSED=0
FAILED=0

check() {
    local label="$1"
    local cmd="$2"
    if eval "$cmd"; then
        echo -e "${GREEN}PASSED${NC} -- $label"
        ((PASSED++))
    else
        echo -e "${RED}FAILED${NC} -- $label"
        ((FAILED++))
    fi
}

echo "========================================"
echo "   Mini-UnionFS Automated Test Suite"
echo "========================================"

rm -rf "$TEST_DIR"
mkdir -p "$LOWER_DIR" "$UPPER_DIR" "$MOUNT_DIR"
echo "base_only_content"  > "$LOWER_DIR/base.txt"
echo "to_be_deleted"      > "$LOWER_DIR/delete_me.txt"
echo "upper_only_content" > "$UPPER_DIR/upper.txt"

"$FUSE_BINARY" "$LOWER_DIR" "$UPPER_DIR" "$MOUNT_DIR"
sleep 1

check "Layer visibility: lower file visible in mount" \
    "grep -q 'base_only_content' '$MOUNT_DIR/base.txt' 2>/dev/null"

check "Layer visibility: upper file visible in mount" \
    "grep -q 'upper_only_content' '$MOUNT_DIR/upper.txt' 2>/dev/null"

echo "modified_content" >> "$MOUNT_DIR/base.txt" 2>/dev/null
sleep 0.2

check "CoW: modified content visible through mount" \
    "grep -q 'modified_content' '$MOUNT_DIR/base.txt' 2>/dev/null"

check "CoW: modified file promoted to upper layer" \
    "grep -q 'modified_content' '$UPPER_DIR/base.txt' 2>/dev/null"

check "CoW: lower layer file unchanged" \
    "! grep -q 'modified_content' '$LOWER_DIR/base.txt' 2>/dev/null"

rm "$MOUNT_DIR/delete_me.txt" 2>/dev/null
sleep 0.2

check "Whiteout: deleted file hidden from mount" \
    "[ ! -f '$MOUNT_DIR/delete_me.txt' ]"

check "Whiteout: original file intact in lower layer" \
    "[ -f '$LOWER_DIR/delete_me.txt' ]"

check "Whiteout: .wh. marker created in upper layer" \
    "[ -f '$UPPER_DIR/.wh.delete_me.txt' ]"

touch "$MOUNT_DIR/newfile.txt" 2>/dev/null
sleep 0.2

check "Create: new file appears in upper layer" \
    "[ -f '$UPPER_DIR/newfile.txt' ]"

echo "Hello New" > "$MOUNT_DIR/newfile.txt" 2>/dev/null
sleep 0.2

check "Write: content written correctly to upper layer" \
    "grep -q 'Hello New' '$UPPER_DIR/newfile.txt' 2>/dev/null"

mkdir "$MOUNT_DIR/newdir" 2>/dev/null
sleep 0.2

check "mkdir: new directory created in upper layer" \
    "[ -d '$UPPER_DIR/newdir' ]"

rmdir "$MOUNT_DIR/newdir" 2>/dev/null
sleep 0.2

check "rmdir: directory removed from upper layer" \
    "[ ! -d '$UPPER_DIR/newdir' ]"

fusermount -u "$MOUNT_DIR" 2>/dev/null || umount "$MOUNT_DIR" 2>/dev/null
rm -rf "$TEST_DIR"

echo "========================================"
echo "  Results: $PASSED passed, $FAILED failed"
echo "========================================"

[ "$FAILED" -eq 0 ] && exit 0 || exit 1

