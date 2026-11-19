
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <time.h>

// 두 문장의 편집 거리를 이용해 오타 수 계산
size_t count_errors(const char *correct, const char *typed) {
    size_t a = strlen(correct);
    size_t b = strlen(typed);

    size_t previous[b + 1];
    size_t current[b + 1];

    for (size_t j = 0; j <= b; j++) {
        previous[j] = j;
    }

    for (size_t i = 1; i <= a; i++) {
        current[0] = i;

        for (size_t j = 1; j <= b; j++) {
            size_t del = previous[j] + 1;
            size_t ins = current[j - 1] + 1;
            size_t sub = previous[j - 1] +
                (correct[i - 1] != typed[j - 1]);

            size_t min = del < ins ? del : ins;
            current[j] = min < sub ? min : sub;
        }

        for (size_t j = 0; j <= b; j++) {
            previous[j] = current[j];
        }
    }

    return previous[b];
}

int main(void) {
    const char *sentences[] = {
        "Linux is an open source operating system.",
        "Rust provides memory safety.",
        "Processes communicate using pipes."
    };

    const int count = sizeof(sentences) / sizeof(sentences[0]);
    char input[512];

    size_t total_chars = 0;
    size_t total_errors = 0;

    struct timespec start, end;

    printf("=== Linux 타자 연습 프로그램 ===\n");
    printf("제시된 문장을 정확하게 입력하세요.\n\n");

    // 시간 측정 시작
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0) {
        perror("clock_gettime");
        return 1;
    }

    for (int i = 0; i < count; i++) {
        printf("문장 %d: %s\n", i + 1, sentences[i]);
        for (;;) {
            printf("입력: ");
            fflush(stdout);

            if (fgets(input, sizeof(input), stdin) == NULL) {
                printf("\n입력이 종료되었습니다.\n");
                return 1;
            }

            char *newline = strchr(input, '\n');
            if (newline != NULL) {
                *newline = '\0';
                break;
            }

            // 511바이트 입력 직후의 개행/EOF는 허용한다.
            int ch = getchar();
            if (ch == '\n' || (ch == EOF && !ferror(stdin)))
                break;

            while (ch != '\n' && ch != EOF)
                ch = getchar();
            if (ferror(stdin)) {
                perror("입력");
                return 1;
            }
            puts("입력이 너무 깁니다. 511바이트 이하로 다시 입력하세요.");
        }

        // 편집 거리를 이용한 오타 수 계산
        size_t errors = count_errors(sentences[i], input);

        total_errors += errors;
        total_chars += strlen(input);

        printf("오타 수: %zu\n\n", errors);
    }

    // 시간 측정 종료
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0) {
        perror("clock_gettime");
        return 1;
    }

    double elapsed =
        (end.tv_sec - start.tv_sec) +
        (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    if (elapsed <= 0.0) {
        printf("측정 시간이 너무 짧습니다.\n");
        return 1;
    }

    // 분당 타자 수 계산
    double cpm = total_chars * 60.0 / elapsed;

    printf("=== 타자 연습 결과 ===\n");
    printf("입력한 문장 수: %d\n", count);
    printf("총 입력 타자 수: %zu\n", total_chars);
    printf("전체 오타 수: %zu\n", total_errors);
    printf("소요 시간: %.2f초\n", elapsed);
    printf("평균 분당 타자 수: %.2f 타/분\n", cpm);

    return 0;
}
