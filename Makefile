CC = gcc
CFLAGS = -Wall -g -I./include -lpthread
SOURCE = main.c src/rio.c src/http.c src/threadpool.c src/reactor.c
HEADER = include/rio.h include/threadpool.h include/http.h include/reactor.h
OUT = a.out

.PHONY: all install clean run tags debug dist-clean client
all: $(OUT)
$(OUT): $(SOURCE) $(HEADER)
	$(CC) $(SOURCE) $(CFLAGS) 

clean:
	$(RM) *.o
	$(RM) $(OUT)

run:
	./$(OUT)
