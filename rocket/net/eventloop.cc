#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <string.h>
#include "rocket/net/eventloop.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"



#define ADD_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd());        \
    int op = EPOLL_CTL_ADD;                             \
    if(it != m_listen_fds.end()) {                      \
        op = EPOLL_CTL_MOD;                             \
    }                                                   \
    epoll_event tmp = event->getEpollEvent();           \
    INFOLOG("epoll_event.events = %d",(int)tmp.events); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);   \
    if(rt == -1) {                                               \
        ERRORLOG("failed epoll_ctl when add fd %d, errno=%d, error info=%s", event->getFd(), errno, strerror(errno));   \
        exit(0);                                        \
    }                                                   \
    DEBUGLOG("add event success, fd[%d]", event->getFd()); \


#define DEL_TO_EPOLL() \
    auto it = m_listen_fds.find(event->getFd());        \
    int op = EPOLL_CTL_DEL;                             \
    if(it == m_listen_fds.end()) {                      \
        return;                                         \
    }                                                    \
    epoll_event tmp = event->getEpollEvent();           \
    INFOLOG("epoll_event.events = %d",(int)tmp.events); \
    int rt = epoll_ctl(m_epoll_fd, op, event->getFd(), &tmp);   \
    if(rt == -1) {                                               \
        ERRORLOG("failed epoll_ctl when del fd %d, errno=%d, error info=%s", event->getFd(), errno, strerror(errno));   \
        exit(0);                                        \
    }                                                   \
    DEBUGLOG("del event success, fd[%d]", event->getFd()); \

namespace rocket {


//有且只有 thread_local 关键字修饰的变量具有线程（thread）周期，
//这些变量在线程开始的时候被生成，在线程结束的时候被销毁，并且每一个线程都拥有一个独立的变量实例。
static thread_local Eventloop* t_current_eventloop = NULL;
static int g_epoll_max_timeout = 10000;
static int g_epoll_max_events = 10;

Eventloop::Eventloop() {
    if(t_current_eventloop != NULL) {
        ERRORLOG("failed to create event loop, this thread has created event loop");
        exit(0);
    }
    m_thread_id = getThreadId();

    m_epoll_fd = epoll_create(10);

    if(m_epoll_fd == -1) {
        ERRORLOG("failed to create event loop, epoll_wait error, error info[%d]", errno);
        exit(0); 
    }

    initWakeUpFdEvent();
    INFOLOG("successd create event loop in thread %d", m_thread_id);
    t_current_eventloop = this;

}


Eventloop::~Eventloop(){
    close(m_epoll_fd);
    if(m_wakeup_fd_event) {
        delete m_wakeup_fd_event;
        m_wakeup_fd_event = NULL;
    }
}

void Eventloop::initWakeUpFdEvent() { 
    m_wakeup_fd = eventfd(0, EFD_NONBLOCK);
    if(m_wakeup_fd < 0) {
        ERRORLOG("failed to create event loop, eventfd create error, error info[%d]", errno);
        exit(0); 
    }
    m_wakeup_fd_event = new WakeUpFdEvent(m_wakeup_fd);
        
    m_wakeup_fd_event->listen(FdEvent::IN_EVENT, [this]() {     
            char buf[8];
            while(read(m_wakeup_fd, buf, 8)!=-1 && errno != EAGAIN){
            }
            DEBUGLOG("read full bytes from wakeup fd[%d]", m_wakeup_fd);
        
    });

    addEpollEvent(m_wakeup_fd_event);

}

void Eventloop::loop() {
    while(!m_stop_flag) {
        ScopeMutex<Mutex> lock(m_mutex);
        std::queue<std::function<void()>> tmp_tasks;
        m_pending_tasks.swap(tmp_tasks);
        lock.unlock();

        while(!tmp_tasks.empty()) {
            std::function<void()> cb = tmp_tasks.front();
            //tmp_tasks.front()();
            tmp_tasks.pop();
            if(cb) {
                cb();
            }
        }

        int timeout = g_epoll_max_timeout;
        epoll_event result_events[g_epoll_max_events];

        //DEBUGLOG("now begin to epoll_wait");
        int rt = epoll_wait(m_epoll_fd, result_events, g_epoll_max_events, timeout);
        DEBUGLOG("now end epoll_wait, rt = %d", rt);

        if(rt < 0) {
            ERRORLOG("epoll_wait error, errno=", errno);
        }else {
            for(int i=0; i<rt; ++i) {
                epoll_event trigger_event = result_events[i];
                FdEvent* fd_event = static_cast<FdEvent*>(trigger_event.data.ptr);
                if(fd_event == NULL) {
                    continue;
                }
                if(trigger_event.events & EPOLLIN) {

                    DEBUGLOG("fd %d trigger EPOLLIN event", fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::IN_EVENT)); 
                }
                if(trigger_event.events & EPOLLOUT) {
                    DEBUGLOG("fd %d trigger EPOLLOUT event", fd_event->getFd());
                    addTask(fd_event->handler(FdEvent::OUT_EVENT)); 
                }
            }
        }

    }
}

void Eventloop::wakeup() {
    m_wakeup_fd_event->wakeup(); 
}

void Eventloop::dealWakeup() {

}
void Eventloop::stop() {
    m_stop_flag = true;
}

//跨线程修改事件，如主从Reactor中,主线程调用io线程的addEpollEvent,
void Eventloop::addTask(std::function<void()> cb, bool is_wake_up) {
    ScopeMutex<Mutex> lock(m_mutex);
    m_pending_tasks.push(cb);
    lock.unlock();
    if(is_wake_up) {
        wakeup();
    }
}

void Eventloop::addEpollEvent(FdEvent* event) {
    if(isInLoopThread()) {
        ADD_TO_EPOLL();  

    }else{ 
        auto cb = [this, event]() {
            ADD_TO_EPOLL();
        };
        addTask(cb, true);
    }

}

void Eventloop::delEpollEvent(FdEvent* event) {
    if(isInLoopThread()) {
        DEL_TO_EPOLL();
    }else {
        auto cb = [this, event]() {
            DEL_TO_EPOLL();
        };
        addTask(cb, true);
    }
}

//判断当前线程是不是Loop线程
bool Eventloop::isInLoopThread() {
    return getThreadId() == m_thread_id; 
}

}