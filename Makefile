CFLAGS = -Wall -Wextra -g

first: all

%.o: %.c
	cc $(CFLAGS) -c -o $@ $<

serdes: serdes.o
	cc $(LDFLAGS) -o $@ $< -L/usr/local/lib -lsqlite3

clean:
	rm -f serdes.o serdes

all: serdes
