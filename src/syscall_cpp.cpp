#include "../h/syscall_cpp.hpp"

#include "../h/tcb.hpp"


Thread::Thread(void (*body)(void *), void *arg)
{
    myHandle = nullptr;
    Thread::body = body;
    Thread::arg = arg;
}

Thread::~Thread() = default;

Thread::Thread()
{
    myHandle = nullptr;
    Thread::body = nullptr;
    Thread::arg = nullptr;
}

void Thread::dispatch()
{
    thread_dispatch();
}

int Thread::sleep(time_t t)
{
    return time_sleep(t);
}

void Thread::threadWrapper(void *arg)
{
    if (static_cast<Thread *>(arg)->body)
    {
        static_cast<Thread *>(arg)->body(static_cast<Thread *>(arg)->arg);
    } else
    {
        static_cast<Thread *>(arg)->run();
    }

    thread_exit();
}

int Thread::start()
{
    return thread_create(&myHandle, threadWrapper, this);
}

uint64 Thread::getThreadId()
{
    return myHandle->getId();
}

void Thread::join()
{
    if (myHandle)
    {
        thread_join(&myHandle);
    }
}

void Thread::send(char *message)
{
    ::send(myHandle, message);
}

char *Thread::receive()
{
    return ::receive();
}

void Thread::barrier()
{
    return ::barrier();
}

void Thread::pair(Thread *t1, Thread *t2)
{
    ::pair(t1->myHandle, t2->myHandle);
}

void Thread::sync()
{
    ::sync();
}


Semaphore::Semaphore(unsigned init) : myHandle(nullptr)
{
    sem_open(&myHandle, init);
}

Semaphore::~Semaphore()
{
    sem_close(myHandle);
}

int Semaphore::wait()
{
    return sem_wait(myHandle);
}

int Semaphore::signal()
{
    return sem_signal(myHandle);
}

int Semaphore::signalN(int n)
{
    return sem_signal_n(myHandle, n);
}

int Semaphore::waitN(int n)
{
    return sem_wait_n(myHandle, n);
}

char Console::getc()
{
    return ::getc();
}

void Console::putc(char c)
{
    ::putc(c);
}
