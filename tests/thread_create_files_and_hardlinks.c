#include "../fs/operations.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "prettyprint.h"

#define NUM_THREADS 2
#define NUM_OF_LINKS 10
#define FILE_TO_CREATE_PER_THREAD 5
#define FILE_NAME_MAX_LEN 10

void *create_file(void* args) {
    int file = *((int *)args);

    // Create 5 files and 5 hard links to them
    for (int i = 0; i < FILE_TO_CREATE_PER_THREAD; i++) {
        char path[FILE_NAME_MAX_LEN] = {'/'};
        sprintf(path + 1, "%d", file + i);
        
        // Create the file
        int f = tfs_open(path, TFS_O_CREAT);
        assert(f != -1);

        // Write the path name to the file
        assert(tfs_write(f, path, strlen(path) + 1) == strlen(path) + 1);

        assert(tfs_close(f) != -1);

        // Create the hard link patb
        char link_path[FILE_NAME_MAX_LEN] = "/l";
        sprintf(link_path + 2, "%d", file + i);
        
        // Create the hard link to the file
        assert(tfs_link(path, link_path) != -1);
    }

    return NULL;
}

int main() {
    pthread_t threads[NUM_THREADS];
    assert(tfs_init(NULL) != -1);
    int table[NUM_THREADS];

    // Create indexes from where the name of the file is going to be called
    for (int i = 0; i < NUM_THREADS; i++) {
        table[i] = i * FILE_TO_CREATE_PER_THREAD +1;
    }

    // Create 2 threads that will, together, create 10 files
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_create(&threads[i], NULL, create_file, &table[i]) != 0) {
            return -1;
        }
    }

    // Wait for the threads to finish
    for (int i = 0; i < NUM_THREADS; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            return -1;
        }
    }
    
    // Open the files and check if the content is correct
    for (int i = 0; i < NUM_THREADS * FILE_TO_CREATE_PER_THREAD; i++) {
        char path[FILE_NAME_MAX_LEN] = {'/'};
        sprintf(path + 1, "%d", i + 1);
        char link_path[FILE_NAME_MAX_LEN] = "/l";
        sprintf(link_path + 2, "%d", i + 1);

        int f = tfs_open(link_path, 0);
        assert(f != -1);

        char buffer[FILE_NAME_MAX_LEN];
        // Read the file content
        assert(tfs_read(f, buffer, FILE_NAME_MAX_LEN) != -1);

        // Compares the content with the path
        assert(strcmp(path, buffer) == 0);

        assert(tfs_close(f) != -1);
    }

    tfs_destroy();

    PRINT_GREEN("Successful test.\n");
}