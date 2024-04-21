#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <thread>
#include <cstring>
#include <regex>



using namespace std;

void* handleClient(void* arg);
void writeLog(const string& message);
void supprimerBlocIP(const string& cheminFichier, const string& adresseIP);


#endif //SERVER_H