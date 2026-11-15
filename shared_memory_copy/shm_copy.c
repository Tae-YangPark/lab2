
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

static int read_full(int fd, char *buffer, size_t size) {
    size_t done = 0;
    while (done < size) {
        size_t count = size - done;
        if (count > 65536) count = 65536;
        ssize_t n = read(fd, buffer + done, count);
        if (n < 0 && errno == EINTR) continue;
        if (n < 0) { perror("read"); return -1; }
        if (n == 0) {
            fprintf(stderr, "원본 파일이 읽는 중 변경되었습니다.\n");
            return -1;
        }
        done += (size_t)n;
    }
    return 0;
}

static int write_full(int fd, const char *buffer, size_t size) {
    size_t done = 0;
    while (done < size) {
        size_t count = size - done;
        if (count > 65536) count = 65536;
        ssize_t n = write(fd, buffer + done, count);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { perror("write"); return -1; }
        done += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "사용법: %s 원본파일 대상파일\n", argv[0]);
        return 1;
    }

    int source = open(argv[1], O_RDONLY);
    if (source < 0) { perror("원본 open"); return 1; }

    struct stat st;
    if (fstat(source, &st) < 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "원본은 읽을 수 있는 일반 파일이어야 합니다.\n");
        close(source);
        return 1;
    }

    struct stat dst;
    if (stat(argv[2], &dst) == 0) {
        if (dst.st_dev == st.st_dev && dst.st_ino == st.st_ino) {
            fprintf(stderr, "원본과 대상 파일이 동일합니다.\n");
            close(source);
            return 1;
        }
    } else if (errno != ENOENT) {
        perror("대상 stat");
        close(source);
        return 1;
    }

    if (st.st_size < 0 || (uintmax_t)st.st_size > SIZE_MAX) {
        fprintf(stderr, "파일 크기가 너무 큽니다.\n");
        close(source);
        return 1;
    }

    size_t size = (size_t)st.st_size;
    if (size == 0) {
        int target = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (target < 0) {
            perror("대상 open");
            close(source);
            return 1;
        }
        if (close(target) < 0) {
            perror("대상 close");
            close(source);
            return 1;
        }
        close(source);
        puts("빈 파일 복사 완료 (0바이트)");
        return 0;
    }

    char shm_name[64];
    snprintf(shm_name, sizeof(shm_name),
             "/lab2_copy_%ld", (long)getpid());

    int shm_fd = shm_open(shm_name,
                         O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd < 0) {
        perror("shm_open");
        close(source);
        return 1;
    }

    int result = 1;
    void *shared = MAP_FAILED;

    if (ftruncate(shm_fd, st.st_size) < 0) {
        perror("ftruncate");
        goto cleanup;
    }

    shared = mmap(NULL, size, PROT_READ | PROT_WRITE,
                  MAP_SHARED, shm_fd, 0);
    if (shared == MAP_FAILED) {
        perror("mmap");
        goto cleanup;
    }

    // 부모가 원본을 공유 메모리에 기록
    if (read_full(source, shared, size) < 0)
        goto cleanup;

    if (close(source) < 0) {
        perror("원본 close");
        source = -1;
        goto cleanup;
    }
    source = -1;

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        goto cleanup;
    }

    if (pid == 0) {
        // 자식이 공유 메모리에서 대상 파일로 복사
        int target = open(argv[2],
                          O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (target < 0) {
            perror("대상 open");
            _exit(1);
        }

        int ok = write_full(target, shared, size);
        if (close(target) < 0) {
            perror("대상 close");
            ok = -1;
        }
        _exit(ok == 0 ? 0 : 1);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        goto cleanup;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "자식 프로세스 파일 복사 실패\n");
        goto cleanup;
    }

    printf("부모가 공유 메모리에 %zu바이트 기록\n", size);
    printf("자식이 대상 파일에 %zu바이트 저장\n", size);
    puts("POSIX 공유 메모리 파일 복사 성공");
    result = 0;

cleanup:
    if (source >= 0) close(source);
    if (shared != MAP_FAILED && munmap(shared, size) < 0) {
        perror("munmap");
        result = 1;
    }
    if (close(shm_fd) < 0) {
        perror("shm close");
        result = 1;
    }
    if (shm_unlink(shm_name) < 0) {
        perror("shm_unlink");
        result = 1;
    }
    return result;
}
