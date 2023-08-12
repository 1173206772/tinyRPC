#include "rocket/net/tcp/tcp_buffer.h"
#include "rocket/common/log.h"
#include <string.h>
namespace rocket {

TcpBuffer::TcpBuffer(int size):m_size(size) {
    m_buffer.resize(m_size);
}

TcpBuffer::~TcpBuffer(){

}

//返回可读字节数
int TcpBuffer::readAble() {
    return m_write_index - m_read_index;
}

//返回可写字节数
int TcpBuffer::writeAble() {
    return m_buffer.size() - m_write_index;
}

void TcpBuffer::resizeBuffer(int newsize) {
    std::vector<char> tmp(newsize);
    int count = std::min(newsize, readAble());
    memcpy(&tmp[0], &m_buffer[m_read_index], count);
    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = count;
}

void TcpBuffer::writeToBuffer(const char* buf, int size) {
    if( size > writeAble() ) {
        //调整buffer的大小，扩容
        int new_size = (int)(1.5 * (m_write_index + size));
        resizeBuffer(new_size);
    }
    memcpy(&m_buffer[m_write_index], buf, size);
    m_write_index += size;
}


void TcpBuffer::readFromBuffer(std::vector<char>& re, int size) {
    if( readAble() == 0) return;
    int read_size = std::min(size, readAble());
    
    std::vector<char> tmp(read_size);
    memcpy(&tmp[0], &m_buffer[m_read_index], read_size);

    re.swap(tmp);
    m_read_index += read_size;

    adjustBuffer();
}

void TcpBuffer::adjustBuffer() {
    if(m_read_index < int(m_buffer.size() / 3)) {
        return;
    }

    int count = readAble();
    std::vector<char> buffer(m_buffer.size());
    memcpy(&buffer[0], &m_buffer[m_read_index], count);
    m_buffer.swap(buffer);

    m_read_index = 0;
    m_write_index = m_read_index + count;
    
    buffer.clear(); //清除旧的buffer
}


void TcpBuffer::moveReadIndex(int size) {
    size_t j = m_read_index + size;
    if(j >= m_buffer.size()) {
        ERRORLOG("moveReadIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());
        return;
    }
    m_read_index = j;
    adjustBuffer();
}

void TcpBuffer::moveWriteIndex(int size) {
    size_t j = m_write_index + size;
    if(j >= m_buffer.size()) {
        ERRORLOG("moveWriteIndex error, invalid size %d, old_read_index %d, buffer size %d", size, m_read_index, m_buffer.size());
        return;
    }
    m_write_index = j;

}

}