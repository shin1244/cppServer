#pragma once
#include <cstring>

class RingBuffer {
private:
    char buffer[4096];
    int head = 0;
    int tail = 0;
public:
    // 현재 버퍼에 쌓인 데이터의 크기를 반환합니다.
    int GetUsedSize();
    // 현재 버퍼에 빈 공간을 반환합니다.
    int GetFreeSize();
    // 현재 버퍼에 연속된 빈 공간을 반환합니다.
    int GetLinearFreeSize();
    //
    int GetLinearUsedSize();
    // 현재 데이터를 새로 적을 수 있는 빈 공간의 시작 주소를 반환합니다.
    char* GetWriteBuffer();
    // 현재 읽을 수 있는 가장 오래된 데이터의 시작 주소를 반환합니다.
    char* GetReadBuffer();
    // 메인 스레드가 패킷 하나를 온전히 조립해서 로직에 반영했다면 head를 전진시킵니다.
    void OnRead(int bytes);
    // OS가 네트워크 카드에서 데이터를 받아 버퍼에 성공적으로 채웠을 때 tail을 전진시킵니다.
    void OnWrite(int bytes);
    // 
    void Peek(char* dest, int len);
    // 
    bool Write(const char* data, int len);
    // 버퍼를 초기화 합니다.
    void Clear();
};