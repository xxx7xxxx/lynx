.PHONY: all install clean run tags debug dist-clean client

CC = gcc
CFLAGS = -Wall -g -I./src -lpthread
SOURCE = src/lynx.c src/rio.c src/http.c src/threadpool.c src/reactor.c src/configure.c \
	 	src/wrapper.c
HEADER = src/rio.h src/threadpool.h src/http.h src/reactor.h src/configure.h src/wrapper.h
OUT = a.out

all: $(OUT)
$(OUT): $(SOURCE) $(HEADER)
	$(CC) $(SOURCE) $(CFLAGS)

clean:
	$(RM) *.o
	$(RM) $(OUT)

run:
	./$(OUT)
