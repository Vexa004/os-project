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

    if (static_cast<Thread*>(arg)->body)
    {
        static_cast<Thread*>(arg)->body(static_cast<Thread*>(arg)->arg);
    }else
    {
        static_cast<Thread*>(arg)->run();
    }

    thread_exit();
}

int Thread::start()
{
    return thread_create(&myHandle, threadWrapper, this);
}
