//
// Created by marko on 20.4.22..
//

#include "../h/tcb.hpp"
#include "../h/riscv.hpp"

TCB *TCB::running = nullptr;

uint64 TCB::timeSliceCounter = 0;


TCB *TCB::createThread(Body start_routine, void *arg, void *stack)
{
    return new TCB(start_routine, arg, stack);
}

void TCB::yield()
{
    register uint64 a0 asm("a0") = 0x13;
    __asm__ volatile("ecall" : "+r"(a0) : : "memory");

    // dispatch();
}

int TCB::threadExit()
{
    TCB *current = running;

    if (current == nullptr || current->isFinished())
    {
        return -1;
    }
    current->setFinished(true);
    TCB::yield();
    return 0;
}

void TCB::dispatch()
{
    TCB *old = running;

    if (!old->isFinished() && !old->getWaiting())
    {
        // printStringMoj("\nPOZVAN DISPATCH\n");
        Scheduler::put(old);
    }
    running = Scheduler::get();
    if (old == running)
    {
        printStringMoj("\nDISPATCH: old == running\n");
    }

    TCB::contextSwitch(&old->context, &running->context);
}

void TCB::threadWrapper()
{
    if (running->body == nullptr)
    {
        Riscv::ms_sstatus(Riscv::SSTATUS_SPP);
    }else
    {
        Riscv::mc_sstatus(Riscv::SSTATUS_SPP);
    }
    Riscv::popSppSpie();
    running->body(running->arg);
    running->setFinished(true);

    TCB::yield();
}
