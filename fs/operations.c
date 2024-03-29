#include "operations.h"
#include "config.h"
#include "state.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "betterassert.h"
#include "pthread.h"

static pthread_mutex_t tfs_open_mutex;

tfs_params tfs_default_params() {
    tfs_params params = {
        .max_inode_count = 64,
        .max_block_count = 1024,
        .max_open_files_count = 16,
        .block_size = 1024,
    };
    return params;
}

int tfs_init(tfs_params const *params_ptr) {
    tfs_params params;

    if (params_ptr != NULL) {
        params = *params_ptr;
    } else {
        params = tfs_default_params();
    }

    if (state_init(params) != 0) {
        return -1;
    }

    pthread_mutex_init(&tfs_open_mutex, NULL);

    // create root inode
    int root = inode_create(T_DIRECTORY);
    if (root != ROOT_DIR_INUM) {
        return -1;
    }

    return 0;
}

int tfs_destroy() {
    if (state_destroy() != 0) {
        return -1;
    }
    pthread_mutex_destroy(&tfs_open_mutex);
    return 0;
}

static bool valid_pathname(char const *name) {
    return name != NULL && strlen(name) > 1 && name[0] == '/';
}

/**
 * Looks for a file.
 *
 * Note: as a simplification, only a plain directory space (root directory only)
 * is supported.
 *
 * Input:
 *   - name: absolute path name
 *   - root_inode: the root directory inode
 * Returns the inumber of the file, -1 if unsuccessful.
 */
static int tfs_lookup(char const *name) {
    if (!valid_pathname(name)) {
        return -1;
    }

    inode_t *root_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_inode != NULL, "tfs_open: root dir inode must exist");

    // skip the initial '/' character
    name++;

    return find_in_dir(root_inode, name);
}

int tfs_open(char const *name, tfs_file_mode_t mode) {
    // Checks if the path name is valid
    if (!valid_pathname(name)) {
        return -1;
    }

    pthread_mutex_lock(&tfs_open_mutex);
    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_open: root dir inode must exist");

    // We need to ensure that while we check is the file exists there isn't
    // another one being created
    int inum = tfs_lookup(name);
    size_t offset;

    if (inum >= 0) {
        pthread_mutex_unlock(&tfs_open_mutex);

        // The file already exists
        inode_t *inode = inode_get(inum);
        ALWAYS_ASSERT(inode != NULL,
                      "tfs_open: directory files must have an inode");

        // if we're opening a soft link
        if (inode->i_node_type == T_LINK) {
            pthread_mutex_lock(&tfs_open_mutex);
            // get the target pahtname to open it
            char *target = (char *)data_block_get(inode->i_data_block);
            ALWAYS_ASSERT(valid_pathname(target),
                          "tfs_open: symlink name must be valid");

            // checks if the file exists
            int target_inum = tfs_lookup(target);
            pthread_mutex_unlock(&tfs_open_mutex);
            if (target_inum == -1) {
                return -1;
            }
            inum = target_inum;
            inode = inode_get(inum);
            ALWAYS_ASSERT(inode != NULL,
                          "tfs_open: directory files must have an inode");
        }

        // Truncate (if requested)
        if (mode & TFS_O_TRUNC) {
            if (inode->i_size > 0) {
                data_block_free(inode->i_data_block);
                inode->i_size = 0;
            }
        }
        // Determine initial offset
        if (mode & TFS_O_APPEND) {
            offset = inode->i_size;
        } else {
            offset = 0;
        }
    } else if (mode & TFS_O_CREAT) {
        // The file does not exist; the mode specified that it should be created
        // Create inode
        inum = inode_create(T_FILE);
        if (inum == -1) {
            pthread_mutex_unlock(&tfs_open_mutex);
            return -1; // no space in inode table
        }

        // Add entry in the root directory
        if (add_dir_entry(root_dir_inode, name + 1, inum) == -1) {
            inode_delete(inum);
            pthread_mutex_unlock(&tfs_open_mutex);
            return -1; // no space in directory
        }

        pthread_mutex_unlock(&tfs_open_mutex);
        offset = 0;
    } else {
        pthread_mutex_unlock(&tfs_open_mutex);
        return -1;
    }

    // Finally, add entry to the open file table and return the corresponding
    // handle
    return add_to_open_file_table(inum, offset);

    // Note: for simplification, if file was created with TFS_O_CREAT and there
    // is an error adding an entry to the open file table, the file is not
    // opened but it remains created
}

int tfs_sym_link(char const *target, char const *link_name) {
    // Checks if the path names are valid
    if (!valid_pathname(link_name) && !valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_sym_link: root inode must exist");

    // Creates the inode for the soft link
    int link_inum = inode_create(T_LINK);
    if (link_inum == -1) {
        return -1;
    }

    inode_t *link_inode = inode_get(link_inum);
    ALWAYS_ASSERT(link_inode != NULL,
                  "tfs_sym_link: inode of open file deleted");

    // Allocates a data block and assigns it to the inode
    int bnum = data_block_alloc();
    if (bnum == -1) {
        return -1; // no space
    }

    link_inode->i_data_block = bnum;

    void *block = data_block_get(link_inode->i_data_block);
    ALWAYS_ASSERT(block != NULL, "tfs_sym_link: data block deleted mid-write");

    // Copies hte target pathname to the actal data block
    memcpy(block, target, strlen(target) + 1);

    add_dir_entry(root_dir_inode, link_name + 1, link_inum);
    return 0;
}

int tfs_link(char const *target, char const *link_name) {
    // Checks if the path names are valid
    if (!valid_pathname(link_name) || !valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_link: root dir inode must exist");
    int target_inum = tfs_lookup(target);
    // Checks if the target file exists
    if (target_inum < 0) {
        return -1;
    }

    inode_t *target_inode = inode_get(target_inum);
    ALWAYS_ASSERT(target_inode != NULL,
                  "tfs_link: target file must have an inode");

    if (target_inode->i_node_type == T_LINK) {
        return -1;
    }

    // Checks if the link file already exists
    int link_inum = tfs_lookup(link_name);
    if (link_inum >= 0) {
        return -1;
    }

    add_dir_entry(root_dir_inode, link_name + 1, target_inum);
    target_inode->i_link_count++;
    return 0;
}

int tfs_close(int fhandle) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1; // invalid fd
    }

    remove_from_open_file_table(fhandle);

    return 0;
}

ssize_t tfs_write(int fhandle, void const *buffer, size_t to_write) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }
    pthread_mutex_lock(&tfs_open_mutex);

    //  From the open file table entry, we get the inode
    inode_t *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_write: inode of open file deleted");

    // Determine how many bytes to write
    size_t block_size = state_block_size();
    if (to_write + file->of_offset > block_size) {
        to_write = block_size - file->of_offset;
    }

    if (to_write > 0) {
        if (inode->i_size == 0) {
            // If empty file, allocate new block
            int bnum = data_block_alloc();
            if (bnum == -1) {
                pthread_mutex_unlock(&tfs_open_mutex);
                return -1; // no space
            }

            inode->i_data_block = bnum;
        }

        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_write: data block deleted mid-write");

        // Perform the actual write
        memcpy(block + file->of_offset, buffer, to_write);

        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_write;
        if (file->of_offset > inode->i_size) {
            inode->i_size = file->of_offset;
        }
    }
    pthread_mutex_unlock(&tfs_open_mutex);
    return (ssize_t)to_write;
}

ssize_t tfs_read(int fhandle, void *buffer, size_t len) {
    open_file_entry_t *file = get_open_file_entry(fhandle);
    if (file == NULL) {
        return -1;
    }

    pthread_mutex_lock(&tfs_open_mutex);
    // From the open file table entry, we get the inode
    inode_t const *inode = inode_get(file->of_inumber);
    ALWAYS_ASSERT(inode != NULL, "tfs_read: inode of open file deleted");

    // Determine how many bytes to read
    size_t to_read = inode->i_size - file->of_offset;
    if (to_read > len) {
        to_read = len;
    }

    if (to_read > 0) {
        void *block = data_block_get(inode->i_data_block);
        ALWAYS_ASSERT(block != NULL, "tfs_read: data block deleted mid-read");

        // Perform the actual read
        memcpy(buffer, block + file->of_offset, to_read);
        // The offset associated with the file handle is incremented accordingly
        file->of_offset += to_read;
    }

    pthread_mutex_unlock(&tfs_open_mutex);

    return (ssize_t)to_read;
}

int tfs_unlink(char const *target) {
    // Checks if the path name is valid
    if (!valid_pathname(target)) {
        return -1;
    }

    inode_t *root_dir_inode = inode_get(ROOT_DIR_INUM);
    ALWAYS_ASSERT(root_dir_inode != NULL,
                  "tfs_unlink: root dir inode must exist");
    int target_inum = tfs_lookup(target);

    // Checks if the target file exists
    if (target_inum < 0) {
        return -1;
    }

    if (inumber_is_open(target_inum)) {
        return -1;
    }

    inode_t *target_inode = inode_get(target_inum);
    ALWAYS_ASSERT(target_inode != NULL,
                  "tfs_unlink: target file must have an inode");

    // unlink the file
    if (target_inode->i_link_count >= 1) {
        target_inode->i_link_count--;
        clear_dir_entry(root_dir_inode, target + 1);
    }

    // delete the file if it is not linked to any other file
    if (target_inode->i_link_count == 0) {
        if (target_inode->i_data_block != -1)
            data_block_free(target_inode->i_data_block);
        inode_delete(target_inum);
    }

    return 0;
}

int tfs_copy_from_external_fs(char const *source_path, char const *dest_path) {
    // Checks if the source path name is valid
    FILE *src_file = fopen(source_path, "r");
    if (src_file == NULL) {
        return -1;
    }

    // Checks if the destination path name is valid
    int dest_file = tfs_open(dest_path, TFS_O_CREAT | TFS_O_TRUNC);
    if (dest_file == -1) {
        return -1;
    }

    char buffer[state_block_size()];
    size_t read_bytes;

    // Read from the source file and write to the destination file
    while ((read_bytes = fread(buffer, 1, state_block_size(), src_file)) > 0) {
        if (tfs_write(dest_file, buffer, read_bytes) != read_bytes) {
            fclose(src_file);
            tfs_close(dest_file);
            return -1;
        }
    }

    // Close the source file and the destination file
    if (fclose(src_file) == EOF) {
        tfs_close(dest_file);
        return -1;
    }
    if (tfs_close(dest_file) == -1) {
        return -1;
    }
    return 0;
}
