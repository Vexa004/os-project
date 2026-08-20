//
// Created by os on 8/19/26.
//

#include "../h/syscall_c.h"

void *syscall_c::mem_alloc(size_t size)
{
    uint64 code = 0x01;
    void *ret;
    __asm__ volatile(
        "mv a0, %1\n"
        "mv a1, %2\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r" (ret)
        : "r" (code),
          "r" (size)
        : "a0", "a1"
    );
    return ret;
}

int syscall_c::mem_free(void *ptr)
{
    uint64 code = 0x02;
    uint64 ret;
    __asm__ volatile(
        "mv a0, %1\n"
        "mv a1, %2\n"
        "ecall\n"
        "mv %0, a0\n"
        : "=r" (ret)
        : "r" (code),
          "r" (ptr)
        : "a0", "a1"
    );
    return ret;
}
