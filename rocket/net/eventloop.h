#ifndef ROCKET_NET_EVENTLOOP_H
#define ROCKET_NET_EVENTLOOP_H
#include <set>
#include <pthread.h>
#include <functional>
#include <queue>

#include "rocket/common/mutex.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/wakeup_fd_event.h"
namespace rocket {
class Eventloop {
public:
    Eventloop();
    ~Eventloop();

    void loop();
    void wakeup();
    void stop();

    void addEpollEvent(FdEvent* event);
    void delEpollEvent(FdEvent* event);

    void addTask(std::function<void()> cb, bool is_wake_up = false);
    bool isInLoopThread();

private:
    void dealWakeup();

    void addWakeUpFd();

    void initWakeUpFdEvent();
private:
    pthread_t m_thread_id{0};
    int m_wakeup_fd{0};
        
    int m_epoll_fd{0};

    WakeUpFdEvent* m_wakeup_fd_event{NULL};

    std::set<int>  m_listen_fds;

    std::queue<std::function< void() >> m_pending_tasks;
    bool m_stop_flag;
    Mutex m_mutex;
};


}


#endif // _EVENTLOOP_H_