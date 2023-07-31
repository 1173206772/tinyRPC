#ifndef ROCKET_NET_TIMEREVENT_H
#define ROCKET_NET_TIMEREVENT_H
#include <functional>
#include <memory>

namespace rocket {

class TimerEvent {
public:
    typedef std::shared_ptr<TimerEvent> s_ptr;
    TimerEvent(int interval, bool is_repeated, std::function<void()> cb);



    int64_t getArriveTime() const {
        return m_arrive_time;
    }
    void setCancled(bool value) {
        m_is_cancled = value;
    }
    bool is_cancled() const { return m_is_cancled;}
    bool is_repeated() const { return m_is_repeated;}
    std::function<void()> getCallBack() const { return m_task;}
    void resetArriveTime();
private:
    int64_t m_arrive_time; //ms
    int64_t m_interval; //ms
    bool m_is_repeated {false};
    bool m_is_cancled {false};

    std::function<void()> m_task;
};

}

#endif