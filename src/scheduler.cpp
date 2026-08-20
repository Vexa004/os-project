//
// Created by marko on 20.4.22..
//

#include "../h/scheduler.hpp"

List<TCB> Scheduler::readyCoroutineQueue;

TCB *Scheduler::get()
{
    return readyCoroutineQueue.removeFirst();
}

void Scheduler::put(TCB *ccb)
{
    readyCoroutineQueue.addLast(ccb);
}