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

int get_thread_id();

void thread_join(thread_t *handle);

void send(thread_t handle, char *message);

char *receive();

void barrier();

void setBarrierLimit(int limit);

void pair(thread_t handle1, thread_t handle2);

void sync();

void upari(thread_t t1, thread_t t2);

void blokiraj();

void signalUparenSemafor(thread_t);

// *Semaphore*

class CSemaphore;
typedef CSemaphore *sem_t;

int sem_open(
    sem_t *handle,
    unsigned init
);

int sem_close(sem_t handle);

int sem_wait(sem_t id);

int sem_signal(sem_t id);

int sem_wait_n(sem_t id,
               int n);

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
