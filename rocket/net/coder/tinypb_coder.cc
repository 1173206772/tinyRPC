#include <vector>
#include <string.h>
#include <arpa/inet.h>
#include "rocket/common/log.h"
#include "rocket/common/util.h"
#include "rocket/net/coder/tinypb_coder.h"
#include "rocket/net/coder/tinypb_protocol.h"

#define INDEX_VALID(index) if(index >= end_idx) { \
            message->parse_success = false;              \
            ERRORLOG("parse error, [%s][%d] >= end_idx",#index,index)\
        }\

#define ADD_LEN_FIELD_TO_BUF(field)    int32_t field##_net = htonl(field);\
    memcpy(tmp, &field##_net, sizeof(field##_net));\
    tmp += sizeof(field##_net);\

#define ADD_FIELD_TO_BUF(field)  if (!message->field.empty()) {\
    memcpy(tmp, &(message->field[0]), message->field.length());\
    tmp += message->field.length();\
    }\

namespace rocket {

//Message对象转换为字节流，然后导入buffer
void TinyPBCoder::encode(std::vector<AbstractProtocol::s_ptr>& messages, TcpBuffer::s_ptr out_buffer) {
    //for(size_t i=0; i<messages.size(); ++i) {
    for(auto& i:messages) {
        std::shared_ptr<TinyPBProtocol> message = std::dynamic_pointer_cast<TinyPBProtocol>(i);
        int len = 0;
        const char* buf = encodeTinyPB(message, len);
        if(buf!=NULL &&len!=0) {
            out_buffer->writeToBuffer(buf, len);
        }

        if(buf) {
            free((void*)buf);
            buf = NULL;
        }
    }

}


//从buffer中导出字节流，转化为Message对象
void TinyPBCoder::decode(std::vector<AbstractProtocol::s_ptr>& out_messages, TcpBuffer::s_ptr buffer ) {
    //遍历buffer, 找到PB_START后解析出整包长度，然后得到结束符位置，判断是否为PB_END


    while(true) {
        std::vector<char> tmp = buffer->m_buffer; 
        int start_idx = buffer->readIndex();
        int end_idx = -1;

        int pk_len = 0;
        bool parse_success = false;

        int idx = start_idx;
        for(; idx<buffer->writeIndex(); ++idx) {
            if( tmp[idx] == TinyPBProtocol::PB_START) {
                //从下取四个字节。 由于是网络字节序，需要转为主机字节序
                if( idx + 1 < buffer->writeIndex()) {
                    //pk_Len
                    pk_len = getInt32FromNetByte(&tmp[idx+1]);
                    DEBUGLOG("get pk_len = %d", pk_len);

                    //结束符
                    int j = idx + pk_len - 1;
                    if(j >= buffer->writeIndex()) {
                        //本次没读到结束符
                        continue;
                    }
                    if(tmp[j] == TinyPBProtocol::PB_END) {
                        start_idx = idx;
                        end_idx = j;
                        parse_success = true;
                        break; 
                    }
                }
            }
        }
        if(idx >= buffer->writeIndex()) {
            DEBUGLOG("decode end, read all buffer data")
            return;
        }

        if(parse_success) {
            buffer->moveReadIndex(end_idx - start_idx + 1);
            std::shared_ptr<TinyPBProtocol> message = std::make_shared<TinyPBProtocol>();

            message->m_pk_len = pk_len;
            int msg_id_len_index = start_idx + sizeof(char) + sizeof(message->m_pk_len);
            INDEX_VALID(msg_id_len_index)

            message->m_msg_id_len = getInt32FromNetByte(&tmp[msg_id_len_index]);
            DEBUGLOG("parse msg_id_len = %d", message->m_msg_id_len);

            int msg_id_index = msg_id_len_index + sizeof(message->m_msg_id_len);
            INDEX_VALID(msg_id_index)

            char msg_id[100] = {0};
            memcpy(&msg_id[0], &tmp[msg_id_index], message->m_msg_id_len);
            message->m_msg_id = std::string(msg_id);
            DEBUGLOG("parse msg_id = %s", message->m_msg_id.c_str());

            int method_name_len_index = msg_id_index + message->m_msg_id_len;
            INDEX_VALID(method_name_len_index)
            message->m_method_name_len = getInt32FromNetByte(&tmp[method_name_len_index]);

            int method_name_idx = method_name_len_index + sizeof(message->m_method_name_len);
            INDEX_VALID(method_name_idx)

            char method_name[512] ={0};
            memcpy(&method_name[0], &tmp[method_name_idx], message->m_method_name_len);
            message->m_method_name = std::string(method_name);
            DEBUGLOG("parse method_name = %s", message->m_method_name.c_str());

            int err_code_idx = method_name_idx + message->m_method_name_len;
            INDEX_VALID(err_code_idx)
            message->m_err_code = getInt32FromNetByte(&tmp[err_code_idx]);

            int err_info_len_idx = err_code_idx + sizeof(message->m_err_code);
            INDEX_VALID(err_info_len_idx)
            message->m_err_info_len = getInt32FromNetByte(&tmp[err_info_len_idx]);

            int err_info_idx = err_info_len_idx + sizeof(message->m_err_info_len);
            INDEX_VALID(err_info_idx)

            char err_info[512] ={0};
            memcpy(&err_info[0], &tmp[err_info_idx], message->m_err_info_len);
            message->m_err_info = std::string(err_info);
            DEBUGLOG("parse err_info = %s", message->m_err_info.c_str());

            int pd_data_len = message->m_pk_len - message->m_msg_id_len - message->m_method_name_len - message->m_err_info_len - 2 - 24;

            int pd_data_idx = err_info_idx + message->m_err_info_len;
            message->m_pb_data = std::string(&tmp[pd_data_idx], pd_data_len);

            //校验和解析
            message->parse_success = true;
            out_messages.push_back(message);

        }
    }
   


}

const char* TinyPBCoder::encodeTinyPB(std::shared_ptr<TinyPBProtocol> message, int& len) {
    if(message->m_msg_id.empty()) {
        message->m_msg_id = "123456";
    }

    DEBUGLOG("msg_id = %d",message->m_msg_id)
    int pk_len = 2 + 24 + message->m_msg_id.length() + message->m_method_name.length() + message->m_err_info.length() + message->m_pb_data.length();
    DEBUGLOG("pk_len = %d", pk_len);
 

    char* buf = reinterpret_cast<char*>(malloc(sizeof(char)*pk_len));
    //char* buf = reinterpret_cast<char*>(malloc(pk_len));
    char* tmp = buf;

    *tmp = TinyPBProtocol::PB_START;
    tmp++;

    //增加包长度字段
    ADD_LEN_FIELD_TO_BUF(pk_len)


    //增加msg_id长度字段
    int msg_id_len = message->m_msg_id.length();
    ADD_LEN_FIELD_TO_BUF(msg_id_len)

    //增加msg_id字段
    ADD_FIELD_TO_BUF(m_msg_id)

    //增加method_name长度字段
    int method_name_len = message->m_method_name.length();
    ADD_LEN_FIELD_TO_BUF(method_name_len)


    //增加method_name字段
    ADD_FIELD_TO_BUF(m_method_name)

    //增error_code字段, 调试到这里有问题 free(): invalid pointer
    int err_code = message->m_err_code;
    ADD_LEN_FIELD_TO_BUF(err_code)

    DEBUGLOG("aaaaaaaaaa");
    //增error_info长度字段
    int err_info_len = message->m_err_info.length();
    ADD_LEN_FIELD_TO_BUF(err_info_len)

    //增error_info字段
    ADD_FIELD_TO_BUF(m_err_info)
    DEBUGLOG("aaaaaaaaaa");
    //增加包数据字段
    ADD_FIELD_TO_BUF(m_pb_data)

    int32_t check_sum_net = htonl(1);
    memcpy(tmp, &check_sum_net, sizeof(check_sum_net));
    tmp += sizeof(check_sum_net);
    *tmp = TinyPBProtocol::PB_END;
    DEBUGLOG("aaaaaaaaaa");
    message->m_pk_len = pk_len;
    message->m_msg_id_len = msg_id_len;
    message->m_method_name_len = method_name_len;
    message->m_err_info_len = err_info_len;
    message->parse_success = true;
    len = pk_len;

    DEBUGLOG("encode message[%s] success", message->m_msg_id.c_str());

    return buf;

}


}