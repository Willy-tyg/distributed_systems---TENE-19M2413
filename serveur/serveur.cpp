#include "serveur.h"
#include <cstring>
#include <regex>

struct File {
    string name;
    int size;
};


void writeLog(const string& message) {
    // Get the current date and time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[80];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    static mutex fileMutex;  // Static mutex to ensure exclusive access to the log file
    lock_guard<mutex> lock(fileMutex);  // Lock the mutex to guarantee exclusive access

    // Open the log file in append mode if it's not already open
    ofstream logFile("log.txt", ios::app);
    if (!logFile) {
        cerr << "Error opening the log file." << endl;
        writeLog("ERROR: error when opennig the log file");

        return;
    }

    // Write the message and date to the log file
    logFile << "[" << timeStr << "] " << message << endl;
}


void supprimerBlocIP(const string& cheminFichier, const string& adresseIP) {
    ifstream fichier(cheminFichier);
    if (!fichier.is_open()) {
        cerr << "Erreur : impossible d'ouvrir le fichier." << endl;
        writeLog("ERROR: unable to open the file to update him");

        return;
    }

    ofstream fichierTemp("temp.txt"); // Fichier temporaire pour stocker les lignes restantes
    if (!fichierTemp.is_open()) {
        cerr << "Erreur : impossible de créer le fichier temporaire." << endl;
        writeLog("ERROR: can't create temporal file: temp.txt");

        return;
    }

    string ligne;
    bool blocTrouve = false;
    bool nouvelleAdresseIP = false;
    // Lire les lignes du fichier
    while (getline(fichier, ligne)) {
        if (blocTrouve) {
            // Écrire les lignes restantes dans le fichier temporaire
            if (!nouvelleAdresseIP ){
                if(ligne.find(':') != string::npos) {
                    nouvelleAdresseIP = true;
                    fichierTemp << ligne << endl;
                }
            }
            else{
               fichierTemp << ligne << endl; 
            }
            
        } else {
            // Vérifier si la ligne contient l'adresse IP spécifiée suivie de ":"
            size_t pos = ligne.find(adresseIP + " :");
            if (pos != string::npos) {
                // Si oui, marquer le début du bloc à supprimer
                blocTrouve = true;
            } else {
                // Si non, écrire la ligne dans le fichier temporaire
                fichierTemp << ligne << endl;
            }
        }
    }

    fichier.close();
    fichierTemp.close();

    // Remplacer le fichier d'origine par le fichier temporaire
    remove(cheminFichier.c_str());
    rename("temp.txt", cheminFichier.c_str());
}




vector<File> deserialize(const string& serializedData) {
    vector<File> files;

    string dataCopy = serializedData; // Copie de la chaîne d'origine

    size_t pos = 0;
    while ((pos = dataCopy.find(";")) != string::npos) {
        string token = dataCopy.substr(0, pos);
        size_t commaPos = token.find(",");
        if (commaPos != string::npos) {
            string name = token.substr(0, commaPos);
            int size = stoi(token.substr(commaPos + 1));
            files.push_back({name, size});
        }
        dataCopy.erase(0, pos + 1);
    }

    return files;
}

void* handleClient(void* arg) {
    int clientSocket = *(int*)arg;
    delete (int*)arg;

    // Obtention de l'adresse IP et du numéro de port du client
    sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    if (getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressLength) == -1) {
        cerr << "Erreur lors de l'obtention de l'adresse IP du client." << endl;
        writeLog("ERROR: error when obtaining the client ip");

        close(clientSocket);
        pthread_exit(NULL);
    }
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddress.sin_port); // Convertir le numéro de port en format d'entier

    string filePath = "infos_client.txt";

    supprimerBlocIP(filePath, clientIP);
    // Réception des données du client
    char buffer[4096];
    ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        cerr << "Erreur lors de la réception des données du client " << clientIP << ":" << clientPort << "." << endl;
        std::ostringstream erreur;
        erreur << "ERROR: error when obtaining the client data '" << clientIP << "':'" << clientPort << "'";
        writeLog(erreur.str());

        close(clientSocket);
        pthread_exit(NULL);
    }

    // Convertir les données reçues en une chaîne de caractères
    string receivedData(buffer, bytesReceived);

    // Désérialiser les données
    vector<File> receivedFiles = deserialize(receivedData);

    // Écrire les données dans un fichier
    ofstream fichier("infos_client.txt", ios::app);
    if (fichier.is_open()) {
        // Écrire les informations de l'adresse IP et du port du client
        fichier << clientIP << " : " << clientPort << endl;

        // Écrire les informations des fichiers dans le fichier
        for (const auto& file : receivedFiles) {
            fichier << file.name << " " << file.size << " octets" << endl;
        }
        // Ajouter un retour à la ligne après chaque ensemble d'informations
        fichier << endl;

        // Fermer le fichier
        fichier.close();

        cout << "reception des données du client "<< clientIP << endl;
        std::ostringstream log_info;
        log_info << "INFO: data receipt '" << clientIP << "':'" << clientPort << "'";
        writeLog(log_info.str());

    } else {
        cerr << "Erreur lors de l'ouverture du fichier infos_client.txt" << endl;
        writeLog("ERROR: error when openning file infos_client.txt");

    }

    // Fermer le socket client
    close(clientSocket);
    pthread_exit(NULL);
}


