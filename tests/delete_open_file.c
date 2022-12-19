#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "prettyprint.h"

uint8_t const file_contents[] = "This is a string to test links";
char const target_path[] = "/file";

int main() {
    assert(tfs_init(NULL) != -1);

    int f = tfs_open(target_path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_close(f) != -1);

    assert(tfs_unlink(target_path) == -1);

    PRINT_GREEN("Successful test.\n");
    return 0;
}