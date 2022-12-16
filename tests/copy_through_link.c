#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "prettyprint.h"

int main() {
    
    char *str_file = "This is a string to test copy function";
    char *path_src = "/file1";
    char *path_link = "/link1";
    char *path_copied_file = "/file2";

    assert(tfs_init(NULL) != -1);

    int f_source = tfs_open(path_src, TFS_O_CREAT);
    assert(f_source != -1);
    int f_destiny = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f_destiny != -1);

    // writes to the source file
    assert(tfs_write(f_source, str_file, sizeof(str_file)) != -1);

    assert(tfs_close(f_source) != -1);
    assert(tfs_close(f_destiny) != -1);

    // deletes the source file
    assert(tfs_unlink(path_src) != -1);

    //  tries to copy from path, which doesn't exist
    assert(tfs_copy_from_external_fs(path_src, path_copied_file) == -1);

    // creates the source file again
    f_destiny = tfs_open(path_src, TFS_O_CREAT);
    assert(f_destiny != -1);

    assert(tfs_write(f_source, str_file, sizeof(str_file)) != -1);

    assert(tfs_close(f_source) != -1);

    // creates a soft link to the destination file
    assert(tfs_sym_link(path_copied_file, path_link) != -1);

    // copies the content from source to the destination by a soft link
    assert(tfs_copy_from_external_fs(path_src, path_link) == -1);
    
    PRINT_GREEN("Successful test\n");
}