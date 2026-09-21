//
// Created by marko on 20.4.22..
//

#include "../h/tcb.hpp"
#include "../h/riscv.hpp"
#include "../h/CSemaphore.hpp"

TCB *TCB::running = nullptr;

uint64 TCB::timeSliceCounter = 0;

CSemaphore *TCB::barrierMutex = nullptr;
CSemaphore *TCB::gate = nullptr;
uint64 TCB::barrierCounter = 0;
uint64 TCB::barrierLimit = 0;


TCB::TCB(Body body, void *arg, void *stack) : body(body),
                                              stack((uint64 *) stack),
                                              context({
                                                  (uint64) &threadWrapper,
                                                  this->stack != nullptr
                                                      ? (uint64) &this->stack[STACK_SIZE - 1]
                                                      : 0
                                              }),
                                              finished(false), waiting(false), waitSemErr(false),
                                              arg(arg)
{
    id = running_id++;
    semSend = new CSemaphore();
    semReceive = new CSemaphore(0);
    msg = nullptr;
    if (barrierMutex == nullptr)
        barrierMutex = new CSemaphore();
    if (gate == nullptr)
        gate = new CSemaphore(0);
    pairThread = nullptr;
    synced = new CSemaphore(0);
    syncedBool = false;
    if (body != nullptr)
    {
        Scheduler::put(this);
    }
}

bool TCB::isFinished() const
{
    return finished;
}

void TCB::thread_join(thread_t *handle)
{
    while (!(*handle)->isFinished())
    {
        // printStringMoj("Thread waiting for completion...\n");
        thread_dispatch();
    }
}

uint64 TCB::running_id = -1;

uint64 TCB::getId()
{
    thread_dispatch();
    return id;
}

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
    } else
    {
        Riscv::mc_sstatus(Riscv::SSTATUS_SPP);
    }
    Riscv::popSppSpie();
    running->body(running->arg);
    running->setFinished(true);

    TCB::yield();
}

void TCB::send(char *message)
{
    semSend->wait();
    this->msg = message;
    semReceive->signal();
}


char *TCB::receive()
{
    semReceive->wait();
    char *poruka = this->msg;
    semSend->signal();
    return poruka;
}

void TCB::barrier()
{
    barrierMutex->wait();
    barrierCounter++;

    /*printStringMoj("Broj na barijeri: ");
     printIntegerMoj(barrierCounter);
     printStringMoj("\n");*/

    /*printStringMoj("Limit na barijeri: ");
    printIntegerMoj(barrierLimit);
    printStringMoj("\n");*/

    if (barrierCounter == barrierLimit)
    {
        gate->signalN(barrierCounter);
        barrierCounter = 0;
    }

    barrierMutex->signal();
    gate->wait();
}

void TCB::pair(TCB *other)
{
    if (pairThread != nullptr || other->pairThread != nullptr || this == other)
    {
        return;
    }
    pairThread = other;
    other->pairThread = this;
}


void TCB::sync()
{
    if (pairThread == nullptr)return;
    pairThread->synced->signal();
    synced->wait();
}

/*
void TCB::sync()
{
    if (syncedBool == true) return;
    syncedBool = true;

    while (!pairThread->syncedBool)
    {
        thread_dispatch();
    }
    pairThread->syncedBool = false;
}
*/