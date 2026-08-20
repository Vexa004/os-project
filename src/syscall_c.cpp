//
// Created by os on 8/19/26.
//

#include "../h/syscall_c.h"

#include "../h/print.hpp"


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


int thread_create(thread_t *handle, void (*start_routine)(void *), void *arg)
{
    uint64 broj = 0x11;
    void *stack = mem_alloc(broj);
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

    return (int)a0;
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
    return (int)a0;
}
