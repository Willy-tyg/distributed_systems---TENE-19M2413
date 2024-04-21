CC = g++
CFLAGS = -Wall -Wextra -pthread

serveur: serveur.o main.o
	$(CC) $(CFLAGS) $^ -o $@

serveur.o: serveur.cpp serveur.h
	$(CC) $(CFLAGS) -c $<

main.o: main.cpp serveur.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o serveur 