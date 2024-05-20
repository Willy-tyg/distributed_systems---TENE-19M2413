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
    string clientIP;
};

struct DownloadArgs {
    int clientSocket;
    ssize_t bytesReceived;
    int clientPort;
    string clientIP;
    string file_to_download;
    string myIP;
};

struct ClientData {
    std::string ip;
    int socket;
};

//mutex pour verouiller le vecteur clientData
extern std::mutex client_mutex;

//mutex pour verouiller le fichier infos_client.txt
extern std::mutex info_mutex;

//mutex pour verouiller le vecteur File
extern std::mutex file_mutex;


//vecteur pour stocker les adresses ip et socket des differents clients
extern std::vector<ClientData> clients;

void writeLog(const string& message); //log function
void* handleClient(void* arg);
//function to deserialize data
vector<File> deserialize(const string& serializedData);


void* sendFileList(void *arg);
void supprimerBlocIP(const string& cheminFichier, const string& adresseIP);
string serialize(const vector<File>& files);

//function to send the list of file to a client when asking
void* sendFileList(void* arg);
int getClientSocketByIP(const std::string& clientIP);
void* DownloadFile(void* arg);

#endif //SERVER_H