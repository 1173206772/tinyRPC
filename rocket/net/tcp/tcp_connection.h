#if !defined(ROCKET_NET_TCP_TCP_CONNECTION_H)
#define ROCKET_NET_TCP_TCP_CONNECTION_H
#include <memory>
#include <map>
#include <vector>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/io_thread.h"
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/fd_event.h"
#include "rocket/net/rpc/rpc_dispatcher.h"
namespace rocket {


enum TcpState {
    NotConnected = 1,
    Connected = 2,
    HalfClosing = 3,
    Closed = 4,
};

enum TcpConnectionType {
    TcpConnectionByServer = 1, //作为服务端使用，代表跟对端客户端连接
    TcpConnectionByClient = 2, //作为客户端使用，代表跟对端服务端连接
};

class RpcDispatcher;
class TcpConnection {

public:
    typedef std::shared_ptr<TcpConnection> s_ptr;

    TcpConnection(Eventloop* event_loop, int fd, int buffer_size,  NetAddr::s_ptr local_addr, NetAddr::s_ptr peer_addr, TcpConnectionType type = TcpConnectionByServer);

    ~TcpConnection();
    
    void onRead();

    //将rpc请求作为入参，执行业务逻辑得到RPC响应
    void excute();

    void onWrite();

      // 启动监听可写事件
    void listenWrite();

    // 启动监听可读事件
    void listenRead();
    

    void setState(const TcpState state);

    void clear(); //清理关闭连接

    void shutdown(); //服务器主动关闭连接

    void setConnectionType(TcpConnectionType type);

    TcpState getState();

    void reply(std::vector<AbstractProtocol::s_ptr>& replay_messages);
    void pushSendMessage(AbstractProtocol::s_ptr message, std::function<void(AbstractProtocol::s_ptr)> done);

    void pushReadMessage(const std::string& msg_id, std::function<void(AbstractProtocol::s_ptr)> done);

    NetAddr::s_ptr getLocalAddr() const;
    NetAddr::s_ptr getPeerAddr() const;

private:

    Eventloop* m_eventloop {NULL}; //持有该连接对应io线程
    
    NetAddr::s_ptr m_local_addr;  
    NetAddr::s_ptr m_peer_addr; 

    int m_fd {-1};

    TcpBuffer::s_ptr m_in_buffer;   //接收缓冲区
    TcpBuffer::s_ptr m_out_buffer;  //发送缓冲区


    FdEvent* m_fd_event {NULL}; //

    TcpState m_state;

    TcpConnectionType m_connection_type {TcpConnectionByServer};

    std::vector<std::pair<AbstractProtocol::s_ptr, std::function<void(AbstractProtocol::s_ptr)>>> m_write_dones;

    // key 为 msg_id
    std::map<std::string, std::function<void(AbstractProtocol::s_ptr)>> m_read_dones;

    AbstractCoder* m_coder {NULL};

};



}


#endif // ROCKET_NET_TCP_TCP_CONNECTION_H
    