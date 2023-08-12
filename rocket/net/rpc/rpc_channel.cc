#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include "rocket/net/coder/tinypb_protocol.h"
#include "rocket/common/msg_id_util.h"
#include "rocket/common/error_code.h"
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/timer_event.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/common/log.h"

namespace rocket
{
    

RpcChannel::RpcChannel(NetAddr::s_ptr peer_addr) :m_peer_addr(peer_addr) {
    DEBUGLOG("RpcChannel create")
    m_tcpclient = std::make_shared<TcpClient>(peer_addr);

}

RpcChannel::~RpcChannel() {
    DEBUGLOG("~RpcChannel create")

}

void RpcChannel::Init(controller_s_ptr controller, message_s_ptr request, message_s_ptr response, closure_s_ptr closure) {
    if(m_is_init) {
        return;
    }

    m_controller = controller;
    m_request = request;
    m_response = response;
    m_closure = closure;
    m_is_init = true;
}

//获取智能指针对应指针
google::protobuf::RpcController* RpcChannel::getController() {
    return m_controller.get();
}

google::protobuf::Message* RpcChannel::getResponse() {
    return m_response.get();
}

google::protobuf::Message* RpcChannel::getRequest() {
    return m_request.get();
}

google::protobuf::Closure* RpcChannel::getClosure() {
    return m_closure.get();
}

TcpClient* RpcChannel::getTcpClient() {
    return m_tcpclient.get();
}

TimerEvent::s_ptr RpcChannel::getTimerEvent() {
    return m_timer_event;
}

void RpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                        google::protobuf::RpcController* controller, const google::protobuf::Message* request,
                        google::protobuf::Message* reponse, google::protobuf::Closure* done) {

    std::shared_ptr<rocket::TinyPBProtocol> req_protocol = std::make_shared<rocket::TinyPBProtocol>();
    RpcController* my_controller = dynamic_cast<RpcController*>(controller);
    if(my_controller == NULL) {
        ERRORLOG("failed callmethod, RpcController convert error");
        return;
    }

    if(my_controller->GetMsgId().empty()) {

        req_protocol->m_msg_id = MsgIDUtil::GetMsgID();
        my_controller->SetMsgId(req_protocol->m_msg_id);

    }else {
        req_protocol->m_msg_id =  my_controller->GetMsgId();
    }
    req_protocol->m_method_name = method->full_name();
    INFOLOG("%s | callmethod name [%s]",req_protocol->m_msg_id.c_str(),req_protocol->m_method_name.c_str())

    if(!m_is_init) {
        //序列化前一定要初始化，设置好各智能指针，通过成员变量避免调用过程中各对象被析构了
        std::string err_info = "RpcChannel not init";
        my_controller->SetError(ERROR_RPC_CHANNEL_INIT, err_info);
        ERRORLOG("%s | %s, RpcChannel not init", req_protocol->m_msg_id.c_str(), err_info.c_str())
        return;
    }

    //请求序列化
    if (!request->SerializePartialToString(&(req_protocol->m_pb_data))) {

        std::string err_info = "failed serialize";
        my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);

        ERRORLOG("%s | %s, original request [%s]",req_protocol->m_msg_id.c_str(), err_info.c_str(), request->ShortDebugString().c_str())
        return;
    }

    s_ptr channel = shared_from_this(); //因为是回调函数的形式，避免channel是栈对象被析构，所以需要this指针，延长生命周期

    //构建超时任务
    m_timer_event = std::make_shared<TimerEvent>(my_controller->GetTimeout(), false, [my_controller, channel]() mutable{
        my_controller->StartCancel(); //标志rpc请求取消
        my_controller->SetError(ERROR_RPC_CALL_TIMEOUT, "rpc call timeout "+ std::to_string(my_controller->GetTimeout()));

        if(channel->getClosure()) {
            channel->getClosure()->Run();
        }

        channel.reset();
    });

    m_tcpclient->addTimerEvent(m_timer_event); //添加定时时间

   
    
    //按值捕获到lambda函数中的变量在函数体中默认是const类型，即不可修改，
    //在添加了mutable修饰符后，便可以对此变量进行修改，但此时仍然修改的是位于lambda函数体中的局部变量
    m_tcpclient->connect([req_protocol, channel]() mutable{

        RpcController* my_controller = dynamic_cast<RpcController*>(channel->getController());
        if(channel->getTcpClient()->getConnectErrorCode() !=0 ){
            my_controller->SetError(channel->getTcpClient()->getConnectErrorCode(), channel->getTcpClient()->getConnectErrorInfo());
            ERRORLOG("%s | connect error, error code[%d], error info[%s], peer addr[%s]",
                    req_protocol->m_msg_id.c_str(),channel->getTcpClient()->getConnectErrorCode(),
                    channel->getTcpClient()->getConnectErrorInfo().c_str(), channel->getTcpClient()->getPeerAddr()->toString().c_str())
            return;
        }

        channel->getTcpClient()->writeMessage(req_protocol, [req_protocol, channel, my_controller](AbstractProtocol::s_ptr) mutable{
            INFOLOG("%s | send rpc request success. call method name[%s], peer addr[%s], local addr[%s]",
                    req_protocol->m_msg_id.c_str(), req_protocol->m_method_name.c_str(),
                    channel->getTcpClient()->getPeerAddr()->toString().c_str(),
                    channel->getTcpClient()->getLocalAddr()->toString().c_str())

            channel->getTcpClient()->readMessage(req_protocol->m_msg_id, [channel, my_controller](AbstractProtocol::s_ptr msg) mutable {
                std::shared_ptr<rocket::TinyPBProtocol> rsp_protocol  = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg);
                DEBUGLOG("%s | receive rpc response success. call method name[%s], peer addr[%s], local addr[%s]", 
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->getTcpClient()->getPeerAddr()->toString().c_str(),
                    channel->getTcpClient()->getLocalAddr()->toString().c_str())

                    

                if(!channel->getResponse()->ParseFromString(rsp_protocol->m_pb_data)) {
                    std::string err_info = "failed serialize response";
                    my_controller->SetError(ERROR_FAILED_SERIALIZE, err_info);
                    return;
                } 

                if(rsp_protocol->m_err_code != 0) {
                    ERRORLOG("%s | call rpc method[%s] failed, error code[%s], error info[%s]",
                        rsp_protocol->m_msg_id, rsp_protocol->m_method_name, rsp_protocol->m_err_code, rsp_protocol->m_err_info
                        )
                    my_controller->SetError(rsp_protocol->m_err_code, rsp_protocol->m_err_info);
                    return;

                }

                INFOLOG("%s | call rpc success, call method name[%s], peer addr[%s], local addr[%s]",
                    rsp_protocol->m_msg_id.c_str(), rsp_protocol->m_method_name.c_str(),
                    channel->getTcpClient()->getPeerAddr()->toString().c_str(),
                    channel->getTcpClient()->getLocalAddr()->toString().c_str())

                
                
                if(!my_controller->IsCanceled() && channel->getClosure()) {
                    channel->getClosure()->Run();
                }

                channel.reset(); //引用计数-1
                
            });
        });

    });
    
}
} // namespace rocket
