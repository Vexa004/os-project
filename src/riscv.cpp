//
// Created by marko on 20.4.22..
//

#include "../h/riscv.hpp"

#include "../h/MemoryAllocator.hpp"
#include "../h/print.hpp"
#include "../h/tcb.hpp"
#include "../lib/console.h"

void Riscv::popSppSpie()
{
    Riscv::mc_sstatus(Riscv::SSTATUS_SPP);
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
        case 0x01:
            {
                /*printStringMoj("HOCU SIZE2: \n");
                printIntegerMoj((uint64)a1);
                printStringMoj("\n");*/

                void *ptr = MemoryAllocator::getInstance().mem_alloc(a1);
                return (uint64) ptr;
                break;
            }
        case 0x02:
            {
                int ret = MemoryAllocator::getInstance().mem_free((void *) a1);
                return ret;
                break;
            }
        case 0x11:
            {
                thread_t *thr = (thread_t *) a1;
                TCB::Body body = (TCB::Body) a2;
                TCB *thread = TCB::createThread(body, (void *) a3, (void *) a4);

                if (thread == nullptr)
                {
                    return -1;
                }else
                {
                    *thr = thread;
                    return 0;
                }
                break;
            }
        case 0x12:
            {
                TCB::threadExit();
                break;
            }
        case 0x13:
            {
                TCB::dispatch();
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
