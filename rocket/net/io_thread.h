#if !defined(ROCKET_NET_IOTHREAD_H)
#define ROCKET_NET_IOTHREAD_H

#include <pthread.h>
#include <semaphore.h>
#include "rocket/net/eventloop.h"

namespace rocket {

class IOThread {
public:
    IOThread();

    ~IOThread();

    Eventloop* getEventLoop();

    void start();

    void join();

    static void* Main(void *args);

private:
    pid_t m_thread_id {-1}; //线程号
    pthread_t m_thread {0}; //线程句柄

    Eventloop* m_event_loop{NULL}; //当前io线程的Loop对象

    sem_t m_init_semaphore;

    sem_t m_start_semaphore;
};

}

#endif // ROCKET_NET_IOTHREAD_H
