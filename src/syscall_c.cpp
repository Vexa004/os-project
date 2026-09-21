//
// Created by os on 8/19/26.
//

#include "../h/syscall_c.h"
#include "../h/print.hpp"
#include "../h/tcb.hpp"
#include "../lib/console.h"

//region Memory

void *mem_alloc(size_t size)
{
    uint64 broj = 0x01;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = size;

    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");

    return (void *) a0;
}

int mem_free(void *ptr)
{
    uint64 broj = 0x02;

    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) ptr;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");
    return (int) a0;
}

//endregion

//region Thread

int thread_create(thread_t *handle, void (*start_routine)(void *), void *arg)
{
    uint64 broj = 0x11;
    void *stack = mem_alloc(DEFAULT_STACK_SIZE);
    if (!stack || stack == nullptr || stack == 0 || !handle)
    {
        return -1;
    }
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) handle;
    register uint64 a2 asm("a2") = (uint64) start_routine;
    register uint64 a3 asm("a3") = (uint64) arg;
    register uint64 a4 asm("a4") = (uint64) stack;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2), "r"(a3),"r"(a4)
        : "memory");

    return (int) a0;
}

void thread_dispatch()
{
    uint64 broj = 0x13;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        : "+r"(a0)
        :
        : "memory");
}

int thread_exit()
{
    uint64 broj = 0x12;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        : "+r"(a0)
        :
        : "memory");
    return (int) a0;
}

int get_thread_id()
{
    uint64 broj = 0x14;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :
        :"memory");
    return (int) a0;
}

void thread_join(thread_t *handle)
{
    uint64 broj = 0x15;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) handle;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1)
        :"memory"
    );
}

void send(thread_t handle, char *message)
{
    uint64 broj = 0x16;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) handle;
    register char *a2 asm("a2") = message;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1), "r"(a2)
        :"memory"
    );
}

char *receive()
{
    uint64 broj = 0x17;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :
        :"memory"
    );
    return reinterpret_cast<char *>(a0);
}

void barrier()
{
    uint64 broj = 0x18;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :
        :"memory"
    );
}

void setBarrierLimit(int limit)
{
    uint64 broj = 0x19;

    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) limit;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1)
        :"memory"
    );
}

void pair(thread_t handle1, thread_t handle2)
{
    uint64 broj = 0x30;
    register uint64 a0 asm("a0") = broj;
    register auto a1 asm("a1") = (uint64) handle1;
    register auto a2 asm("a2") = (uint64) handle2;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1),"r"(a2)
        :"memory"
    );
}

void sync()
{
    uint64 broj = 0x31;

    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :
        :"memory"
    );
}

void upari(thread_t handle1, thread_t handle2)
{
    uint64 broj = 0x32;
    register uint64 a0 asm("a0") = broj;
    register auto a1 asm("a1") = (uint64) handle1;
    register auto a2 asm("a2") = (uint64) handle2;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1),"r"(a2)
        :"memory"
    );
}

void blokiraj()
{
    uint64 broj = 0x33;

    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :
        :"memory"
    );
}

void signalUparenSemafor(thread_t handle)
{
    uint64 broj = 0x34;

    register uint64 a0 asm("a0") = broj;
    register auto a1 asm("a1") = (uint64) handle;
    __asm__ volatile("ecall"
        :"+r"(a0)
        :"r"(a1)
        :"memory"
    );
}

//endregion

//region Semaphore

int sem_open(sem_t *handle, unsigned init)
{
    uint64 broj = 0x21;
    if (!handle || init < 0)
    {
        return -1;
    }
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) handle;
    register uint64 a2 asm("a2") = (uint64) init;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2)
        : "memory");
    return (int) a0;
}

int sem_close(sem_t handle)
{
    uint64 broj = 0x22;
    if (!handle)
    {
        return -1;
    }
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) handle;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");
    return (int) a0;
}

int sem_wait(sem_t id)
{
    uint64 broj = 0x23;
    if (!id)
    {
        return -1;
    }
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) id;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");
    return (int) a0;
}

int sem_signal(sem_t id)
{
    uint64 broj = 0x24;
    if (!id)
    {
        return -1;
    }
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) id;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");
    return (int) a0;
}

int sem_wait_n(sem_t id, int n)
{
    if (!id || n == 0)
        return -1;
    uint64 broj = 0x25;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) id;
    register uint64 a2 asm("a2") = (uint64) n;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2)
        : "memory");
    return (int) a0;
}

int sem_signal_n(sem_t id, unsigned n)
{
    if (!id || n == 0)
        return -1;
    uint64 broj = 0x26;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = (uint64) id;
    register uint64 a2 asm("a2") = (uint64) n;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1), "r"(a2)
        : "memory");
    return (int) a0;
}

//endregion

//region Time

int time_sleep(time_t)
{
    return 0;
}

//endregion

//region Console

char getc()
{
    uint64 broj = 0x41;
    register uint64 a0 asm("a0") = broj;
    __asm__ volatile("ecall"
        : "+r"(a0)
        :
        : "memory");
    return (char) a0;
}

void putc(char c)
{
    uint64 broj = 0x42;
    register uint64 a0 asm("a0") = broj;
    register uint64 a1 asm("a1") = c;
    __asm__ volatile("ecall"
        : "+r"(a0)
        : "r"(a1)
        : "memory");
}

//endregion
