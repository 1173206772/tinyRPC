#if !defined(ROCKET_NET_TCP_TCP_SERVER_H)
#define ROCKET_NET_TCP_TCP_SERVER_H

#include <memory>
#include "rocket/net/tcp/tcp_acceptor.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/io_thread_group.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/tcp/net_addr.h"

namespace rocket {
class TcpServer {
public:
    TcpServer(NetAddr::s_ptr local_addr);

    ~TcpServer();

    void start();

private:
    void init();

    //当有新客户端连接后需要执行
    void OnAccept();


private:
    TcpAcceptor::s_ptr m_acceptor;
    NetAddr::s_ptr m_local_addr;  //本地监听地址

    Eventloop* m_main_event_loop {NULL};    //mainReactor

    IOThreadGroup* m_io_thread_group {NULL};    //subReactor group

    FdEvent* m_listen_fd_event;

    int m_client_counts {0};

    std::set<TcpConnection::s_ptr> m_client;

};


}


#endif // ROCKET_NET_TCP_TCP_SERVER_H
