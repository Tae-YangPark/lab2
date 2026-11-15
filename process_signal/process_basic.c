
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    printf("=== 프로세스 생성 실습 ===\n");
    printf("부모 PID: %ld\n", (long)getpid());
    fflush(stdout);

    // 자식 프로세스 생성
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // 자식 프로세스
        printf("[자식] PID: %ld\n", (long)getpid());
        printf("[자식] 부모 PID: %ld\n", (long)getppid());
        printf("[자식] 종료 코드 7로 종료\n");
        fflush(stdout);
        _exit(7);
    }

    // 부모 프로세스
    printf("[부모] 생성한 자식 PID: %ld\n", (long)pid);

    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        return 1;
    }

    if (WIFEXITED(status)) {
        printf("[부모] 자식 종료 코드: %d\n",
               WEXITSTATUS(status));
    }

    printf("[부모] 프로그램 종료\n");

    return 0;
}
