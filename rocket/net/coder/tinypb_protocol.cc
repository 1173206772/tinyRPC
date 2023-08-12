#include "rocket/net/coder/tinypb_protocol.h"

namespace rocket {
//在cpp文件对类的静态成员变量进行初始化，否则会出现重定义问题，因为多个文件包含了同一个头文件

char TinyPBProtocol::PB_START = 0x02;
char TinyPBProtocol::PB_END = 0x03;

}