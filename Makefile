CC ?= gcc
CARGO ?= cargo
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

C_SOURCES := \
	file_processing/file_basic.c \
	file_processing/file_dir.c \
	recursive_dir/recursive_ls.c \
	typing_practice/typing_practice.c \
	process_signal/process_basic.c \
	process_signal/signal_basic.c \
	ipc_basic/fifo_basic.c \
	pipe_chat/pipe_chat.c \
	message_queue_chat/mq_chat.c \
	shared_memory_copy/shm_copy.c \
	dbus_notification/dbus_server.c \
	dbus_notification/dbus_client.c
C_PROGRAMS := $(patsubst %.c,bin/%,$(C_SOURCES))

.PHONY: all c rust clean
all: c rust

c: $(C_PROGRAMS)

bin/dbus_notification/dbus_server bin/dbus_notification/dbus_client: LDLIBS += -lsystemd
bin/message_queue_chat/mq_chat bin/shared_memory_copy/shm_copy: LDLIBS += -lrt

bin/%: %.c Makefile
	mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LDLIBS)

rust:
	$(CARGO) build --locked --manifest-path rust_matrix/Cargo.toml

clean:
	$(RM) $(C_PROGRAMS)
	$(CARGO) clean --manifest-path rust_matrix/Cargo.toml
