# IOCP 기반 실시간 대전 게임 서버 포트폴리오 초안

## 전체 방향

- 형식: 16:9 가로형 PPT, 약 10장 ~ 15장
- 톤: 발표자료처럼 한 장에 한 메시지
- 중심 문장: "실시간 게임 서버에서 중요한 설계 선택지를 직접 구현하고, 부하 상황에서 비교한 프로젝트"
- 강조점:
  - 힙 메모리 최소화와 예측 가능한 메모리 사용
  - 캐시 친화적인 고정 배열/풀 기반 데이터 배치
  - 무작정 멀티스레드화하지 않고 IOCP 워커와 단일 로직 스레드를 분리한 구조
  - 큐, 버퍼, AOI처럼 서버 성능에 직접 영향을 주는 설계 선택지 비교

---

## 1. 표지

### 제목

IOCP 기반 실시간 대전 게임 서버

### 부제

C++17과 Windows IOCP로 구현한 400명 규모 부하 대응형 top-down 슈팅 서버

### 핵심 한 줄

네트워크 I/O, 메모리 사용, AOI 탐색 구조를 직접 구현하고 비교하며 서버 설계의 병목과 안정성을 검증했습니다.

### 시각 자료

- 서버 흐름을 간단히 보여주는 큰 다이어그램
- `IOCP`, `30Hz Tick`, `400 CCU Load`, `AOI Grid` 같은 큰 키워드 배치

---

## 2. 프로젝트 개요

### 메시지

단순히 동작하는 서버가 아니라, 실시간 게임 서버의 핵심 설계 선택지를 실험하기 위한 프로젝트입니다.

### 내용

- C++17 기반 Windows IOCP 서버
- Go 렌더링 클라이언트와 Go 부하 봇 구현
- 방 2개, 방당 최대 200명, 총 400명 부하 구조
- 이동, 사격, 벽 파괴, 피격 후 관전, LOS + 반경 기반 AOI 전송

### 코드 근거

- `cppServer/main.cpp`: IOCP 생성, 워커 스레드 32개, 33ms 로직 루프
- `cppServer/World.h`: `MAX_PLAYER = 200`, `MAX_BULLETS = 4096`
- `client/main.go`: 렌더링 클라이언트
- `client/bot/main.go`: 부하 봇, 악성 봇

---

## 3. 중점 1 - 예측 가능한 메모리와 캐시 친화성

### 메시지

힙 할당을 최소화하고, 고정 크기 배열과 풀을 사용해 서버가 부하 상황에서도 예측 가능한 메모리 패턴을 갖도록 설계했습니다.

### 내용

- 세션은 `ObjectPool<Session, 1000>`로 미리 확보한 배열에서 할당/반납
- 월드 내부 플레이어 슬롯과 총알도 고정 배열로 관리
- 총알은 생성/삭제 대신 `active` 플래그로 재사용
- 세션, 플레이어, 총알처럼 자주 접근하는 데이터는 포인터 기반 동적 생성보다 연속된 저장 구조에 가깝게 배치

### 코드 근거

- `cppServer/ObjectPool.h`: `T items[N]`, `freeList`
- `cppServer/NetworkCore.cpp`: `ObjectPool<Session, 1000> g_sessions`
- `cppServer/World.h`: `PlayerSlot slots[MAX_PLAYER]`, `Bullet bullets[MAX_BULLETS]`
- `cppServer/Bullet.cpp`: `Fire`, `Clear`, `IsActive`

### 표현 후보

> 서버에서 중요한 것은 평균 속도뿐 아니라, 부하가 걸렸을 때 메모리 사용량과 접근 패턴이 예측 가능하게 유지되는 것입니다.

---

## 4. 중점 1 - 메인 스레드와 보조 스레드 분리

### 메시지

모든 로직을 멀티스레드로 나누기보다, 네트워크 I/O와 게임 상태 갱신의 책임을 분리했습니다.

### 내용

- IOCP 워커 스레드 32개: 수신 완료 처리, 패킷 프레이밍, 큐 push
- 단일 로직 스레드: 33ms마다 입력 큐 소비, 월드 업데이트, 충돌, AOI 전송
- 게임 상태는 단일 로직 스레드가 소유하므로 플레이어/총알/맵 상태에 별도 락이 필요 없음
- 락은 세션 송신 버퍼, 큐 push/swap 등 경계 지점에 집중

### 코드 근거

- `cppServer/main.cpp`: 워커 스레드 생성, `TICK_MS = 33`, `World.Update`
- `cppServer/NetworkCore.cpp`: `GetQueuedCompletionStatus`, `WSARecv`, 패킷 파싱
- `cppServer/World.cpp`: `UpdatePlayers`, `UpdateBullets`, `Collision`, `SendAOIUpdates`

### 시각 자료

```text
IOCP Worker x32       Logic Thread x1
recv/parse/push  -->  consume/update/send
```

---

## 5. 서버 실행 플로우

### 메시지

서버는 시작 시 네트워크/월드 기반을 준비하고, 게임이 시작되면 33ms 틱마다 입력 처리, 상태 검증, 월드 갱신, AOI 송신을 반복합니다.

### 구성 방식

이 페이지는 긴 문장보다 플로우차트 중심으로 구성합니다.

왼쪽: 서버 시작과 방 준비  
오른쪽: 게임 시작 후 매 틱 반복

### 플로우

```text
[서버 시작]
     |
     v
[Winsock 초기화]
     |
     v
[IOCP 생성]
     |
     v
[Listen Socket 생성 / bind / listen]
     |
     v
[IOCP Worker x32 + Accept Thread 시작]
     |
     v
[World 4개 생성]
     |
     v
[Map 생성]
  - 랜덤 벽 배치
  - 연결성 보장
  - 유저 스폰 좌표 설정
     |
     v
[방당 100명 대기]
     |
     v
[게임 시작]
```

```text
[33ms Logic Tick]
     |
     v
[수신 패킷 소비]
  - Join / Leave
  - Move 입력
  - Attack 요청
  - Observe 요청
     |
     v
[패킷 핸들링]
  - 클라이언트는 좌표가 아닌 입력 키만 전송
  - 공격 요청도 서버가 쿨타임 검증
     |
     v
[World.Update]
  1. 유저 이동 가능 여부 판단
  2. 총알 이동 / 벽 충돌 / 벽 파괴 판단
  3. Grid 업데이트
  4. 유저-총알 충돌 판단
  5. 게임 종료 여부 판단
  6. Grid + LOS + 반경 기준으로 현재 상황 송신
```

### 핵심 표현

> 클라이언트는 "내 위치는 여기"라고 말하지 않고, "이 방향키를 누르고 있다"만 보냅니다. 서버가 이동 가능 여부, 공격 쿨타임, 충돌, 시야 범위를 모두 판단해 최종 상태를 결정합니다.

### 코드 근거

- `cppServer/main.cpp`: Winsock/IOCP 초기화, listen, worker 생성, `World` 생성, 33ms 루프
- `cppServer/NetworkCore.cpp`: accept 후 세션을 IOCP에 바인딩, 수신 패킷 큐 push
- `cppServer/Map.cpp`: 맵 생성, 연결성 검사, 스폰 좌표 생성
- `cppServer/World_Net.cpp`: `HandleMove`, `HandleAttack`, `HandleJoin`, `HandleObserve`
- `cppServer/World.cpp`: `UpdatePlayers`, `UpdateBullets`, `UpdateGrid`, `Collision`, `CheckMatchEnd`, `SendAOIUpdates`

### 디자인 메모

- 실제 PDF에서는 텍스트 박스 12개를 모두 넣기보다, 큰 박스 6~8개로 줄이고 보조 설명은 작은 캡션으로 처리
- `Client Input Only`, `Server Authority`, `33ms Tick`, `AOI Filtered Send`를 강조 라벨로 사용

---

## 6. 실험 1 - 이벤트 큐 vs 더블 스왑 버퍼

### 메시지

락을 매번 잡는 이벤트 큐와, 틱당 한 번 교체하는 더블 스왑 버퍼를 비교했습니다.

### 내용

- 이벤트 큐: push/pop마다 mutex
- 더블 스왑: 워커는 back buffer에 push, 로직은 틱당 1회 swap
- 전체 틱 시간에서는 차이가 묻혔지만, 큐 소비 시간만 분리하자 더블 스왑이 약 2~3배 빠름
- 성능 최적화는 구현보다 "무엇을 측정할지"가 중요하다는 결론

### 코드 근거

- `cppServer/EventQueue.h`
- `cppServer/DoubleBuffer.h`
- `cppServer/main.cpp`: `bench.totalConsumeTime`

---

## 7. 실험 2 - 링 버퍼 vs 가변 버퍼

### 메시지

정상 부하에서는 두 방식이 비슷하지만, backpressure 상황에서는 실패 방식이 완전히 달라집니다.

### 내용

- 링 버퍼: 고정 크기, 메모리 상한이 명확함
- 가변 버퍼: 필요한 만큼 증가하지만 악성 패킷에 취약
- 악성 봇은 완성되지 않는 큰 패킷을 천천히 보내 수신 버퍼를 압박
- 이 실험은 "무엇이 더 빠른가"보다 "최악의 상황에서 어떻게 무너지는가"를 확인하는 데 목적이 있음

### 코드 근거

- `cppServer/RingBuffer.*`
- `cppServer/VecterBuffer.*`
- `client/bot/main.go`: `runMaliciousBot`

---

## 8. 실험 3 - 전체 순회 AOI vs 공간 그리드

### 메시지

AOI 최적화의 핵심은 단순한 속도 개선이 아니라, 플레이어 수 증가에 대한 확장성입니다.

### 내용

- 전체 순회: 플레이어 O(N^2), 총알 O(N*M)
- 공간 그리드: 주변 셀 후보만 수집해 O(N*k)에 가깝게 축소
- 후보 수집 이후 거리/LOS 필터는 동일하게 유지해 결과 일관성 확보
- 유저가 넓게 분산될수록 그리드 방식의 이점이 커짐

### 코드 근거

- `cppServer/SpatialGrid.*`
- `cppServer/World.cpp`: `UpdateGrid`, `SendAOIUpdates`
- `cppServer/Map.cpp`: `HasLineOfSight`

---

## 9. 게임 로직과 네트워크 프로토콜

### 메시지

서버는 입력만 받고, 위치/충돌/피격/관전 같은 주요 상태는 서버가 결정합니다.

### 내용

- 클라이언트는 이동 키와 공격 목표 좌표를 전송
- 서버는 플레이어 이동, 벽 충돌, 총알 이동, 피격 판정 처리
- 벽 파괴, 플레이어 제거, 총알 제거, 관전 대상 변경을 패킷으로 브로드캐스트
- 패킷은 `[size | id | body]` 형태의 작은 바이너리 프로토콜

### 코드 근거

- `cppServer/Protocol.h`
- `cppServer/World_Net.cpp`
- `cppServer/World.cpp`

---

## 10. 부하 테스트와 관찰 지표

### 메시지

렌더링 클라이언트와 별도로 헤드리스 봇을 구현해 서버 부하를 재현했습니다.

### 내용

- Go bot으로 다수의 가상 플레이어 접속
- 이동/사격 패킷을 주기적으로 전송
- 악성 모드로 미완성 대형 패킷을 보내 수신 버퍼 압박 테스트
- 서버는 120초 동안 평균 틱, 최고 틱, 최소 틱, 큐 소비 시간, 메모리 사용량 측정

### 코드 근거

- `client/bot/main.go`
- `cppServer/main.cpp`: `TickBenchmark`, `GetProcessMemoryInfo`

---

## 11. 결과와 배운 점

### 메시지

이 프로젝트의 핵심 성과는 IOCP 서버 구현 자체보다, 서버 설계 선택지를 직접 비교하며 병목과 실패 조건을 확인한 것입니다.

### 정리 문장

- 메모리 배치는 평균 성능뿐 아니라 서버의 예측 가능성을 만든다.
- 멀티스레드는 많이 쓰는 것이 아니라, 상태 소유권을 명확히 나누는 것이 중요하다.
- 큐 최적화는 전체 틱 시간이 아니라 소비 시간처럼 정확한 지표로 봐야 한다.
- 버퍼 설계는 정상 부하보다 악성/역압 상황에서 차이가 드러난다.
- AOI는 알고리즘 복잡도를 바꾸는 순간 확장성이 달라진다.

### 추가하면 좋은 자료

- 실제 벤치마크 수치 표
- 더블 스왑 vs 이벤트 큐 그래프
- 링 버퍼 vs 가변 버퍼 메모리 그래프
- 전체 순회 vs 그리드 AOI 틱 시간 그래프
- Go 클라이언트 실행 화면 캡처
