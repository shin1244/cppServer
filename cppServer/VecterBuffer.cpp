#include"VecterBuffer.h"

int VectorBuffer::GetUsedSize() {
    return tail - head;
}

int VectorBuffer::GetFreeSize() {
    return static_cast<int>(buffer.size()) - tail;
}

int VectorBuffer::GetLinearUsedSize() {
    return GetUsedSize();
}

int VectorBuffer::GetLinearFreeSize() {
    return GetFreeSize();
}

char* VectorBuffer::GetWriteBuffer() {
    return buffer.data() + tail;
}

char* VectorBuffer::GetReadBuffer() {
    return buffer.data() + head;
}

void VectorBuffer::OnRead(int byte) {
    head += byte;
    if (head == tail) Clear();
}

void VectorBuffer::OnWrite(int byte) {
    tail += byte;
}

void VectorBuffer::Peek(char* dest, int len) {
    memcpy(dest, buffer.data() + head, len);
}

bool VectorBuffer::Write(const char* data, int len) {
    Reserve(len);
    memcpy(buffer.data() + tail, data, len);
    tail += len;
    return true;
}

void VectorBuffer::Clear() {
    head = 0;
    tail = 0;
}

void VectorBuffer::Reserve(int len) {
    if (GetLinearFreeSize() >= len) return;

    int used = GetUsedSize();
    memmove(buffer.data(), buffer.data() + head, used);
    head = 0;
    tail = used;

    if (GetLinearFreeSize() < len) buffer.resize(tail + len);
}