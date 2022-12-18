#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <utils.h>
#include "betterassert.h"

void rwlock_writelock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_wrlock(lock) != 0) {
        perror("Failed to lock RWlock");
        exit(EXIT_FAILURE);
    }
}

void rwlock_readlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_rdlock(lock) != 0) {
        perror("Failed to lock RWlock");
        exit(EXIT_FAILURE);
    }
}

void rwlock_unlock(pthread_rwlock_t *lock) {
    if (pthread_rwlock_unlock(lock) != 0) {
        perror("Failed to unlock RWlock");
        exit(EXIT_FAILURE);
    }
}

void rwlock_init(pthread_rwlock_t *lock) {
    if (pthread_rwlock_init(lock, NULL) != 0) {
        perror("Failed to initialize RWlock");
        exit(EXIT_FAILURE);
    }
}

void rwlock_destroy(pthread_rwlock_t *lock) {
    if (pthread_rwlock_destroy(lock) != 0) {
        perror("Failed to destroy RWlock");
        exit(EXIT_FAILURE);
    }
}

void mutex_lock(pthread_mutex_t *lock) {
    if (pthread_mutex_lock(lock) != 0) {
        perror("Failed to lock mutex");
        exit(EXIT_FAILURE);
    }
}

void mutex_unlock(pthread_mutex_t *lock) {
    if (pthread_mutex_unlock(lock) != 0) {
        perror("Failed to unlock mutex");
        exit(EXIT_FAILURE);
    }
}

void mutex_init(pthread_mutex_t *lock) {
    if (pthread_mutex_init(lock, NULL) != 0) {
        perror("Failed to initialize mutex");
        exit(EXIT_FAILURE);
    }
}

void mutex_destroy(pthread_mutex_t *lock) {
    if (pthread_mutex_destroy(lock) != 0) {
        perror("Failed to destroy mutex");
        exit(EXIT_FAILURE);
    }
}