//
// Created by marko on 20.4.22..
//

#include "../h/riscv.hpp"

#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"
#include "../h/tcb.hpp"
#include "../lib/console.h"
#include "../h/CSemaphore.hpp"

void Riscv::popSppSpie()
{
    __asm__ volatile ("csrw sepc, ra");
    __asm__ volatile ("sret");
}

void Riscv::handleSupervisorTrap(uint64 *regs)
{
    uint64 broj, ar1, ar2, ar3, ar4;

    broj = regs[10];
    ar1 = regs[11];
    ar2 = regs[12];
    ar3 = regs[13];
    ar4 = regs[14];

    uint64 cause = r_scause();

    if (cause == 0x0000000000000009UL || cause == 0x0000000000000008UL)
    {
        // interrupt: no, cause code: supervisor environment call from U-mode or S-mode

        uint64 sepc = r_sepc() + 4;
        uint64 sstatus = r_sstatus();


        regs[10] = handleEnvironmentCall(broj, ar1, ar2, ar3, ar4);

        w_sstatus(sstatus);
        w_sepc(sepc);
    } else if (cause == 0x8000000000000001UL)
    {
        // interrupt: yes, cause code: supervisor software interrupt (timer)

        //TCB::running->timeSliceCounter = 0;
        mc_sip(SIP_SSIP);
        uint64 sepc = r_sepc();
        uint64 sstatus = r_sstatus();
        TCB::dispatch();
        w_sstatus(sstatus);
        w_sepc(sepc);
    } else if (cause == 0x8000000000000009UL)
    {
        // interrupt: yes, cause code: external interrupt (console)
        console_handler();
    } else
    {
        // interrupt: no, cause code: exception
        uint64 sepc = r_sepc();
        uint64 stval = r_stval();
        printStringMoj("Exception! scause: ");
        printIntegerMoj(cause);
        printStringMoj(", sepc: ");
        printIntegerMoj(sepc);
        printStringMoj(", stval: ");
        printIntegerMoj(stval);
        while (1);
    }
}

uint64 Riscv::handleEnvironmentCall(uint64 code, uint64 a1, uint64 a2, uint64 a3, uint64 a4)
{
    // uint64 retVal;
    switch (code)
    {
        case 0x01: // mem_alloc
            {
                /*printStringMoj("HOCU SIZE2: \n");
                printIntegerMoj((uint64)a1);
                printStringMoj("\n");*/

                void *ptr = MemoryAllocator::getInstance().mem_alloc(a1);
                return (uint64) ptr;
                break;
            }
        case 0x02: // mem_free
            {
                int ret = MemoryAllocator::getInstance().mem_free((void *) a1);
                return ret;
                break;
            }
        case 0x11: // thread_create
            {
                thread_t *thr = (thread_t *) a1;
                TCB::Body body = (TCB::Body) a2;
                TCB *thread = TCB::createThread(body, (void *) a3, (void *) a4);

                if (thread == nullptr)
                {
                    return -1;
                } else
                {
                    *thr = thread;
                    return 0;
                }
                break;
            }
        case 0x12: // thread_exit
            {
                TCB::threadExit();
                break;
                return 0;
            }
        case 0x13: // thread_dispatch
            {
                TCB::dispatch();
                break;
                return 0;
            }
        case 0x14: // get_thread_id
            {
                return TCB::running->getId();
                break;
            }
        case 0x15: // thread_join
            {
                thread_t *thr = (thread_t *) a1;
                if (thr == nullptr)
                {
                    return -1;
                }
                TCB::thread_join(thr);
                return 0;
                break;
            }

        case 0x16: // thread send
            {
                thread_t thr = (thread_t) a1;
                char *message = (char *) a2;
                if (thr == nullptr)
                {
                    return -1;
                }
                thr->send(message);
                return 0;
                break;
            }
        case 0x17: // thread receive
            {
                char *message = TCB::running->receive();
                return (uint64) message;
                break;
            }
        case 0x18: // thread barrier
            {
                TCB::barrier();
                break;
            }
        case 0x19: // barrier Limit
            {
                auto limit = a1;

                TCB::setLimitBarrier(limit);
                break;
            }
        case 0x30: // thread pair
            {
                auto thr1 = (thread_t) a1;
                auto thr2 = (thread_t) a2;
                thr1->pair(thr2);
                break;
            }
        case 0x31: // thread pair sync
            {
                TCB::running->sync();
                break;
            }
        case 0x32: // thread pair sem
            {
                auto *thr1 = (thread_t) a1;
                auto *thr2 = (thread_t) a2;
                thr1->upari(thr2);
                break;
            }
        case 0x33: // thread blokiraj Upareni semafori
            {
                TCB::running->blokiraj();
                break;
            }
        case 0x34: // signal thread uparen Semafor
            {
                auto *thr = (thread_t) a1;
                thr->uparenSemafor->signal();
                break;
            }

        case 0x21: // sem_open
            {
                sem_t *sem = (sem_t *) a1;
                unsigned init = (unsigned) a2;
                CSemaphore *csem = new CSemaphore(init);
                if (csem == nullptr)
                {
                    return -1;
                } else
                {
                    *sem = csem;
                    return 0;
                }
                break;
            }
        case 0x22: // sem_close
            {
                sem_t csem = (sem_t) a1;
                return csem->semClose();
                //delete csem;
                return 0;
                break;
            }
        case 0x23: // sem_wait
            {
                sem_t csem = (sem_t) a1;
                if (csem == nullptr)
                {
                    return -1;
                }
                return csem->wait();
                break;
            }
        case 0x24: // sem_signal
            {
                sem_t csem = (sem_t) a1;
                if (csem == nullptr)
                {
                    return -1;
                }
                return csem->signal();
                break;
            }
        case 0x25: // sem_wait_n
            {
                CSemaphore *csem = (CSemaphore *) a1;
                int n = (int) a2;
                if (csem == nullptr || n == 0)
                {
                    return -1;
                }
                return csem->waitN(n);
                break;
            }
        case 0x26: // sem_signal_n
            {
                CSemaphore *csem = (CSemaphore *) a1;
                unsigned n = (unsigned) a2;
                if (csem == nullptr || n == 0)
                {
                    return -1;
                }
                return csem->signalN(n);
                break;
            }
        case 0x41: // GetC
            {
                return __getc();
                break;
            }
        case 0x42: // PutC
            {
                __putc(a1);
                return 0;
                break;
            }
        default:
            {
                printStringMoj("Unknown system call: ");
                printIntegerMoj(code);
                break;
            }
    }
    return -1;
}
