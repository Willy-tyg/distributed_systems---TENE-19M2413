#ifndef CLIENT_H
#define CLIENT_H

#include <iostream>
#include <sys/socket.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include <unistd.h>
#include <cstdio>
#include <cerrno>
#include <vector>
#include <netdb.h>
#include <filesystem>
#include <sys/inotify.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <thread>



using namespace std;
namespace fs = filesystem;


//structure qui contient les infos des fichiers dans le dossier data
struct File{
    int file_size;
    string file_name;
};

//structure qui contiedra les infos sur la liste des fichiers disponible au niveau du serveur
struct ListFile{
    string file_info;
    int file_number;
};

//structure pour contenir les informations a envoyer au thread pour demander un fichier
struct ThreadParams {
    int clientSocket;
    string file_info;
};


struct DownloadArgs {
    int clientSocket;
    int fileSize;
    int port;
    string ipAddress;
    string filename;

};


//vecteur pour contenir les info sur tous les fichiers disponible chez lz serveur
extern vector<ListFile> fileList;

//log function
void writeLog(const string& message); 

//function that take information to a file in the datapath
vector<File> getFileInfo(const string& dataPath);

// function to serialize
string serialize(const vector<File>& files);

//function to send the data to a server
void sendData(int socketFd, const vector<File>& files);

//function for detect change int the directory data
void* UnderstandingDirectory(void *arg);

//function to get the list of file available
void getlistfile(int socketFd);

//vecteur pour contenir la liste des fichiers du client
extern vector<File> files;

//mutex pour verouiller le fichier vecteur file
extern std::mutex file_mutex;

//mutex pour verouiller le fichier vecteur fileList
extern std::mutex fileList_mutex;

void* download_file(void* arg);
void* sendFile(void *arg);
void* startReceive(void* arg);
void* sendFileToClient(void* arg);
#endif