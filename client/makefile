CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -pthread 

client: main.o
	$(CC) $(CFLAGS) $^ -o $@ -lcurl
main.o: main.cpp client.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o client