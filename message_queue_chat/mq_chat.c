
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define MSG_SIZE 256

// 사용자 메시지 입력
static int get_input(const char *prompt, char *buffer) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        size_t len = 0;
        int too_long = 0;

        for (;;) {
            char ch;
            ssize_t n = read(STDIN_FILENO, &ch, 1);

            if (n == -1 && errno == EINTR)
                continue;

            if (n <= 0) {
                if (n < 0)
                    perror("read");
                return -1;
            }

            if (ch == '\n')
                break;

            if (len + 1 < MSG_SIZE)
                buffer[len++] = ch;
            else
                too_long = 1;
        }

        if (too_long) {
            puts("메시지는 255자 이하로 입력하세요.");
            continue;
        }

        buffer[len] = '\0';
        return 0;
    }
}

// 메시지 큐로 전송
static int send_message(mqd_t queue, const char *message) {
    while (mq_send(queue, message,
                   strlen(message) + 1, 0) == -1) {
        if (errno == EINTR)
            continue;

        perror("mq_send");
        return -1;
    }

    return 0;
}

// 메시지 큐에서 수신
static int receive_message(mqd_t queue, char *buffer) {
    ssize_t n;

    do {
        n = mq_receive(queue, buffer, MSG_SIZE, NULL);
    } while (n == -1 && errno == EINTR);

    if (n < 0) {
        perror("mq_receive");
        return -1;
    }

    buffer[MSG_SIZE - 1] = '\0';
    return 0;
}

int main(void) {
    char to_child_name[64];
    char to_parent_name[64];

    // 고유한 메시지 큐 이름 생성
    snprintf(to_child_name, sizeof(to_child_name),
             "/lab2_child_%ld", (long)getpid());

    snprintf(to_parent_name, sizeof(to_parent_name),
             "/lab2_parent_%ld", (long)getpid());

    struct mq_attr attr = {0};
    attr.mq_maxmsg = 10;
    attr.mq_msgsize = MSG_SIZE;

    // 메시지 큐 2개 생성
    mqd_t to_child = mq_open(
        to_child_name, O_CREAT | O_EXCL | O_RDWR,
        0600, &attr
    );

    if (to_child == (mqd_t)-1) {
        perror("mq_open child");
        return 1;
    }

    mqd_t to_parent = mq_open(
        to_parent_name, O_CREAT | O_EXCL | O_RDWR,
        0600, &attr
    );

    if (to_parent == (mqd_t)-1) {
        perror("mq_open parent");
        mq_close(to_child);
        mq_unlink(to_child_name);
        return 1;
    }

    printf("=== POSIX 메시지 큐 텍스트 채팅 ===\n");
    printf("종료하려면 /quit 입력\n");
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        mq_close(to_child);
        mq_close(to_parent);
        mq_unlink(to_child_name);
        mq_unlink(to_parent_name);
        return 1;
    }

    char message[MSG_SIZE];
    int error = 0;

    if (pid == 0) {
        // 자식 프로세스
        for (;;) {
            if (receive_message(to_child, message) != 0) {
                error = 1;
                break;
            }

            if (strcmp(message, "/quit") == 0)
                break;

            printf("[자식이 받은 메시지] %s\n", message);

            if (get_input("자식> ", message) != 0)
                strcpy(message, "/quit");

            if (send_message(to_parent, message) != 0) {
                error = 1;
                break;
            }

            if (strcmp(message, "/quit") == 0)
                break;
        }

        mq_close(to_child);
        mq_close(to_parent);
        _exit(error);
    }

    // 부모 프로세스
    for (;;) {
        if (get_input("부모> ", message) != 0)
            strcpy(message, "/quit");

        if (send_message(to_child, message) != 0) {
            error = 1;
            break;
        }

        if (strcmp(message, "/quit") == 0)
            break;

        if (receive_message(to_parent, message) != 0) {
            error = 1;
            break;
        }

        if (strcmp(message, "/quit") == 0)
            break;

        printf("[부모가 받은 메시지] %s\n", message);
    }

    // 자식 프로세스 종료 확인
    int status;

    if (waitpid(pid, &status, 0) == -1) {
        perror("waitpid");
        error = 1;
    } else if (!WIFEXITED(status) ||
               WEXITSTATUS(status) != 0) {
        error = 1;
    }

    // 메시지 큐 닫기 및 삭제
    if (mq_close(to_child) == -1) {
        perror("mq_close child");
        error = 1;
    }
    if (mq_close(to_parent) == -1) {
        perror("mq_close parent");
        error = 1;
    }
    if (mq_unlink(to_child_name) == -1) {
        perror("mq_unlink child");
        error = 1;
    }
    if (mq_unlink(to_parent_name) == -1) {
        perror("mq_unlink parent");
        error = 1;
    }

    if (error) {
        puts("메시지 큐 채팅 실패");
        return 1;
    }

    puts("메시지 큐 채팅 종료 및 자원 정리 완료");
    return 0;
}
