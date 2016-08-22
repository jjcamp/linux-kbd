CC=clang++
CFLAGS=-Wall -std=c++1y -stdlib=libc++

all: 
	$(CC) $(CFLAGS) main.cpp -o kbd

clean:
	rm kbd