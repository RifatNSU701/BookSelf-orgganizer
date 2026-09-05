# Makefile for the Bookshelf Management System
# ------------------------------------------------------------
# Works on Linux, macOS, Cygwin and MSYS2 (all provide POSIX
# fork/pthreads/semaphores). On native MinGW you can also use this
# via `mingw32-make`; the program builds but the fork() demo prints
# an honest "not supported" notice (see src/process_manager.c).
#
# Targets:
#   make        -> build ./bin/BookshelfManagement
#   make run    -> build and run
#   make clean  -> remove build artefacts
# ------------------------------------------------------------

CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS := -lpthread

# On some MinGW setups pthread lives in -lpthread already; if your
# toolchain needs -pthread instead, change LDFLAGS to -pthread.

SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
BIN     := bin/BookshelfManagement

.PHONY: all run clean dirs

all: dirs $(BIN)

dirs:
	@mkdir -p bin logs

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

# Compile each .c to .o, depending on all headers for simplicity.
src/%.o: src/%.c $(wildcard include/*.h)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f src/*.o
	rm -f $(BIN)
