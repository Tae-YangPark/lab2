#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

static int first_dir = 1;

// 숨김 파일과 . 및 .. 제외
int visible(const struct dirent *entry) {
    return entry->d_name[0] != '.';
}

// 디렉터리 재귀 탐색
int list_recursive(const char *path) {
    struct dirent **entries;
    int count = scandir(path, &entries, visible, alphasort);

    if (count < 0) {
        perror(path);
        return 1;
    }

    if (!first_dir) {
        printf("\n");
    }
    first_dir = 0;

    printf("%s:\n", path);

    // 현재 디렉터리 항목 출력
    for (int i = 0; i < count; i++) {
        printf("%s\n", entries[i]->d_name);
    }

    int error = 0;

    // 하위 디렉터리 재귀 방문
    for (int i = 0; i < count; i++) {
        const char *name = entries[i]->d_name;

        size_t len = strlen(path) + strlen(name) + 2;
        char *child = malloc(len);

        if (child == NULL) {
            fprintf(stderr, "메모리 할당 실패\n");
            error = 1;
            break;
        }

        snprintf(child, len, "%s/%s", path, name);

        struct stat st;

        // 심볼릭 링크는 재귀적으로 따라가지 않음
        if (lstat(child, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (list_recursive(child) != 0) {
                    error = 1;
                }
            }
        } else {
            perror(child);
            error = 1;
        }

        free(child);
    }

    for (int i = 0; i < count; i++) {
        free(entries[i]);
    }
    free(entries);

    return error;
}

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";
    return list_recursive(path);
}
