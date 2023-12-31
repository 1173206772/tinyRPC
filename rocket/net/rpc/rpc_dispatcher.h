#if !defined(ROCKET_NET_RPC_RPC_DISPATCHER_H)
#define ROCKET_NET_RPC_RPC_DISPATCHER_H
#include <string>
#include <memory>
#include <google/protobuf/service.h>
#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/tcp/tcp_connection.h"

namespace rocket {
class TcpConnection;

class RpcDispatcher {
public:
    typedef std::shared_ptr<google::protobuf::Service> service_s_ptr;
   
    static RpcDispatcher* GetRpcDispatcher();

    void dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection);
    void registerService(service_s_ptr service);

    void setTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string error_info);



private:
    bool parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name);

private:
    //保存注册了的service对象
    std::map<std::string, service_s_ptr> m_service_map;  

};

}

#endif // ROCKET_NET_RPC_RPC_DISPATCHER_H
