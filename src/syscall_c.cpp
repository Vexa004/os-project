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

    return (void *)a0;
}

int mem_free(void *ptr)
{
    uint64 broj = 0x02;
    uint64 ret;

    __asm__ volatile ("mv a0, %0" : : "r"(broj));
    __asm__ volatile ("mv a1, %0" : : "r"(ptr));
    __asm__ volatile ("ecall");
    __asm__ volatile ("mv %0, a0" : "=r"(ret));

    return ret;
}


int thread_create(thread_t *handle, void * (*start_routine)(void *), void *arg)
{
    //uint64 broj = 0x10;
    //void* stack  = mem_alloc(broj);
    return 1;
}
