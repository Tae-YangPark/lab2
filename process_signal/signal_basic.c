
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

volatile sig_atomic_t signal_received = 0;

// 시그널 처리 함수
void signal_handler(int signo) {
    if (signo == SIGUSR1) {
        signal_received = 1;
    }
}

int main(void) {
    struct sigaction sa = {0};

    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    // SIGUSR1 처리 함수 등록
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("=== 시그널 처리 실습 ===\n");
    printf("현재 프로세스 PID: %ld\n", (long)getpid());

    // 현재 프로세스에 SIGUSR1 전송
    printf("SIGUSR1 시그널 전송\n");

    if (kill(getpid(), SIGUSR1) == -1) {
        perror("kill");
        return 1;
    }

    if (signal_received) {
        printf("SIGUSR1 시그널 수신 및 처리 성공\n");
    } else {
        printf("시그널 처리 실패\n");
        return 1;
    }

    printf("프로그램 정상 종료\n");
    return 0;
}
