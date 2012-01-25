#
# Makefile for Pyr0byt3's rougelike
#

CC=gcc
CFLAGS=-Wall -m32
LIBS=-lcurses
UNAME=$(shell uname)

all: rpg

OBJS=					\
		bin/$(UNAME)/rpg.o	\
		bin/$(UNAME)/wad.o

rpg:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) \
	-o bin/$(UNAME)/rpg $(LIBS)

bin/$(UNAME)/%.o:	src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf bin/$(UNAME)/*.o
	rm -rf bin/$(UNAME)/rpg