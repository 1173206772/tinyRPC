#include <unistd.h>
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/fd_event_group.h"
#include "rocket/common/log.h"

namespace rocket
{
    TcpConnection::TcpConnection(Eventloop* eventloop, int fd, int buffer_size,NetAddr::s_ptr local_addr, NetAddr::s_ptr peer_addr, TcpConnectionType type /*= TcpConnectionByServer*/) 
        :m_eventloop(eventloop), m_local_addr(local_addr), m_peer_addr(peer_addr), m_fd(fd), m_state(NotConnected), m_connection_type(type){
        
        m_in_buffer = std::make_shared<TcpBuffer>(buffer_size);
        m_out_buffer = std::make_shared<TcpBuffer>(buffer_size);

        m_fd_event = FdEventGroup::GetFdEventGroup()->getFdEvent(fd);  
        m_fd_event->setNonBlock();

        m_coder = new TinyPBCoder();

        //m_coder = new StringCoder();
        if (m_connection_type == TcpConnectionByServer) {
            listenRead();
        }

    }

    TcpConnection::~TcpConnection() { 
        DEBUGLOG("~tcp connection")
    }

    
    void TcpConnection::onRead() {
        //1. 从socket缓冲区，调用系统的read函数读取字节到m_in_buffer

        if(m_state != Connected) {
            INFOLOG("client has already disconnected, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd)
            return;
        }

        bool is_read_all = false;
        bool is_close = false;

        while(!is_read_all) {

            if(m_in_buffer->writeAble() == 0) {
                m_in_buffer->resizeBuffer(2 * m_in_buffer->m_buffer.size());
            }

            int read_count = m_in_buffer->writeAble();
            int write_idx = m_in_buffer->writeIndex();

            int rt = read(m_fd, &(m_in_buffer->m_buffer[write_idx]), read_count);
            DEBUGLOG("success read %d bytes from addr[%s], clientfd[%d]",rt, m_peer_addr->toString().c_str(), m_fd)

            if(rt > 0) {
                m_in_buffer->moveWriteIndex(rt);

                if(rt == read_count){
                    continue;
                }else if(rt < read_count) {
                    is_read_all = true;
                    break;
                }
            }else if(rt == 0) {
                is_close = true;
                break;
            }else if(rt == -1 && errno == EAGAIN) {
                is_read_all = true;
                break;
            }
        }

        //连接被对端关闭，此时读到的结果为0,当TCP连接在一侧关闭时,另一侧的read()返回0字节.
        if(is_close) {
            clear();
            DEBUGLOG("peer closed, peer addr [%s], clientfd [%d]", m_peer_addr->toString().c_str(), m_fd)
            return;
        }

        if(!is_read_all) {
            ERRORLOG("not read all data")
        }

        //TODO: 简单的echo,后面补充rpc协议
        excute();

    }

    
    void TcpConnection::excute() {
        if(m_connection_type == TcpConnectionByServer) {
           
            //将RPC请求执行业务逻辑， 获取RPC响应，再把RPC响应返送回
            std::vector<AbstractProtocol::s_ptr> result;
            std::vector<AbstractProtocol::s_ptr> reply_messages;
            m_coder->decode(result, m_in_buffer);
            for( size_t i=0; i<result.size(); ++i) {
                //1.    针对每一个请求，调用RPC方法，获得响应message
                //2.    将响应message放入发送缓冲区，监听可写事件回包
                INFOLOG("success get request[%s] from client [%s]", result[i]->m_msg_id.c_str() ,m_peer_addr->toString().c_str())

                TinyPBProtocol::s_ptr reply_message = std::make_shared<TinyPBProtocol>();
                // message->m_pb_data = "hello, this is rocket rpc test data";
                // message->m_msg_id = result[i]->m_msg_id;
           
                RpcDispatcher::GetRpcDispatcher()->dispatch(result[i], reply_message, this);
                reply_messages.emplace_back(reply_message);
                

            }
           
            //注册可写事件
            listenWrite();

        }else {
                //从buffer里decode得到message对象，执行回调
            std::vector<AbstractProtocol::s_ptr> result;
            m_coder->decode(result, m_in_buffer);

            for(size_t i =0; i<result.size(); ++i) {
                std::string msg_id = result[i]->m_msg_id;
                auto it = m_read_dones.find(msg_id);

                if(it != m_read_dones.end()) {
                    it->second(result[i]->shared_from_this());
                }
            }   
            
         }
        
    }

    void TcpConnection::onWrite() {
        //将当前outbuff里的数据全写入socket缓冲区
        if (m_state != Connected) {
            ERRORLOG("onWrite error, client has already disconneced, addr[%s], clientfd[%d]", m_peer_addr->toString().c_str(), m_fd);
            return;
        }

        if (m_connection_type == TcpConnectionByClient) {
            // 1. 将 message encode 得到字节流
            // 2. 将字节流入到 buffer 里面，然后全部发送
            std::vector<AbstractProtocol::s_ptr> messages;

            for (size_t i = 0; i< m_write_dones.size(); ++i) {
                messages.push_back(m_write_dones[i].first);
            } 

            m_coder->encode(messages, m_out_buffer);
        }

        bool is_write_all = false;

        while(true) {
            if (m_out_buffer->readAble() == 0) {
                DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
                is_write_all = true;
                break;
            }
            int write_size = m_out_buffer->readAble();
            int read_index = m_out_buffer->readIndex();

            int rt = write(m_fd, &(m_out_buffer->m_buffer[read_index]), write_size);

            if (rt >= write_size) {
                DEBUGLOG("no data need to send to client [%s]", m_peer_addr->toString().c_str());
                is_write_all = true;
                break;
            }else if (rt == -1 && errno == EAGAIN) {
            // 发送缓冲区已满，不能再发送了。 
            // 这种情况我们等下次 fd 可写的时候再次发送数据即可
                ERRORLOG("write data error, errno==EAGIN and rt == -1");
                break;
            }
        }
        if(is_write_all) {
            m_fd_event->cancel(FdEvent::OUT_EVENT);
            m_eventloop->addEpollEvent(m_fd_event);
        }

        if (m_connection_type == TcpConnectionByClient) {
        for (size_t i = 0; i < m_write_dones.size(); ++i) {
            m_write_dones[i].second(m_write_dones[i].first);
        }
        m_write_dones.clear();
  }
    }

void TcpConnection::reply(std::vector<AbstractProtocol::s_ptr>& replay_messages) {
  m_coder->encode(replay_messages, m_out_buffer);
  listenWrite();
}

void TcpConnection::setState(const TcpState state) {
    //m_state = state;
    m_state = Connected;
}

TcpState TcpConnection::getState() {
    return m_state;
}

void TcpConnection::clear() {
    //处理关闭连接后的清理动作
    if (m_state == Closed) {
        return;
    }
    m_fd_event->cancel(FdEvent::IN_EVENT);
    m_fd_event->cancel(FdEvent::OUT_EVENT);

    m_eventloop->delEpollEvent(m_fd_event);

    m_state = Closed;
}

void TcpConnection::shutdown() {
    //当连接长时间没有数据交换，服务器可以主动关闭连接进行四次挥手
    if( m_state == Closed || m_state == NotConnected) {
        return;
    }

    //处于半关闭
    m_state = HalfClosing;

    //调用shutdown系统调用，意味着服务器不会对这个fd进行读写操作
    //发送FIN报文，触发了四次挥手的第一次
    //当fd发生可读事件，但是read到0字节，意味着对端发送了FIN报文关闭连接

    ::shutdown(m_fd, SHUT_RDWR);
}

  // 启动监听可写事件
void TcpConnection::listenWrite() {

    m_fd_event->listen(FdEvent::OUT_EVENT, std::bind(&TcpConnection::onWrite, this));
    m_eventloop->addEpollEvent(m_fd_event);
}

  // 启动监听可读事件
void TcpConnection::listenRead() {
    m_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpConnection::onRead, this));
    m_eventloop->addEpollEvent(m_fd_event);

}

void TcpConnection::setConnectionType(TcpConnectionType type) {
    m_connection_type = type;
}

void TcpConnection::pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_write_dones.push_back(std::make_pair(message, done));

}

void TcpConnection::pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done) {
    m_read_dones.insert(std::make_pair(msg_id, done));
}

NetAddr::s_ptr TcpConnection::getLocalAddr() const {
    return m_local_addr;
}

NetAddr::s_ptr TcpConnection::getPeerAddr() const {
    return m_peer_addr;
}

} // namespace rocket
