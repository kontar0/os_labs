CC      = gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g
CPPFLAGS = -D_POSIX_C_SOURCE=200809L -Iinclude

BUILD   = build
COMMON  = $(BUILD)/io_utils.o
PARENT_OBJS = $(BUILD)/parent.o $(BUILD)/process.o $(COMMON)
CHILD_OBJS  = $(BUILD)/child.o $(BUILD)/sum.o $(COMMON)

.PHONY: all clean test trace

all: $(BUILD)/parent $(BUILD)/child

$(BUILD)/parent: $(PARENT_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/child: $(CHILD_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD)/%.o: src/%.c | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD):
	mkdir -p $@

-include $(wildcard $(BUILD)/*.d)

test: all
	./tests/run_tests.sh

# Трассировка всех процессов (-f), запись в docs/strace.log
trace: all
	echo tests/data/basic.txt | strace -f -tt -e trace=process,pipe,pipe2,dup2,dup3,openat,close,read,write,rt_sigaction \
		-o docs/strace.log ./$(BUILD)/parent
	@echo "trace written to docs/strace.log"

clean:
	rm -rf $(BUILD)
