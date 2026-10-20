CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
ASAN_FLAGS = -std=c11 -Wall -Wextra -Wpedantic -g -fsanitize=address,undefined

TARGET = analyzer

SRCS = src/main.c \
       src/lexer.c \
       src/ast.c \
       src/parser.c \
       src/symtab.c \
       src/checks.c \
       src/diag.c \
       src/report_console.c \
       src/report_html.c

OBJS = $(SRCS:.c=.o)
ASAN_OBJS = $(SRCS:.c=.asan.o)

.PHONY: all clean test asan

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.asan.o: %.c
	$(CC) $(ASAN_FLAGS) -c $< -o $@

asan: $(ASAN_OBJS)
	$(CC) $(ASAN_FLAGS) -o $(TARGET) $(ASAN_OBJS)

test: $(TARGET)
	@sh tests/run_tests.sh

clean:
	rm -f $(OBJS) $(ASAN_OBJS) $(TARGET) $(TARGET).exe
