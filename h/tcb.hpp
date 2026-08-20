//
// Created by marko on 20.4.22..
//

#ifndef OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP
#define OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP

#include "../lib/hw.h"
#include "scheduler.hpp"
#include "syscall_c.h"

// Coroutine Control Block
class TCB
{
public:
    ~TCB() { delete[] stack; }

    bool isFinished() const { return finished; }

    void setFinished(bool value) { finished = value; }

    uint64 getTimeSlice() const
    {
        return timeSlice;
    }

    using Body = void (*)(void*);

    static TCB *createThread(Body start_routine, void *arg, void *stack);

    //static TCB *createThread(Body body);

    static void yield();

    static TCB *running;

    static void dispatch();

    void * arg1() const
    {
        return arg;
    }

    int threadExit();

private:
    explicit TCB(Body body, void *arg, void *stack) : body(body),
                                                      stack((uint64 *) stack),
                                                      context({
                                                          (uint64) &threadWrapper,
                                                          this->stack != nullptr ? (uint64) &this->stack[STACK_SIZE - 1] : 0
                                                      }),
                                                      finished(false),  arg(arg)
    {
        if (body != nullptr) { Scheduler::put(this); }
    }

    struct Context
    {
        uint64 ra;
        uint64 sp;
    };

    Body body;
    uint64 *stack;
    Context context;
    bool finished;
    void *arg;

    static void contextSwitch(Context *oldContext, Context *runningContext);


    friend class Riscv;


    static uint64 constexpr STACK_SIZE = 1024;

    uint64 timeSlice;

    static uint64 constexpr TIME_SLICE = 2;

    static uint64 timeSliceCounter;

    static void threadWrapper();
};

#endif //OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP
