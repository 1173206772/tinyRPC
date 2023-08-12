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


## 2.IO模块


## 3.TCP模块

### 3.1 TcpBuffer
为什么需要应用层Buffer:
1.  方便数据处理，特别是应用层的包组装和拆解
2.  方便异步的传送（发送数据直接到一个缓冲区里，等待epoll异步去发送
3.  提高发送效率，多个包合并一起发送



## 4.RPC

                  request               response
read----->decode----------->dispatcher------------>encode------->write

启动的时候就注册OrderService 对象。


1. 从buufer读取数据，然后 decode 得到请求的 TinyPBProtobol 对象。然后从请求的 TinyPBProtobol 得到 method_name, 从 OrderService 对象里根据 service.method_name 找到方法 func
2. 找到对应的 requeset type 以及 response type
3. 将请求体 TinyPBProtobol 里面的 pb_date 反序列化为 requeset type 的一个对象, 声明一个空的 response type 对象
4. func(request, response)
5. 将 reponse 对象序列为 pb_data。 再塞入到 TinyPBProtobol 结构体中。做 encode 然后塞入到buffer里面，就会发送回包了