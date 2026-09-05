CC      := gcc
CFLAGS  := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -Iinclude
LDFLAGS := -lpthread
SRC     := $(wildcard src/*.c)
OBJ     := $(SRC:.c=.o)
BIN     := bin/BookshelfManagement

.PHONY: all run clean dirs

all: dirs $(BIN)

dirs:
	@mkdir -p bin logs

$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN) $(LDFLAGS)

src/%.o: src/%.c $(wildcard include/*.h)
	$(CC) $(CFLAGS) -c $< -o $@

run: all
	./$(BIN)

clean:
	rm -f src/*.o
	rm -f $(BIN)
