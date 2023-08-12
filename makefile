CC=clang
CFLAGS=-Wall -Wextra `pkg-config --cflags raylib` -O3
LIBS=`pkg-config --libs raylib` -lm -pthread -ldl -lglfw

all: muviz libplugin.so

muviz: main.c plugin.h
	${CC} -o $@ $< ${CFLAGS} ${LIBS}

libplugin.so: plugin.c plugin.h
	${CC} -o $@ $< -shared -fPIC ${CFLAGS} ${LIBS}

.PHONY: clean

clean:
	if [ -f muviz        ]; then rm muviz; fi
	if [ -f libplugin.so ]; then rm libplugin.so; fi