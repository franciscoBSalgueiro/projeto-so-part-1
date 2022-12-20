#include "../fs/operations.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "prettyprint.h"

#define NUM_THREADS 3
#define FILE_TO_COPY_PER_THREAD 3
#define FILE_NAME_MAX_LEN 15

static char *path_src = "tests/file_to_copy_from.txt";

void *copy_from_file(void *args) {
    int file = *((int *)args);

    for (int i = 0; i < FILE_TO_COPY_PER_THREAD; i++) {
        char path[FILE_NAME_MAX_LEN] = {'/'};
        sprintf(path + 1, "%d", file + i);

        assert(tfs_copy_from_external_fs(path_src, path) != -1);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    assert(tfs_init(NULL) != -1);
    int table[NUM_THREADS];

    char *message = "HELLO WORLD!";

    // Create indexes from where the name of the file is going to be called
    for (int i = 0; i < NUM_THREADS; i++) {
        table[i] = i * FILE_TO_COPY_PER_THREAD + 1;
    }

    // Create 3 threads that will, together, create 9 files that
    // copy the content of the file tests/file_to_copy_from.txt
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, copy_from_file, &table[i]) != 0) {
            return -1;
        }
    }

    // Wait for the threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            return -1;
        }
    }

    // Check if the content of the files is correct
    for (int i = 0; i < NUM_THREADS * FILE_TO_COPY_PER_THREAD; i++) {
        char path[FILE_NAME_MAX_LEN] = {'/'};
        sprintf(path + 1, "%d", i + 1);

        int f = tfs_open(path, 0);
        assert(f != -1);

        char buffer[FILE_NAME_MAX_LEN];
        assert(tfs_read(f, buffer, FILE_NAME_MAX_LEN) != -1);
        assert(strncmp(buffer, message, strlen(message)) == 0);

        assert(tfs_close(f) != -1);
    }

    assert(tfs_destroy() != -1);

    PRINT_GREEN("Successful test.\n");
    return 0;
}