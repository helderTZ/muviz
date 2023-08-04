CC=clang
CFLAGS=-Wall -Wextra `pkg-config --cflags raylib` -O3
# LIBS=`pkg-config --libs raylib` -lm -pthread -ldl -lglfw
LIBS=-lm -pthread -ldl -lglfw

all: muviz

muviz: main.c
	${CC} -o $@ $^ /usr/local/lib/libraylib.a ${CFLAGS} ${LIBS}

.PHONY: clean

clean:
	if [ -f muviz ]; then rm muviz; fi