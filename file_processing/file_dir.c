
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

int main(int argc, char *argv[]) {
    const char *path = (argc > 1) ? argv[1] : ".";

    // 디렉터리 열기
    DIR *dir = opendir(path);
    if (dir == NULL) {
        perror("opendir");
        return 1;
    }

    printf("디렉터리 목록: %s\n", path);

    struct dirent *entry;

    // 디렉터리 내부 항목 읽기
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        printf("%s\n", entry->d_name);
    }

    // 디렉터리 닫기
    if (closedir(dir) == -1) {
        perror("closedir");
        return 1;
    }

    return 0;
}
