#include"main.h"

int main() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    std::cout << "Winsock ready\n";

    g_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (g_iocp == NULL) {
        std::cout << "CreateIoCompletionPort failed: " << GetLastError() << "\n";
        return 1;
    }
    std::cout << "IOCP created\n";

    unsigned int n = std::thread::hardware_concurrency() - 1;
    std::cout << "spawning " << n << " worker threads\n";

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "socket failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(5050);
    if (bind(listenSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cout << "bind failed: " << WSAGetLastError() << "\n";
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cout << "listen failed: " << WSAGetLastError() << "\n";
        return 1;
    }
    std::cout << "listening on port 5050...\n";

    // IOCP 워커 쓰레드(코어 - 1)와 접속 쓰레드 생성
    for (unsigned int i = 0; i < n; i++)
        std::thread(workerThread).detach();
    std::thread (Accepter, listenSocket).detach();

    constexpr int   TICK_MS = 33;    
    constexpr float TICK_DT = TICK_MS / 1000.0f;

    World world;
    world.Init();

    // -- 메인 루프 시작 --

    std::vector<RecvPacket> buffer;
    TickBenchmark bench; // 벤치마크
    auto lastReportTime = std::chrono::steady_clock::now(); // 마지막 결과 출력 시간

    while (true) {
        auto tickStart = std::chrono::steady_clock::now();

        buffer.clear();
        g_recvQueue.Swap(buffer);
        for (auto& packet : buffer) {
            world.HandlePacket(packet);
        }
        world.Update(TICK_DT);

        auto tickEnd = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(tickEnd - tickStart).count();
        
        bench.tickCount++;
        bench.totalTime += duration;
        if (duration > bench.maxTime) bench.maxTime = duration;
        if (duration < bench.minTime) bench.minTime = duration;
        
        if (tickStart - lastReportTime >= std::chrono::seconds(3)) { // 3초마다 체크
            double avgMs = (bench.totalTime / (double)bench.tickCount) / 1'000'000.0;
            double maxMs = bench.maxTime / 1'000'000.0;
            double minMs = bench.minTime / 1'000'000.0;

            PROCESS_MEMORY_COUNTERS pmc;
            double currentMemMb = 0.0;
            double peakMemMb = 0.0;

            if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
                // Byte 단위를 MB 단위로 변환 (1024 * 1024)
                currentMemMb = pmc.WorkingSetSize / (1024 * 1024);
                peakMemMb = pmc.PeakWorkingSetSize / (1024 * 1024);
            }

            std::cout << "[Benchmark] -------------------------\n"
                << " - FPS Ticks (3s): " << bench.tickCount << "\n"
                << " - Avg Tick Time  : " << avgMs << " ms\n"
                << " - Max Tick Time  : " << maxMs << " ms\n"
                << " - Min Tick Time  : " << minMs << " ms\n"
                << " - Memory Usage   : " << currentMemMb << " MB (Peak: " << peakMemMb << " MB)\n"
                << "-------------------------------------\n" << std::endl;

            bench.reset();
            lastReportTime = tickStart;
        }
        
        std::this_thread::sleep_until(tickStart + std::chrono::milliseconds(TICK_MS));
    }
    
    WSACleanup();
    return 0;
}