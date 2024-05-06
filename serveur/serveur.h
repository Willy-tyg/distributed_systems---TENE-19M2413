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
struct File {
    string name;
    int size;
};

struct ThreadArgs {
    int clientSocket;
    ssize_t bytesReceived;
    int clientPort;
    string clientIP;
};

struct DownloadArgs {
    int clientSocket;
    ssize_t bytesReceived;
    int clientPort;
    string clientIP;
    string file_to_download;
    string url_ftp;
};

struct ClientData {
    std::string ip;
    int socket;
};

void* handleClient(void* arg);
void* sendFileList(void *arg);
void writeLog(const string& message);
void supprimerBlocIP(const string& cheminFichier, const string& adresseIP);
string serialize(const vector<File>& files);


#endif //SERVER_H