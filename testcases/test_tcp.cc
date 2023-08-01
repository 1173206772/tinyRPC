#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include "rocket/net/tcp/net_addr.h"


int main()
{
    rocket::Config::SetGlobalConfig("../conf/rocket.xml");
    rocket::Logger::InitGlobalLogger();

    rocket::IPNetAddr addr("156.101.10.2",12345);
    DEBUGLOG("IP: %s ",addr.toString().c_str())
    
    rocket::IPNetAddr addr2("192.168.12.3:8800");
    DEBUGLOG("IP: %s ",addr2.toString().c_str())

    return 0;

}