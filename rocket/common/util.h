#ifndef ROCKET_COMMON_UTIL_H
#define ROCKET_COMMON_UTIL_H

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
namespace rocket {

pid_t getPid();
pid_t getThreadId();
}
#endif