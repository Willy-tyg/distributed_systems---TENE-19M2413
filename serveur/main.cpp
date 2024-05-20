#include "serveur.h"


//main function
int main()
{
    // création du socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Échec de la création du socket." << endl;
        writeLog("ERROR: error when creating the socket");
        return 1;
    }
    else{
        writeLog("INFO: the socket creating successful");
    }


    // Définir l'option SO_REUSEADDR
    int reuse = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        perror("Error setting SO_REUSEADDR option");
        writeLog("ERROR: error when define option SO_REUSEADDR for reuse address");
        return 1;
    }

    // spécification de l'adresse
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    int port = 10000;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.32.174");


    // liaison du socket
    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        cerr << "Échec de la liaison du socket." << endl;
        writeLog("ERROR: error when linked the socket");
        close(serverSocket);
        return 1;
    }
    else{
        writeLog("INFO: the socket linked successful");
    }

    // écoute sur le socket
    if (listen(serverSocket, 5) == -1) {
        cerr << "Échec de l'écoute sur le socket." << endl;
        writeLog("ERROR: error when listening on the socket");
        close(serverSocket);
        return 1;
    }
    else{
        writeLog("INFO: the socket is understanding");
    }

    std::ofstream("infos_client.txt", std::ofstream::out | std::ofstream::trunc).close();


    // Créer un thread pour gérer chaque client
    cout<<"serveur en ecoute sur le port : "<<port<<endl;
    while (true) {

        // accepter la demande de connexion
        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int* clientSocket = new int(accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength));
        if (*clientSocket == -1) {
            cerr << "Erreur lors de l'acceptation de la connexion." << endl;
            writeLog("ERROR: error when accepting the connection");
            delete clientSocket;
            continue;
        }
        else{
            writeLog("INFO: connection accepting success");
        }

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);

        cout<<" "<<endl;
        cout<<"le client: "<< clientIP <<" s'est connécté"<<endl;
        
        // Créer un thread pour gérer le client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handleClient, (void*)clientSocket) != 0) {
            cerr << "Erreur lors de la création du thread." << endl;
            writeLog("ERROR: error when creating the thread");
            close(*clientSocket);
            delete clientSocket;
            continue;
        }
        else{
            writeLog("INFO: thread creating successful");
        }


    }
    // fermeture du socket serveur
    close(serverSocket);

    return 0;
}
