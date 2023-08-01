#include <assert.h>
#include "rocket/net/io_thread.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

IOThread::IOThread() {
    int rt = sem_init(&m_init_semaphore, 0, 0);
    assert(rt == 0);

    rt = sem_init(&m_start_semaphore, 0, 0);    //pshared=0表示在一个进程的多个线程中共享信号量
    assert(rt == 0);

    pthread_create(&m_thread, NULL, &IOThread::Main, this);

    //wait,直到新线程执行完Main函数的前置
    sem_wait(&m_init_semaphore);

    DEBUGLOG("IOThread [%d] create success", m_thread_id);
}

IOThread::~IOThread() {

  m_event_loop->stop();
  sem_destroy(&m_init_semaphore);
  sem_destroy(&m_start_semaphore);

  pthread_join(m_thread, NULL); //主线程等IO线程结束

  if (m_event_loop) {
    delete m_event_loop;
    m_event_loop = NULL;
  }
}

void* IOThread::Main(void* args) {
    IOThread* thread = static_cast<IOThread*> (args);

    thread->m_event_loop = new Eventloop();
    thread->m_thread_id = getThreadId();

    //唤醒等待的线程
    sem_post(&thread->m_init_semaphore);

    //让IO线程等待，直到我们主动启动
    DEBUGLOG("IOThread %d created, wait start semaphore", thread->m_thread_id);

    sem_wait(&thread->m_start_semaphore);

    DEBUGLOG("IOThread %d start loop ",thread->m_thread_id);
    thread->m_event_loop->loop();

    DEBUGLOG("IOThread %d end loop ",thread->m_thread_id);

    return NULL;

}

Eventloop* IOThread::getEventLoop() {
  return m_event_loop;
}

void IOThread::start() {
  DEBUGLOG("Now invoke IOThread %d", m_thread_id);
  sem_post(&m_start_semaphore);
}


void IOThread::join() {
  pthread_join(m_thread, NULL);
}
}
