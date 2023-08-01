#if !defined(ROCKET_NET_TCP_TCP_ACCEPTOR_H)
#define ROCKET_NET_TCP_TCP_ACCEPTOR_H

#include "rocket/net/tcp/net_addr.h"
namespace rocket {

class TcpAcceptor {
public:

    TcpAcceptor(NetAddr::s_ptr local_addr); 
    ~TcpAcceptor();

    int accept();
private:
    NetAddr::s_ptr m_local_addr; //服务器监听的地址，addr -> ip:port

    int m_family {-1}; //地址协议族

    int m_listenfd {-1}; //监听套接字



}

}


#endif // ROCKET_NET_TCP_TCP_ACCEPTOR_H
