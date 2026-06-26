#include"RingBuffer.h"

 
int RingBuffer::GetUsedSize() { return (tail - head + 4096) % 4096; }

int RingBuffer::GetFreeSize() { return 4096 - GetUsedSize() - 1; }

int RingBuffer::GetLinearFreeSize() {
    if (tail >= head) {
        return 4096 - tail;
    }
    else {
        return head - tail - 1;
    }
}
int RingBuffer::GetLinearUsedSize() {
    if (head == tail) {
        return 0;
    }
    else if (head < tail) {
        return tail - head;
    }
    else {
        return 4096 - head;
    }
}

char* RingBuffer::GetWriteBuffer() { return &buffer[tail]; }

char* RingBuffer::GetReadBuffer() { return &buffer[head]; }

void RingBuffer::OnRead(int bytes) { head = (head + bytes) % 4096; }

void RingBuffer::OnWrite(int bytes) { tail = (tail + bytes) % 4096; }

void RingBuffer::Peek(char* dest, int len) {
    if (GetUsedSize() < len) return;
    if (tail >= head) {
        memcpy(dest, &buffer[head], len);
    }
    else {
        int rightSize = 4096 - head;
        if (len <= rightSize) {
            memcpy(dest, &buffer[head], len);
        }
        else {
            memcpy(dest, &buffer[head], rightSize);
            memcpy(dest + rightSize, &buffer[0], len - rightSize);
        }
    }
}

bool RingBuffer::Write(const char* data, int len) {
    if (len > GetFreeSize()) return false;

    int rightSize = 4096 - tail;
    if (len <= rightSize) {
        memcpy(&buffer[tail], data, len);
    }
    else {
        memcpy(&buffer[tail], data, rightSize);
        memcpy(&buffer[0], data + rightSize, len - rightSize);
    }
    OnWrite(len);
    return true;
}

void RingBuffer::Clear() { head = tail = 0; }