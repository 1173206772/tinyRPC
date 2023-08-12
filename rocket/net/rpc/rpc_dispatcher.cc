#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/rpc/rpc_dispatcher.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/net/tcp/net_addr.h"
#include "rocket/common/run_time.h"
#include "rocket/common/log.h"
#include "rocket/common/error_code.h"

namespace rocket { 
 
static RpcDispatcher* g_rpc_dispatcher = NULL;

RpcDispatcher* RpcDispatcher::GetRpcDispatcher() {
    if(g_rpc_dispatcher != NULL) {
        return g_rpc_dispatcher;
    }
    g_rpc_dispatcher = new RpcDispatcher();
    return g_rpc_dispatcher;
}

void RpcDispatcher::dispatch(AbstractProtocol::s_ptr request, AbstractProtocol::s_ptr response, TcpConnection* connection) {

    std::shared_ptr<TinyPBProtocol> req_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(request);
    std::shared_ptr<TinyPBProtocol> rsp_protocol = std::dynamic_pointer_cast<TinyPBProtocol>(response);
    //method_name 类似OrderService.make_order 所以要解析
    std::string method_full_name = req_protocol->m_method_name;
    std::string service_name;
    std::string method_name;

    rsp_protocol->m_msg_id = req_protocol->m_msg_id;
    rsp_protocol->m_method_name = req_protocol->m_method_name;

    if (false == parseServiceFullName(method_full_name, service_name, method_name) ) {
        ERRORLOG("%s | parse service name[%s] error", req_protocol->m_msg_id.c_str(), method_full_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_PARSE_SERVICE_NAME, "parse service name error");
        return;
    } 

    auto it = m_service_map.find(service_name);
    if ( it == m_service_map.end()) {

        ERRORLOG("%s | service name[%s] not found",req_protocol->m_msg_id.c_str(), service_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_SERVICE_NOT_FOUND, "service not found");
        return;
    }

    service_s_ptr service = it->second;
    const google::protobuf::MethodDescriptor* method =  service->GetDescriptor()->FindMethodByName(method_name);
    if( method == NULL) {
        ERRORLOG("%s | method name[%s] not found in service[%s]", req_protocol->m_msg_id.c_str(), method_name.c_str(), service_name.c_str())
        setTinyPBError(rsp_protocol, ERROR_METHOD_NOT_FOUND, "method not found");
        return;
    }

    //获得request对象
    google::protobuf::Message* req_msg =  service->GetRequestPrototype(method).New();

    //反序列化，将 pb_data 反序列化为req_msg
    if(!req_msg->ParseFromString(req_protocol->m_pb_data)) {
        ERRORLOG("%s | deserialize error", req_protocol->m_msg_id.c_str())
        setTinyPBError(rsp_protocol, ERROR_FAILED_DESERIALIZE, "deserialize error");
        if(req_msg != NULL) {
            delete req_msg;
            req_msg = NULL;
        }
        return;
    }

    INFOLOG("msg_id[%s], get rpc request:[%s]",req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str());


    //获得reponse对象
    google::protobuf::Message* rsp_msg =  service->GetResponsePrototype(method).New();

    RpcController rpcController;
    IPNetAddr::s_ptr local_addr = std::dynamic_pointer_cast<IPNetAddr>(connection->getLocalAddr());
    IPNetAddr::s_ptr peer_addr = std::dynamic_pointer_cast<IPNetAddr>(connection->getPeerAddr());
    rpcController.SetLocalAddr(local_addr);
    rpcController.SetPeerAddr(peer_addr);
    rpcController.SetMsgId(req_protocol->m_msg_id);


    RunTime::GetRunTime()->m_msgid = req_protocol->m_msg_id;
    RunTime::GetRunTime()->m_method_name = method_name;
    //调用proto定义的方法
    service->CallMethod(method, &rpcController, req_msg, rsp_msg, NULL);



    //序列化为字节流
    if(!rsp_msg->SerializePartialToString(&(rsp_protocol->m_pb_data))) {
        ERRORLOG("%s | serialize error, origin message [%s]", req_protocol->m_msg_id.c_str(), rsp_msg->ShortDebugString().c_str())
        
        setTinyPBError(rsp_protocol, ERROR_FAILED_SERIALIZE, "serialize error");

        if(req_msg != NULL) {
            delete req_msg;
            req_msg = NULL;
        }

        if(rsp_msg != NULL) {
            delete rsp_msg;
            rsp_msg = NULL;
        }
        return;
    } 
    

    rsp_protocol->m_err_code = 0;
    INFOLOG("%s | dispatch success, request[%s], response[%s]", req_protocol->m_msg_id.c_str(), req_msg->ShortDebugString().c_str(), rsp_msg->ShortDebugString().c_str())
    std::vector<AbstractProtocol::s_ptr> reply_msgs;
    reply_msgs.push_back(rsp_protocol);
    connection->reply(reply_msgs);
    
    delete req_msg;
    delete rsp_msg;
    req_msg = NULL;
    rsp_msg = NULL;

} 

void RpcDispatcher::registerService(service_s_ptr service) {
    std::string service_name = service->GetDescriptor()->full_name();
    m_service_map[service_name] = service;
}

bool RpcDispatcher::parseServiceFullName(const std::string& full_name, std::string& service_name, std::string& method_name) {
    
    if(full_name.empty()) {
        ERRORLOG("full name is empty")
        return false;
    }
    size_t idx = full_name.find_first_of('.');

    if(idx == full_name.npos) {
        ERRORLOG("not find '.' in full name [%s]", full_name);
        return false;
    }

    service_name = full_name.substr(0, idx);
    method_name = full_name.substr(idx + 1);
    INFOLOG("parse service_name[%s] and method_name[%s] fron full name [%s]",full_name.c_str(),service_name.c_str(),method_name.c_str())
    return true;
}
void RpcDispatcher::setTinyPBError(std::shared_ptr<TinyPBProtocol> msg, int32_t err_code, const std::string error_info) {
    msg->m_err_code = err_code;
    msg->m_err_info = error_info;
    msg->m_err_info_len = error_info.length();
}

} // namespace rocket
 