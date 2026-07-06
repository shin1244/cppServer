#pragma once

#include <thread>
#include <vector>
#include <iostream>
#include <unordered_map>
#include <stack>
#include <atomic>
#include <chrono>
#include"RingBuffer.h"
#include"DoubleBuffer.h"
#include"Protocol.h"
#include"NetworkCore.h"
#include"World.h"

#define PSAPI_VERSION 1
#include <psapi.h>
#pragma comment(lib, "psapi.lib")

HANDLE g_iocp;   // IOCP วฺต้

struct TickBenchmark {
	long long totalTime = 0;
	long long maxTime = 0;
	long long minTime = LLONG_MAX;
	int tickCount = 0;

	void reset() {
		totalTime = 0;
		maxTime = 0;
		minTime = LLONG_MAX;
		tickCount = 0;
	}
};