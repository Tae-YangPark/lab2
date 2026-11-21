# 오픈소스프로젝트 Lab2

동의대학교 컴퓨터소프트웨어공학과 「오픈소스프로젝트」 Lab2 실습 과제입니다.

## 1. 실습 환경

- 운영체제: Ubuntu 24.04 LTS (VMware Workstation)
- 개발 언어: C, Rust
- 개발 도구: GCC, Make, Git, Cargo

### 빌드

Ubuntu에서 D-Bus 실습을 빌드하려면 `sudo apt install libsystemd-dev`를 실행합니다. Rust 실습에는 Rust/Cargo 1.85 이상이 필요합니다.
Ubuntu 24.04에서 기본 Rust 버전이 낮다면 `sudo apt install rustc-1.85 cargo-1.85`로 설치하고, 빌드 터미널에서 `export RUSTC=rustc-1.85 CARGO=cargo-1.85`를 실행합니다.

| 명령 | 동작 |
|---|---|
| `make` | C 실습 전체와 Rust 행렬 프로그램 빌드 |
| `make c` | C 실습만 빌드 (`bin/<실습 폴더>/<프로그램명>`) |
| `make rust` | Rust 행렬 프로그램만 빌드 (`rust_matrix/target/debug/rust_matrix`) |
| `make clean` | C와 Rust 빌드 산출물 정리 |

## 2. 실습 내용

| 실습 | 폴더 | 구현 내용 |
|---|---|---|
| 2 | rust_matrix | Rust Vec을 이용한 동적 행렬 덧셈 |
| 3 | file_processing | 파일 및 디렉터리 관련 함수 실습 |
| 4 | recursive_dir | 재귀적 디렉터리 탐색 프로그램 |
| 5 | typing_practice | 오타 횟수 및 평균 분당 타자수 측정 |
| 6 | process_signal | 프로세스 생성 및 시그널 처리 |
| 7 | ipc_basic | FIFO를 이용한 프로세스 간 통신 |
| 8 | pipe_chat | 파이프를 이용한 텍스트 채팅 |
| 9 | message_queue_chat | POSIX 메시지 큐를 이용한 텍스트 채팅 |
| 10 | shared_memory_copy | POSIX 공유 메모리와 mmap을 이용한 파일 복사 |
| 11 | dbus_notification | D-Bus 알림 메시지 서버·클라이언트 |

## 3. 진행 현황

- 개인 실습 2~11번: 구현 및 기본 실행 검증 완료
- 쉘 프로그램 팀과제(12번): lab2-shell 저장소에서 별도로 진행하며, 11월 30일 제출 예정
