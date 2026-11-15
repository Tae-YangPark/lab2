
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
    const char *fifo = "lab2_demo.fifo";
    const char *message = "Hello from child via FIFO!";

    printf("=== FIFO 프로세스 간 통신 실습 ===\n");
    fflush(stdout);

    // FIFO 파일 생성
    if (mkfifo(fifo, 0600) == -1) {
        perror("mkfifo");
        return 1;
    }

    // 자식 프로세스 생성
    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        unlink(fifo);
        return 1;
    }

    if (pid == 0) {
        // 자식: 메시지 전송
        int fd = open(fifo, O_WRONLY);
        if (fd == -1) {
            perror("child open");
            _exit(1);
        }

        size_t len = strlen(message);

        if (write(fd, message, len) != (ssize_t)len) {
            perror("write");
            close(fd);
            _exit(1);
        }

        close(fd);
        _exit(0);
    }

    // 부모: 메시지 수신
    int fd = open(fifo, O_RDONLY);
    char buffer[128] = {0};
    ssize_t n = -1;

    if (fd != -1) {
        n = read(fd, buffer, sizeof(buffer) - 1);

        if (n > 0) {
            buffer[n] = '\0';
            printf("[부모] 수신 메시지: %s\n", buffer);
        } else {
            perror("read");
        }

        close(fd);
    } else {
        perror("parent open");
    }

    // 자식 종료 대기
    int status;
    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        unlink(fifo);
        return 1;
    }

    // FIFO 파일 정리
    if (unlink(fifo) == -1) {
        perror("unlink");
        return 1;
    }

    if (n <= 0 || !WIFEXITED(status) ||
        WEXITSTATUS(status) != 0) {
        printf("IPC 통신 실패\n");
        return 1;
    }

    printf("IPC 통신 성공\n");
    printf("FIFO 파일 정리 완료\n");

    return 0;
}
