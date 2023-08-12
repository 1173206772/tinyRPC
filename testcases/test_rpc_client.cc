#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/tcp_connection.h"
#include "rocket/net/tcp/tcp_client.h"
#include "rocket/net/coder/string_coder.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "order.pb.h"
#include <pthread.h>
#include <google/protobuf/service.h>
#include "rocket/net/rpc/rpc_channel.h"
#include "rocket/net/rpc/rpc_controller.h"
#include "rocket/net/rpc/rpc_closure.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


void test_tcp_client() {

  rocket::IPNetAddr::s_ptr addr = std::make_shared<rocket::IPNetAddr>("127.0.0.1", 12345);
  rocket::TcpClient client(addr);
  client.connect([addr, &client]() {
    DEBUGLOG("conenct to [%s] success", addr->toString().c_str());
    std::shared_ptr<rocket::TinyPBProtocol> message = std::make_shared<rocket::TinyPBProtocol>();
    message->m_msg_id = "56788765";
    message->m_pb_data = "test pb data";

    makeOrderRequest request;
    request.set_price(100);
    request.set_goods("apples");

    if(!request.SerializeToString(&(message->m_pb_data))) {
        ERRORLOG("serialize error")
        return;
    }

    message->m_method_name = "Order.makeOrder";


    client.writeMessage(message, [request](rocket::AbstractProtocol::s_ptr msg_ptr) {
      DEBUGLOG("send message success, request[%s]", request.ShortDebugString().c_str());
    });

    client.readMessage("56788765", [](rocket::AbstractProtocol::s_ptr msg_ptr) {
      std::shared_ptr<rocket::TinyPBProtocol> message = std::dynamic_pointer_cast<rocket::TinyPBProtocol>(msg_ptr);
      DEBUGLOG("msg_id[%s], get response %s", message->m_msg_id.c_str(), message->m_pb_data.c_str());
      makeOrderResponse response;
      if(!response.ParseFromString(message->m_pb_data)) {
        ERRORLOG("deserialize error")
        return;
      }
      DEBUGLOG("get response success, response[%s]", response.ShortDebugString().c_str());
    });
  });
}

void test_rpc_channel() {

//声明地址

  NEWRPCCHANNEL("127.0.0.1:12345", channel)
  NEWMESSAGE(makeOrderRequest, request)   //构建请求的对象

  request->set_price(100);
  request->set_goods("apple");

  NEWMESSAGE(makeOrderResponse,response) //构建响应的对象
  NEWRPCCONTROLLER(controller)

  controller->SetMsgId("99990000");
  controller->SetTimeout(6000);

  std::shared_ptr<rocket::RpcClosure> closure = std::make_shared<rocket::RpcClosure>([channel, request, response, controller]() mutable{
    if(controller->GetErrorCode() ==0) {
      INFOLOG("call rpc success, request[%s], response[%s]", request->ShortDebugString().c_str(), response->ShortDebugString().c_str())
      //执行业务逻辑
    }else {
      ERRORLOG("call rpc failed, request[%s], error code[%d], error info[%s]", 
                                    request->ShortDebugString().c_str(), 
                                    controller->GetErrorCode(), 
                                    controller->GetErrorInfo().c_str())
    }
    INFOLOG("the eventloop exit now")
    channel->getTcpClient()->stop();
    channel.reset();

  });

  CALLRPRC("127.0.0.1:12345", Order_Stub, makeOrder, controller, request, response, closure)

}

int main()
{
    rocket::Config::SetGlobalConfig(NULL);

    rocket::Logger::InitGlobalLogger();
    test_rpc_channel();

    return 0;
}