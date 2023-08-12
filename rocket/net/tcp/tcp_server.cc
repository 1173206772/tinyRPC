#include "rocket/net/tcp/tcp_server.h"
#include "rocket/common/log.h"
#include "rocket/net/eventloop.h"
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket
{
TcpServer::TcpServer(NetAddr::s_ptr local_addr):m_local_addr(local_addr) {
    init();
    INFOLOG("rocket TcpServer listen sucess on [%s]", m_local_addr->toString().c_str())
}


TcpServer::~TcpServer() {
    if(m_main_event_loop) {
        delete m_main_event_loop;
        m_main_event_loop = NULL;
    }

}

void TcpServer::start() {
    m_io_thread_group->start();
    m_main_event_loop->loop();
}

 void TcpServer::init() {
    m_acceptor = std::make_shared<TcpAcceptor>(m_local_addr);

    //获取当前线程的全局eventloop
    m_main_event_loop = Eventloop::GetCurrentEventLoop();

    m_io_thread_group = new IOThreadGroup(2);

    //将OnAccept函数绑定，指定为回调函数
    m_listen_fd_event = new FdEvent(m_acceptor->getListenFd());
   
    m_listen_fd_event->listen(FdEvent::IN_EVENT, std::bind(&TcpServer::OnAccept, this));

    m_main_event_loop->addEpollEvent(m_listen_fd_event);

 }

 void TcpServer::OnAccept() {

    //新连接到来，listenfd可读，取出新连接
    auto re = m_acceptor->accept();
    int client_fd = re.first;
    NetAddr::s_ptr peer_addr = re.second;
    //FdEvent client_fd_event(client_fd);
    m_client_counts += 1;

    IOThread* io_thread = m_io_thread_group->getIOThread();
    TcpConnection::s_ptr connection = std::make_shared<TcpConnection>(io_thread->getEventLoop(), client_fd, 128, m_local_addr, peer_addr);
    connection->setState(Connected);
    m_client.insert(connection);

    //m_io_thread_group->getIOThread()->getEventLoop()->addEpollEvent(client_fd_event);
    INFOLOG("TcpServer succ get client fd=%d", client_fd)
    

      
 }

} // namespace rocket
