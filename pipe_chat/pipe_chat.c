
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define SIZE 256

// 한 줄씩 메시지 읽기
static ssize_t read_line(int fd, char *buf, size_t cap) {
    size_t i = 0;
    char ch;
    ssize_t n;
    int too_long = 0;

    for (;;) {
        n = read(fd, &ch, 1);

        if (n == -1 && errno == EINTR)
            continue;
        if (n < 0)
            return -1;
        if (n == 0) {
            if (i == 0 && !too_long)
                return 0;
            break;
        }

        if (ch == '\n')
            break;

        if (i + 1 < cap)
            buf[i++] = ch;
        else
            too_long = 1;
    }

    buf[i] = '\0';

    if (too_long)
        return -2;

    return (ssize_t)i + 1;
}

// 파이프로 메시지 전송
static int send_line(int fd, const char *msg) {
    size_t len = strlen(msg);
    char line[SIZE + 1];

    memcpy(line, msg, len);
    line[len] = '\n';

    size_t pos = 0;

    while (pos < len + 1) {
        ssize_t n = write(fd, line + pos, len + 1 - pos);

        if (n < 0 && errno == EINTR)
            continue;
        if (n <= 0)
            return -1;

        pos += (size_t)n;
    }

    return 0;
}

// 사용자 메시지 입력
static int input_message(const char *prompt, char *msg) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        ssize_t n = read_line(STDIN_FILENO, msg, SIZE);

        if (n == -2) {
            puts("메시지는 255자 이내로 입력하세요.");
            continue;
        }

        return n > 0 ? 0 : -1;
    }
}

int main(void) {
    int parent_to_child[2];
    int child_to_parent[2];

    // 양방향 통신을 위한 파이프 2개 생성
    if (pipe(parent_to_child) == -1) {
        perror("pipe");
        return 1;
    }

    if (pipe(child_to_parent) == -1) {
        perror("pipe");
        close(parent_to_child[0]);
        close(parent_to_child[1]);
        return 1;
    }

    printf("=== 파이프 텍스트 채팅 ===\n");
    printf("종료하려면 /quit 입력\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    char msg[SIZE];

    if (pid == 0) {
        // 자식 프로세스
        close(parent_to_child[1]);
        close(child_to_parent[0]);

        for (;;) {
            ssize_t n = read_line(
                parent_to_child[0], msg, SIZE
            );

            if (n <= 0)
                break;

            if (strcmp(msg, "/quit") == 0)
                break;

            printf("[자식이 받은 메시지] %s\n", msg);

            if (input_message("자식> ", msg) < 0)
                break;

            if (send_line(child_to_parent[1], msg) < 0)
                break;

            if (strcmp(msg, "/quit") == 0)
                break;
        }

        close(parent_to_child[0]);
        close(child_to_parent[1]);
        _exit(0);
    }

    // 부모 프로세스
    close(parent_to_child[0]);
    close(child_to_parent[1]);

    for (;;) {
        if (input_message("부모> ", msg) < 0)
            break;

        if (send_line(parent_to_child[1], msg) < 0) {
            perror("send");
            break;
        }

        if (strcmp(msg, "/quit") == 0)
            break;

        ssize_t n = read_line(
            child_to_parent[0], msg, SIZE
        );

        if (n <= 0)
            break;

        if (strcmp(msg, "/quit") == 0)
            break;

        printf("[부모가 받은 메시지] %s\n", msg);
    }

    close(parent_to_child[1]);
    close(child_to_parent[0]);

    if (waitpid(pid, NULL, 0) == -1)
        perror("waitpid");

    puts("채팅 종료");

    return 0;
}
