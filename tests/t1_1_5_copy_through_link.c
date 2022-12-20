#include "fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "prettyprint.h"

int main() {

    char *str_ext_file = "BBB!";
    char *path_copied_file = "/f1";
    char *path_link = "/link1";
    char *path_src = "tests/file_to_copy.txt";
    char buffer[40];

    assert(tfs_init(NULL) != -1);

    int f;
    ssize_t r;

    f = tfs_open(path_copied_file, TFS_O_CREAT);
    assert(f != -1);
    assert(tfs_close(f) != -1);

    // creates a soft link to the destination file
    assert(tfs_sym_link(path_copied_file, path_link) != -1);

    f = tfs_copy_from_external_fs(path_src, path_link);
    assert(f != -1);

    // Contents should be overwriten, not appended
    f = tfs_open(path_link, 0);
    r = tfs_read(f, buffer, sizeof(buffer) - 1);
    assert(r == strlen(str_ext_file));
    assert(!memcmp(buffer, str_ext_file, strlen(str_ext_file)));

    PRINT_GREEN("Successful test.\n");

    return 0;
}
