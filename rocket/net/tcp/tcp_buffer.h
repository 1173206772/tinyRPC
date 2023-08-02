#if !defined(ROCKET_NET_TCP_TCP_BUFFER_H)
#define ROCKET_NET_TCP_TCP_BUFFER_H
#include <vector>
namespace rocket {

class TcpBuffer {


public:
    TcpBuffer(int size);

    ~TcpBuffer();

    //返回可读字节数
    int readAble();

    //返回可写字节数
    int writeAble();

    int readIndex() const { return m_read_index;}

    int writeIndex() const { return m_write_index;}

    void writeToBuffer(const char* buf, int size);

    void readFromBuffer(std::vector<char>& re, int size);

    void resizeBuffer(int newsize);

    void adjustBuffer();

    void moveReadIndex(int size);

    void moveWriteIndex(int size);

private:
    int m_read_index {0};
    int m_write_index {0};
    int m_size {0};

    std::vector<char> m_buffer;
};

}


#endif // ROCKET_NET_TCP_BUFFER_H