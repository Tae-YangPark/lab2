
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
#include <signal.h>
#include <poll.h>
#include <time.h>

#define MSG_SIZE 256

static volatile sig_atomic_t stop_requested = 0;
static volatile sig_atomic_t child_exited = 0;
static pid_t parent_pid;
static int is_child = 0;

static void handle_signal(int signo) {
    if (signo == SIGCHLD)
        child_exited = 1;
    else
        stop_requested = 1;
}

static int peer_exited(void) {
    return is_child ? getppid() != parent_pid : child_exited != 0;
}

static int deadline(struct timespec *until) {
    if (clock_gettime(CLOCK_REALTIME, until) == -1)
        return -1;
    until->tv_nsec += 200000000L;
    if (until->tv_nsec >= 1000000000L) {
        until->tv_sec++;
        until->tv_nsec -= 1000000000L;
    }
    return 0;
}

// 사용자 메시지 입력
static int get_input(const char *prompt, char *buffer) {
    for (;;) {
        printf("%s", prompt);
        fflush(stdout);

        size_t len = 0;
        int too_long = 0;

        for (;;) {
            if (stop_requested || peer_exited())
                return -2;

            struct pollfd input = { .fd = STDIN_FILENO, .events = POLLIN };
            int ready = poll(&input, 1, 200);
            if (ready == -1 && errno == EINTR)
                continue;
            if (ready == -1) {
                perror("poll");
                return -2;
            }
            if (ready == 0)
                continue;
            if (stop_requested || peer_exited())
                return -2;

            char ch;
            ssize_t n = read(STDIN_FILENO, &ch, 1);

            if (n == -1 && errno == EINTR)
                continue;

            if (n < 0) {
                perror("read");
                return -2;
            }
            if (n == 0) {
                if (len > 0 && !too_long)
                    break;
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
    for (;;) {
        if (stop_requested || peer_exited())
            return -1;
        struct timespec until;
        if (deadline(&until) == -1)
            return -1;
        if (mq_timedsend(queue, message, strlen(message) + 1, 0, &until) == 0)
            return 0;
        if (errno == EINTR || errno == ETIMEDOUT)
            continue;

        perror("mq_send");
        return -1;
    }
}

// 메시지 큐에서 수신
static int receive_message(mqd_t queue, char *buffer) {
    ssize_t n;

    for (;;) {
        if (stop_requested)
            return -1;
        struct timespec until;
        if (deadline(&until) == -1)
            return -1;
        n = mq_timedreceive(queue, buffer, MSG_SIZE, NULL, &until);
        if (n >= 0)
            break;
        if (errno == EINTR)
            continue;
        // 상대가 마지막 /quit을 전송한 뒤 종료했을 수 있으므로 먼저 수신한다.
        if (errno == ETIMEDOUT) {
            if (peer_exited())
                return -1;
            continue;
        }
        perror("mq_receive");
        return -1;
    }

    buffer[MSG_SIZE - 1] = '\0';
    return 0;
}

int main(void) {
    char to_child_name[64];
    char to_parent_name[64];

    // 큐가 닫힌 표준입력의 fd 0을 재사용하지 않도록 먼저 확인한다.
    if (fcntl(STDIN_FILENO, F_GETFL) == -1) {
        perror("stdin");
        return 1;
    }

    struct sigaction sa = {0};
    sa.sa_handler = handle_signal;
    sa.sa_flags = SA_NOCLDSTOP;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGINT, &sa, NULL) == -1 ||
        sigaction(SIGTERM, &sa, NULL) == -1 ||
        sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }
    parent_pid = getpid();

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

    // 이미 연 큐는 fork로 상속된다. 이름을 일찍 지워 비정상 종료 시 잔존을 줄인다.
    int unlink_child = mq_unlink(to_child_name);
    int unlink_parent = mq_unlink(to_parent_name);
    if (unlink_child == -1 || unlink_parent == -1) {
        fputs("메시지 큐 이름 삭제 실패\n", stderr);
        mq_close(to_child);
        mq_close(to_parent);
        if (unlink_child == -1)
            mq_unlink(to_child_name);
        if (unlink_parent == -1)
            mq_unlink(to_parent_name);
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
        return 1;
    }

    char message[MSG_SIZE];
    int error = 0;

    if (pid == 0) {
        is_child = 1;
        // 자식 프로세스
        for (;;) {
            if (receive_message(to_child, message) != 0) {
                error = 1;
                break;
            }

            if (strcmp(message, "/quit") == 0)
                break;

            printf("[자식이 받은 메시지] %s\n", message);

            int input = get_input("자식> ", message);
            if (input == -2) {
                error = 1;
                break;
            }
            if (input == -1)
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
        _exit(error || stop_requested);
    }

    // 부모 프로세스
    for (;;) {
        int input = get_input("부모> ", message);
        if (input == -2) {
            error = 1;
            break;
        }
        if (input == -1)
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
    pid_t waited;
    if (error || stop_requested)
        kill(pid, SIGTERM);
    do {
        waited = waitpid(pid, &status, 0);
        if (waited == -1 && errno == EINTR && stop_requested)
            kill(pid, SIGTERM);
    } while (waited == -1 && errno == EINTR);
    if (waited == -1) {
        perror("waitpid");
        error = 1;
    } else if (!WIFEXITED(status) ||
               WEXITSTATUS(status) != 0) {
        error = 1;
    }

    // 메시지 큐 닫기 (이름은 생성 직후 삭제됨)
    if (mq_close(to_child) == -1) {
        perror("mq_close child");
        error = 1;
    }
    if (mq_close(to_parent) == -1) {
        perror("mq_close parent");
        error = 1;
    }

    if (error || stop_requested) {
        puts("메시지 큐 채팅 실패");
        return 1;
    }

    puts("메시지 큐 채팅 종료 및 자원 정리 완료");
    return 0;
}
