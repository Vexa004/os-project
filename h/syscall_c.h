//
// Created by os on 8/19/26.
//

#ifndef PROJECT_BASE_V1_1_SYSCALL_C_HPP
#define PROJECT_BASE_V1_1_SYSCALL_C_HPP

#include "../lib/hw.h"

// *Memory*

void *mem_alloc(size_t size);

int mem_free(void *);

// *Threads*

class TCB;
typedef TCB *thread_t;

int thread_create(thread_t *handle, void (*start_routine)(void *), void *arg);

int thread_exit();

void thread_dispatch();

// *Semaphore*

class _sem;
typedef _sem *sem_t;

int sem_open(
    sem_t *handle,
    unsigned init
);

int sem_close(sem_t handle);

int sem_wait(sem_t id);

int sem_signal(sem_t id);

int sem_wait_n(sem_t id,
               unsigned n);

int sem_signal_n(sem_t id,
unsigned n);


// *Time*

typedef unsigned long time_t;

int time_sleep(time_t);

// *Console*

const int EOF = -1;

char getc();

void putc(char);


#endif //PROJECT_BASE_V1_1_SYSCALL_C_HPP
