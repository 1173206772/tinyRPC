#ifndef ROCKERT_NET_STRING_CODER_H
#define ROCKERT_NET_STRING_CODER_H

#include "rocket/net/coder/abstract_protocol.h"
#include "rocket/net/coder/abstract_coder.h"

namespace  rocket {
class StringProtocol: public AbstractProtocol {
public:
    std::string info;

};




// dynamic_cast 将一个基类对象指针安全地cast为子类，并用派生类的指针或引用调用非虚函数
// dynamic_pointer_cast与dynamic_cast用法类似，
//             当指针是智能指针时候，向下转换，用dynamic_Cast 则编译不能通过，此时需要使用dynamic_pointer_cast。
class StringCoder: public AbstractCoder {
public:
    //将message对象转化为字节流，写入到buffer
    void encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) override {
        for( size_t i=0; i<messages.size(); ++i) {
            //将父类智能指针类型转化为子类
            std::shared_ptr<StringProtocol> msg = std::dynamic_pointer_cast<StringProtocol>(messages[i]);
            out_buffer->writeToBuffer(msg->info.c_str(), msg->info.length());
        }
    }

    //从buffer中导出字节流，转化为Message对象

    void decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer ) override {
        std::vector<char> re;

        buffer->readFromBuffer(re, buffer->readAble());
        std::string info;
        for(size_t i =0; i<re.size(); ++i) {
            info += re[i];
        }

        std::shared_ptr<StringProtocol> msg = std::make_shared<StringProtocol>();
        msg->info = info;
        msg->m_msg_id = "123456";
        out_messages.push_back(msg);


    }

};



}

#endif
