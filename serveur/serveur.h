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
using namespace std;

void* handleClient(void* arg);
void writeLog(const string& message);

#endif // SERVER_H