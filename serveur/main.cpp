#include "serveur.h"



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

    // spécification de l'adresse
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

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
        close(serverSocket);
        return 1;
    }
    else{
        writeLog("INFO: the socket is understanding");

    }



    while (true) {
        // accepter la demande de connexion
        sockaddr_in clientAddress;
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLength);
        if (clientSocket == -1) {
            cerr << "Erreur lors de l'acceptation de la connexion." << endl;
            close(serverSocket);
            return 1;
        }
        else{
            writeLog("INFO: connexion accepting success");

        }

        // Créer un thread pour gérer le client
        int* clientSocketPtr = new int(clientSocket);
        pthread_t thread;
        if (pthread_create(&thread, NULL, handleClient, (void*)clientSocketPtr) != 0) {
            cerr << "Erreur lors de la création du thread." << endl;
            close(clientSocket);
            delete clientSocketPtr;
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