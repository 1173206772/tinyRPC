# tinyRPC
To write a tiny RPC framework from scratch


## 1.日志模块开发

日志模块：
1.  日志级别
2.  打印到文件，支持日期命令，以及日志的滚动
3.  c 格式化风格
4.  线程安全

LogLevel:
```
Debug
Info
Error
```

LogEvent:
```
文件名、行号
MsgNo
进程号
Thread id
日期，时间精确到ms
自定义消息
```
日志格式
```
[Level][%y-%m-%d %M:%H:%s.%ms]\t[pid:thread_id]\t[file_name:line][%msg] 


Logger 日志器

