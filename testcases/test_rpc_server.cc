#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/net/tcp/tcp_server.h"
#include "order.pb.h"
#include <pthread.h>
#include <google/protobuf/service.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

class OrderImpl : public Order {
public:
void makeOrder(google::protobuf::RpcController* controller,
                       const ::makeOrderRequest* request,
                       ::makeOrderResponse* response,
                       ::google::protobuf::Closure* done) {

    APPDEBUGLOG("start sleep 5s")
    sleep(5);
    APPDEBUGLOG("end sleep 5s")
    if(request->price() < 10) {
        response->set_ret_code(-1);
        response->set_res_info("short balance");
        return;
    }
    response->set_order_id("202312345");
    APPDEBUGLOG("call makeOrder success")
}
};


int main(int argc, char* argv[])
{
    if(argc != 2) {
        printf("Start test_rpc_server error, argc not 2\n");
        printf("Start like thie: \n");
        printf("./test_rpc_server ../conf/rocket.xml \n");
        return 0;
    }


    rocket::Config::SetGlobalConfig(argv[1]);

    rocket::Logger::InitGlobalLogger(1);

    std::shared_ptr<OrderImpl> service = std::make_shared<OrderImpl>();
    rocket::RpcDispatcher::GetRpcDispatcher()->registerService(service);


    rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", rocket::Config::GetGlobalConfig()->m_port);
    rocket::TcpServer tcp_server(addr);
    tcp_server.start();
    
    return 0;
}