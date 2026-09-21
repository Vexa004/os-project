//
// Created by os on 8/22/26.
//

#include "../h/CSemaphore.hpp"

#include "../h/syscall_cpp.hpp"

int CSemaphore::block()
{
    if (TCB::running)
    {
        TCB::running->setWaiting(true);
        blockedQueue.addLast(TCB::running);
        // printStringMoj("Thread blocked on semaphore\n");
        thread_dispatch();
        if (TCB::running->getWaitSemErr())
        {
            return -2;
        }
        return 0;
    }
    return -1;
}

int CSemaphore::unblock()
{
    TCB *t = blockedQueue.removeFirst();
    if (t)
    {
        t->setWaiting(false);
        Scheduler::put(t);
        return 0;
    }
    return -1;
}

int CSemaphore::wait()
{
    if (--val < 0) return block();
    return 0;
}

int CSemaphore::signal()
{
    if (++val <= 0) return unblock();
    return 0;
}

int CSemaphore::semClose()
{
    TCB *t = blockedQueue.removeFirst();
    while (t)
    {
        t->setWaiting(false);
        t->setWaitSemErr(true);
        Scheduler::put(t);
        t = blockedQueue.removeFirst();
    }
    return 0;
}

int CSemaphore::signalN(unsigned n)
{
    for (unsigned i = 0; i < n; i++)
    {
        signal();
    }
    return 0;
}

int CSemaphore::waitN(unsigned n)
{
    for (int i = 0; i < (int)n; i++) wait();
    return 0;
}
