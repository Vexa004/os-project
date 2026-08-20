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
    __asm__ volatile ("csrw sepc, ra");
    __asm__ volatile ("sret");
}

void Riscv::handleSupervisorTrap()
{
    uint64 cause = r_scause();
    uint64 code, a1, a2, a3, a4;
    __asm__ volatile ("mv %0, a0" : "=r"(code));
    __asm__ volatile ("mv %0, a1" : "=r"(a1));
    __asm__ volatile ("mv %0, a2" : "=r"(a2));
    __asm__ volatile ("mv %0, a3" : "=r"(a3));
    __asm__ volatile ("mv %0, a4" : "=r"(a4));
    if (cause == 0x0000000000000009UL || cause == 0x0000000000000008UL)
    {
        // interrupt: no, cause code: supervisor environment call from U-mode or S-mode
        printString("1ECALL! code: \n");
        printInteger(code);
        printString("\n2ECALL! code: \n");
        uint64 sepc = r_sepc() + 4;
        uint64 sstatus = r_sstatus();

        w_sstatus(sstatus);
        w_sepc(sepc);
    } else if (cause == 0x8000000000000001UL)
    {
        // interrupt: yes, cause code: supervisor software interrupt (timer)

        TCB::timeSliceCounter++;

        if (TCB::timeSliceCounter >= TCB::running->getTimeSlice())
        {
            //TCB::running->timeSliceCounter = 0;
            uint64 sepc = r_sepc();
            uint64 sstatus = r_sstatus();
            TCB::timeSliceCounter = 0;
            TCB::dispatch();
            w_sstatus(sstatus);
            w_sepc(sepc);
        }
        mc_sip(SIP_SSIP);
    } else if (cause == 0x8000000000000009UL)
    {
        // interrupt: yes, cause code: external interrupt (console)
        console_handler();
    } else
    {
        // interrupt: no, cause code: exception
        uint64 sepc = r_sepc();
        uint64 stval = r_stval();
        printString("Exception! scause: ");
        printInteger(cause);
        printString(", sepc: ");
        printInteger(sepc);
        printString(", stval: ");
        printInteger(stval);
    }
}

void callSystemCall(uint64 code, uint64 a1, uint64 a2, uint64 a3, uint64 a4)
{
    switch (code)
    {
        case 0x01:
            {
                void *ptr = MemoryAllocator::getInstance().mem_alloc(a1);
                break;
            }
        case 0x02:
            {
                int ret = MemoryAllocator::getInstance().mem_free((void *) a1);
                break;
            }
        default:
            {
                printString("Unknown system call: ");
                printInteger(code);
                break;
            }
    }
}
