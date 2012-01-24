#
# Makefile for Otto's rougelike
#

CC=gcc
CFLAGS=-Wall -m32
LIBS=-lcurses

all: rpg

OBJS=				\
		bin/rpg.o

rpg:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) \
	-o bin/rpg $(LIBS)

bin/%.o:	src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin/*.o
	rm -rf bin/rpg