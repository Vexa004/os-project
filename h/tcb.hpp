//
// Created by marko on 20.4.22..
//

#ifndef OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP
#define OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP

#include "print.hpp"
#include "riscv.hpp"
#include "../lib/hw.h"
#include "scheduler.hpp"
#include "syscall_c.h"

class CSemaphore;

// Coroutine Control Block
class TCB
{
public:
    ~TCB()
    {
        delete[] stack;
    }

    bool isFinished() const;

    void setFinished(bool value) { finished = value; }

    static uint64 running_id;


    uint64 getTimeSlice() const
    {
        return timeSlice;
    }

    using Body = void (*)(void *);

    static TCB *createThread(Body start_routine, void *arg, void *stack);

    //static TCB *createThread(Body body);

    static void yield();

    static TCB *running;

    static void dispatch();

    void *arg1() const
    {
        return arg;
    }

    bool getWaiting() const
    {
        return waiting;
    }

    bool getWaitSemErr()
    {
        return waitSemErr;
    }

    void setWaitSemErr(bool v)
    {
        this->waitSemErr = v;
    }


    void setWaiting(bool waiting)
    {
        this->waiting = waiting;
    }

    static int threadExit();

    uint64 getId();

    typedef TCB *thread_t;

    void send(char *message);

    char *receive();

    static void setLimitBarrier(uint64 limit)
    {
        barrierLimit = limit;
    }

    static void barrier();

    void pair(TCB *other);

    void sync();

    void upari(TCB* other);

    void blokiraj();

private:
    char *msg;

    bool syncedBool;

    CSemaphore *uparenSemafor;

    TCB **upareniThreadovi;

    uint64 brojUparenih;

    uint64 kapacitet;

    CSemaphore *synced;

    TCB *pairThread;

    static CSemaphore *barrierMutex;

    static CSemaphore *gate;

    static uint64 barrierCounter;

    static uint64 barrierLimit;

    static void thread_join(thread_t *handle);

    uint64 id;

    CSemaphore *semSend, *semReceive;

    explicit TCB(Body body, void *arg, void *stack);

    struct Context
    {
        uint64 ra, sp;
        uint64 s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    };

    Body body;
    uint64 *stack;
    Context context;
    bool finished;
    bool waiting;
    bool waitSemErr;
    void *arg;

    static void contextSwitch(Context *oldContext, Context *runningContext);


    friend class Riscv;


    static uint64 constexpr STACK_SIZE = DEFAULT_STACK_SIZE / sizeof(uint64);

    uint64 timeSlice;

    static uint64 constexpr TIME_SLICE = 2;

    static uint64 timeSliceCounter;

    static void threadWrapper();
};

#endif //OS1_VEZBE07_RISCV_CONTEXT_SWITCH_1_SYNCHRONOUS_CCB_HPP
