//
// Created by marko on 20.4.22..
//

#include "../h/tcb.hpp"
#include "../h/MemoryAllocator.hpp"
#include "../h/workers.hpp"
#include "../h/print.hpp"
#include "../h/riscv.hpp"
#include "../lib/hw.h"

extern void userMain();

void userMainWrapper(void*) {
    userMain();
}


int main()
{
    MemoryAllocator::getInstance().init();
    Riscv::w_stvec((uint64) &Riscv::supervisorTrap);


    thread_t thread1=nullptr;
    thread_t thread2=nullptr;

    thread_create(&thread1,nullptr,nullptr);
    TCB::running = thread1;

    thread_create(&thread2,userMainWrapper,nullptr);

    while (!thread2->isFinished())
    {
        TCB::dispatch();
    }

    return 0;
}
