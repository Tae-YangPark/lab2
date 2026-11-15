
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(void) {
    const char *message = "Hello, Linux file I/O!\n";
    char buffer[128] = {0};

    // 1. 파일 생성 및 열기
    int fd = open("sample.txt", O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    // 2. 파일에 내용 쓰기
    size_t len = strlen(message);
    if (write(fd, message, len) != (ssize_t)len) {
        perror("write");
        close(fd);
        return 1;
    }

    // 3. 파일의 시작 위치로 이동
    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("lseek");
        close(fd);
        return 1;
    }

    // 4. 파일에서 내용 읽기
    ssize_t n = read(fd, buffer, sizeof(buffer) - 1);
    if (n == -1) {
        perror("read");
        close(fd);
        return 1;
    }

    buffer[n] = '\0';
    printf("읽은 내용: %s", buffer);

    // 5. 파일 닫기
    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    printf("파일 처리 완료\n");
    return 0;
}
