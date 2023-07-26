#include <unistd.h>
#include "rocket/net/wakeup_fd_event.h"
#include "rocket/common/log.h"
namespace rocket {
    WakeUpFdEvent::WakeUpFdEvent(int fd): FdEvent(fd) {

    }
    WakeUpFdEvent::~WakeUpFdEvent() {

    }
    void WakeUpFdEvent::init() {
        m_read_callback = [this]() {
            char buf[8];
            while(read(m_fd, buf, 8) != -1 && errno != EAGAIN) {

            }
            DEBUGLOG("read full bytes from wakeup fd[%d]", m_fd);
        };
       
    }

    void WakeUpFdEvent::wakeup() {
        char buf[8] = {'a'};
        int rt = write(m_fd, buf, 8);

        if(rt != 8) {
            ERRORLOG("write to wakeup fd less than 8 bytes, fd[%d]",m_fd);
        }
        DEBUGLOG("success read 8 bytes");
    }
}