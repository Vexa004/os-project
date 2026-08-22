#include "../h/syscall_cpp.hpp"


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

char Console::getc()
{
    return ::getc();
}

void Console::putc(char c)
{
    ::putc(c);
}
