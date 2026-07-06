#include"VecterBuffer.h"

int VectorBuffer::GetUsedSize() {
    return tail - head;
}

int VectorBuffer::GetLinearFreeSize() {
    return static_cast<int>(buffer.size()) - tail;
}

char* VectorBuffer::GetWriteBuffer() {
    return buffer.data() + tail;
}

char* VectorBuffer::GetReadBuffer() {
    return buffer.data() + head;
}

void VectorBuffer::OnRead(int byte) {
    head += byte;
    if (head == tail) {
        head = 0;
        tail = 0;
    }
}

void VectorBuffer::OnWrite(int byte) {
    tail += byte;
}

void VectorBuffer::Peek(char* dest, int len) {
    memcpy(dest, buffer.data() + head, len);
}