
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <systemd/sd-bus.h>

// 클라이언트가 호출하는 알림 처리 함수
static int handle_notify(
    sd_bus_message *m,
    void *userdata,
    sd_bus_error *error
) {
    (void)userdata;
    (void)error;

    const char *message;
    int r = sd_bus_message_read(m, "s", &message);

    if (r < 0)
        return r;

    printf("[서버] 알림 수신: %s\n", message);
    fflush(stdout);

    return sd_bus_reply_method_return(
        m, "s", "알림 수신 완료"
    );
}

// D-Bus 서비스 인터페이스
static const sd_bus_vtable vtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_METHOD(
        "Notify", "s", "s",
        handle_notify,
        SD_BUS_VTABLE_UNPRIVILEGED
    ),
    SD_BUS_VTABLE_END
};

int main(void) {
    sd_bus *bus = NULL;
    sd_bus_slot *slot = NULL;
    int r;

    // 사용자 세션 버스 연결
    r = sd_bus_default_user(&bus);
    if (r < 0) {
        fprintf(stderr, "D-Bus 연결 실패: %s\n", strerror(-r));
        goto finish;
    }

    // 객체와 메서드 등록
    r = sd_bus_add_object_vtable(
        bus, &slot,
        "/org/example/Lab2Notify",
        "org.example.Lab2Notify",
        vtable, NULL
    );

    if (r < 0) {
        fprintf(stderr, "객체 등록 실패: %s\n", strerror(-r));
        goto finish;
    }

    // 서비스 이름 등록
    r = sd_bus_request_name(
        bus, "org.example.Lab2Notify", 0
    );

    if (r < 0) {
        fprintf(stderr, "서비스 등록 실패: %s\n", strerror(-r));
        goto finish;
    }

    puts("=== D-Bus 알림 서버 ===");
    puts("서비스 등록 완료");
    puts("클라이언트 알림 대기 중...");
    fflush(stdout);

    // 클라이언트 메시지 처리
    for (;;) {
        r = sd_bus_process(bus, NULL);

        if (r < 0) {
            fprintf(stderr, "메시지 처리 실패: %s\n", strerror(-r));
            break;
        }

        if (r > 0)
            continue;

        r = sd_bus_wait(bus, UINT64_MAX);

        if (r < 0) {
            fprintf(stderr, "D-Bus 대기 실패: %s\n", strerror(-r));
            break;
        }
    }

finish:
    sd_bus_slot_unref(slot);
    sd_bus_unref(bus);

    return 1;
}
