
#include <stdio.h>
#include <string.h>
#include <systemd/sd-bus.h>

int main(int argc, char *argv[]) {
    sd_bus *bus = NULL;
    sd_bus_error error = SD_BUS_ERROR_NULL;
    sd_bus_message *reply = NULL;
    const char *response = NULL;
    int r;
    int result = 1;

    if (argc != 2) {
        fprintf(stderr, "사용법: %s \"알림 메시지\"\n",
                argv[0]);
        return 1;
    }

    // 사용자 D-Bus 세션 연결
    r = sd_bus_default_user(&bus);
    if (r < 0) {
        fprintf(stderr, "D-Bus 연결 실패: %s\n",
                strerror(-r));
        goto finish;
    }

    printf("=== D-Bus 알림 클라이언트 ===\n");
    printf("[클라이언트] 전송 메시지: %s\n", argv[1]);

    // 서버의 Notify 메서드 호출
    r = sd_bus_call_method(
        bus,
        "org.example.Lab2Notify",
        "/org/example/Lab2Notify",
        "org.example.Lab2Notify",
        "Notify",
        &error,
        &reply,
        "s",
        argv[1]
    );

    if (r < 0) {
        fprintf(stderr, "알림 전송 실패: %s\n",
                error.message ? error.message : strerror(-r));
        goto finish;
    }

    // 서버 응답 읽기
    r = sd_bus_message_read(reply, "s", &response);

    if (r < 0) {
        fprintf(stderr, "응답 읽기 실패: %s\n",
                strerror(-r));
        goto finish;
    }

    printf("[서버 응답] %s\n", response);
    puts("D-Bus 알림 전송 성공");

    result = 0;

finish:
    sd_bus_error_free(&error);
    sd_bus_message_unref(reply);
    sd_bus_unref(bus);

    return result;
}
