#if !defined(ROCKRT_NET_ABSTRACT_CODER_H)
#define ROCKRT_NET_ABSTRACT_CODER_H
#include <vector>
#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/net/coder/abstract_protocol.h"

namespace rocket {
class AbstractCoder {
public:
    
    //将message对象转化为字节流，写入到buffer
    virtual void encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) = 0;

    //从buffer中导出字节流，转化为Message对象

    virtual void decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer ) = 0;

    virtual ~AbstractCoder() {};

};

}


#endif // ROCKRT_NET_TCP_ABSTRACT_CODER_H
