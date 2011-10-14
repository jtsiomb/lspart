src = $(wildcard *.c)
obj = $(src:.c=.o)
bin = lspart

CC = gcc
CFLAGS = -pedantic -Wall -g

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f $(obj) $(bin)
