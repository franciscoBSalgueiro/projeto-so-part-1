#include "fs/operations.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "prettyprint.h"

#define NUM_THREADS 10

uint8_t const file_contents[] = "AAA!";
const char path[] = "/f1";


void *read_file(void* args)
{
    (void) args;
    int f = tfs_open(path, 0);
    assert(f != -1);

    uint8_t buffer[sizeof(file_contents)];
    assert(tfs_read(f, buffer, sizeof(buffer)) == sizeof(buffer));
    assert(memcmp(buffer, file_contents, sizeof(buffer)) == 0);

    assert(tfs_close(f) != -1);
    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    assert(tfs_init(NULL) != -1);

    int f = tfs_open(path, TFS_O_CREAT);
    assert(f != -1);

    assert(tfs_write(f, file_contents, sizeof(file_contents)) ==
           sizeof(file_contents));

    assert(tfs_close(f) != -1);
    // Create the file 

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, read_file, NULL);
    }

    tfs_destroy();

    PRINT_GREEN("Successful test.\n");
}