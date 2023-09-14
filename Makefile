CFLAGS := -g -std=c2x -fsanitize=address
CC := clang

all: ccc.exe

ccc.exe: main.o
	$(CC) $(CFLAGS) $^ -o $@ -llibffi

main.o: main.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.exe

.PHONY: clean all