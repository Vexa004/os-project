//
// Created by os on 8/22/26.
//

#ifndef PROJECT_BASE_V1_1_CSEMAPHORE_HPP
#define PROJECT_BASE_V1_1_CSEMAPHORE_HPP
#include "list.hpp"
#include "tcb.hpp"


class CSemaphore
{
public:
    CSemaphore(unsigned init = 1) : val(init)
    {
    }

    int wait();

    int signal();

    int waitN(unsigned n);

    int signalN(unsigned n);

    int semClose();

    ~CSemaphore() = default;

protected:
    int block();

    int unblock();

private:
    int val;
    List<TCB> blockedQueue;
};


#endif //PROJECT_BASE_V1_1_CSEMAPHORE_HPP
