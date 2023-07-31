#include <sys/timerfd.h>
#include <string.h>
#include "rocket/net/timer.h"
#include "rocket/common/log.h"
#include "rocket/common/util.h"

namespace rocket {

Timer::Timer():FdEvent() {
    m_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_NONBLOCK);
    DEBUGLOG("timer fd=%d", m_fd);


    //把fd可读事件放到eventloop监听
    listen(FdEvent::IN_EVENT, std::bind(&Timer::onTimer, this));

}

Timer::~Timer() {

}

void Timer::onTimer() {
    char buf[8];
    while(1) {
        if((read(m_fd, buf, 8)== -1) && errno==EAGAIN){
            break;
        }
    }

    int64_t now = getNowMs();

    std::vector<TimerEvent::s_ptr> tmps;
    std::vector<std::pair<int64_t, std::function<void()>>> tasks;

    ScopeMutex<Mutex> lock(m_mutex);

    auto it = m_pending_events.begin();
    //把到期的任务全部加入执行队列，如果该定时器已取消则不添加
    //若某一个到期时间大于当前时间，后面的都大，直接退出
    for(;it!=m_pending_events.end();++it) {
        if((*it).first <= now) {
            if(!(*it).second->is_cancled()) {
                tmps.push_back((*it).second);
                tasks.push_back(std::make_pair((*it).second->getArriveTime(), (*it).second->getCallBack()));
            }
        }else{
            break;
        }
    }

    m_pending_events.erase(m_pending_events.begin(), it);
    lock.unlock();

    //把重复的事件重新放回去

    for(auto it = tmps.begin(); it!=tmps.end(); ++it) {
        if((*it)->is_repeated()) {
            (*it)->resetArriveTime();
            addTimerEvent(*it);
        }
    }

    resetArriveTime();
    //执行回调函数
    for(auto i: tasks) {
        if(i.second) {
            i.second();
        }
    }
}

void Timer::resetArriveTime() {
    ScopeMutex<Mutex> lock(m_mutex);
    auto tmp = m_pending_events;
    lock.unlock();

    if(tmp.size() == 0) {
        return;
    }

    int64_t now = getNowMs();
    auto it = tmp.begin();
    int64_t interval = 0;
    //如果对头最近的到期时间大于当前事件，则脉搏时间为二者差
    //否则说明对头的定时任务到期，但还未执行，将interval设置为100ms
    if(it->second->getArriveTime() > now) {
        interval = it->second->getArriveTime() - now;
    }else{
        interval = 100;
    }

    timespec ts;
    memset(&ts, 0 , sizeof(ts));
    ts.tv_sec = interval / 1000;
    ts.tv_nsec = (interval % 1000) * 1000000;

    itimerspec value;
    memset(&value, 0, sizeof(value));
    value.it_value = ts;

    int rt = timerfd_settime(m_fd, 0, &value, NULL);
    if( rt != 0) {
        ERRORLOG("timerfd_settime error, errno=%d, error=%s", errno, strerror(errno));
    }
}

void Timer::addTimerEvent(TimerEvent::s_ptr event) {
    bool is_reset_timerfd = false;
    ScopeMutex<Mutex> lock(m_mutex);

    //如果队列为空，需要调整定时器容器的到期时间
    //如果需要添加的事件到到期时间小于定时器头任务的到期时间
    //添加新事件后，脉搏时间需要更新为最新任务的到期时间
    if( m_pending_events.empty()) {
        is_reset_timerfd = true;
    }else {
        auto it = m_pending_events.begin();
        if((*it).second->getArriveTime() > event->getArriveTime()) {
            is_reset_timerfd = true;
        }
    }

    m_pending_events.emplace(event->getArriveTime(), event);
    lock.unlock();

    if(is_reset_timerfd) {
        resetArriveTime();
    }
}

void Timer::deleteTimerEvent(TimerEvent::s_ptr event) {
  event->setCancled(true);

  ScopeMutex<Mutex> lock(m_mutex);

//根据时间找到 对应时间的s_ptr，然后找到对应的事件删除
  auto begin = m_pending_events.lower_bound(event->getArriveTime());
  auto end = m_pending_events.upper_bound(event->getArriveTime());

  auto it = begin;
  for (it = begin; it != end; ++it) {
    if (it->second == event) {
      break;
    }
  }

  if (it != end) {
    m_pending_events.erase(it);
  }
  lock.unlock();

  DEBUGLOG("success delete TimerEvent at arrive time %lld", event->getArriveTime());

}

}