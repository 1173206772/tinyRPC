#include "rocket/common/log.h"
#include "rocket/common/config.h"
#include <pthread.h>
#include <unistd.h>

void* func(void* attr) {
    DEBUGLOG("this debug is in %s %d","func",(int*)attr);
    INFOLOG("this info is in %s %d","func",(int*)attr);
    return NULL;
}

int main()
{
    rocket::Config::SetGlobalConfig("../conf/rocket.xml");

    rocket::Logger::InitGlobalLogger();

    pthread_t thread;
    pthread_create(&thread, NULL, &func, (void*)12);
    DEBUGLOG("test debug log %s", "zeriol1221");
    INFOLOG("test info log %s", "zeriol zeriol");

    pthread_join(thread, NULL);
    return 0;
}