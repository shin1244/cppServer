cmake_minimum_required(VERSION 3.16)
project(cppServer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# clangd / VS Code IntelliSense 가 읽는 컴파일 정보
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/cppServer)

# 플랫폼 무관 게임 로직
add_executable(cppServer
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/World.cpp
    ${SRC_DIR}/World_Net.cpp
    ${SRC_DIR}/Player.cpp
    ${SRC_DIR}/Bullet.cpp
    ${SRC_DIR}/Item.cpp
    ${SRC_DIR}/Map.cpp
    ${SRC_DIR}/SpatialGrid.cpp
    ${SRC_DIR}/RingBuffer.cpp
    ${SRC_DIR}/VecterBuffer.cpp
)

target_include_directories(cppServer PRIVATE ${SRC_DIR})

# 네트워크 백엔드: 플랫폼별로 교체
if(WIN32)
    target_sources(cppServer PRIVATE ${SRC_DIR}/NetworkCore.cpp)
    target_link_libraries(cppServer PRIVATE ws2_32 psapi)
else()
    # epoll 백엔드 작성 후 아래 주석 해제
    # target_sources(cppServer PRIVATE ${SRC_DIR}/NetworkCore_epoll.cpp)
    find_package(Threads REQUIRED)
    target_link_libraries(cppServer PRIVATE Threads::Threads)
endif()

if(MSVC)
    target_compile_options(cppServer PRIVATE /W3)
    # 일부 소스가 아직 CP949 라 /utf-8 은 전체 변환 후에 켤 것
else()
    target_compile_options(cppServer PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()
