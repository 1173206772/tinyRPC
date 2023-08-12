#if !defined(ROCKET_NET_CODER_TINYPB_CODER_H)
#define ROCKET_NET_CODER_TINYPB_CODER_H
#include "rocket/net/coder/abstract_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {
class TinyPBCoder : public AbstractCoder{
public:
    TinyPBCoder(){};
    ~TinyPBCoder() {};

private:
void encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer);


 //从buffer中导出字节流，转化为Message对象
void decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer );

const char* encodeTinyPB(std::shared_ptr<TinyPBProtocol> message, int& len);

};


}


#endif // ROCKET_NET_CODER_TINYPB_CODER_H
