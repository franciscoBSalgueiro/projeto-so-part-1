#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "prettyprint.h"

uint8_t const file_contents[] = "This is a string to test links";
char const target_path1[] = "/file1";
char const link_path1[] = "/link1";
char const target_path2[] = "/file2";
char const link_path2[] = "/link2";
char const link_path3[] = "/link3";

void assert_contents_ok(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

// asserts that the file is empty
void assert_empty_file(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
}

// writes the contents to the file
void write_contents(char const *path) {
    int f = tfs_open(path, 0);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
}

int main() {
    assert(tfs_init(NULL) != -1);

    // creates a file and opens it
    int f = tfs_open(target_path1, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);

    // sanity check
    assert_empty_file(target_path1);

    // writes to the file
    write_contents(target_path1);
    assert_contents_ok(target_path1);

    // creates a hard link to the target
    assert(tfs_link(target_path1, link_path1) != -1);
    assert_contents_ok(link_path1);

    // creates a hard link to the previous hard link
    assert(tfs_link(link_path1, link_path2) != -1);
    assert_contents_ok(link_path2);

    // creates a soft link to the previous hard link
    assert(tfs_sym_link(link_path2, link_path3) != -1);
    assert_contents_ok(link_path3);

    // unlinks the middle hard link, so the soft link is now broken
    assert(tfs_unlink(link_path2) != -1);

    // tries to open the soft link, which should fail
    f = tfs_open(link_path3, 0);
    assert(f == -1);

    // unlinks the first hard link
    assert(tfs_unlink(link_path1) != -1);

    // Tries to create the middle link again, but the first hardlink was
    // deleted, so it doesn't work
    assert(tfs_link(link_path1, link_path2) == -1);

    PRINT_GREEN("Successful test.\n");
    return 0;
}