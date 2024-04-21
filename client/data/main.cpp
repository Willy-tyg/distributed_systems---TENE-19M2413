#include "client.h"
#include <mutex>
using namespace std;
namespace fs = filesystem;

struct File {
    string name;
    int size;
};

// Mutex pour la gestion des accès concurrents au fichier de journal
mutex fileMutex;


//fonction pour detecter les changements dans un fichier
bool watchDirectory(const string& directoryPath) {
    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        cerr << "Erreur lors de l'initialisation d'inotify" << endl;
        return false;
    }

    int watchDescriptor = inotify_add_watch(inotifyFd, directoryPath.c_str(), IN_MODIFY | IN_CREATE | IN_DELETE);
    if (watchDescriptor == -1) {
        cerr << "Erreur lors de l'ajout de la surveillance inotify" << endl;
        close(inotifyFd);
        return false;
    }

    char buffer[4096];
    ssize_t bytesRead = read(inotifyFd, buffer, sizeof(buffer));

    if (bytesRead == -1) {
        cerr << "Erreur lors de la lecture d'inotify" << endl;
        close(inotifyFd);
        return false;
    }

    bool changeDetected = false;

    while (true) {
        struct inotify_event* event = reinterpret_cast<struct inotify_event*>(&buffer[0]);
        if (event->mask & IN_MODIFY || event->mask & IN_CREATE || event->mask & IN_DELETE) {
            cout << "Le contenu du dossier a changé." << endl;
            changeDetected = true;
        }

        bytesRead = read(inotifyFd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            cerr << "Erreur lors de la lecture d'inotify" << endl;
            break;
        }
    }

    inotify_rm_watch(inotifyFd, watchDescriptor);
    close(inotifyFd);

    return changeDetected;
}


// Fonction pour écrire dans le fichier de journal
void writeLog(const string& message) {
    // Get the current date and time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[80];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Lock the mutex to prevent concurrent access to the file
    lock_guard<mutex> lock(fileMutex);

    // Open the log file in append mode if it's not already open
    ofstream logFile("log.txt", ios::app);
    if (!logFile) {
        cerr << "Error opening the log file." << endl;
        return;
    }

    // Write the message and date to the log file
    logFile << "[" << timeStr << "] " << message << endl;
}

// Fonction de sérialisation
string serialize(const vector<File>& files) {
    string serializedData;

    // Parcourir chaque élément du vecteur
    for (const File& file : files) {
        // Ajouter le nom du fichier suivi de la taille
        serializedData += file.name + "," + to_string(file.size) + ";";
    }

    return serializedData;
}

vector<File> getFileInfo(const string& dataPath) {
    // Liste des fichiers dans le dossier "data"
    vector<File> files;
    fs::directory_iterator endIter;
    fs::directory_iterator iter(dataPath);

    // Ouverture du fichier de sortie
    ofstream outFile("fichiers.txt", ios::app); // Mode append

    // Boucle sur chaque fichier dans le dossier
    while (iter != endIter) {
        // Nom du fichier
        string fileName = iter->path().filename().string();

        // Taille du fichier
        int fileSize = fs::file_size(*iter);
        // Écriture dans la structure File
        File f;
        f.name = fileName;
        f.size = fileSize;

        files.push_back(f);
        // Puisage à la suite
        iter++;
    }

    return files; // Retourner le vecteur de structures File
}

void sendData(int socketFd, const vector<File>& files) {
    // Sérialiser le vecteur
    string serializedFiles = serialize(files);

    // Envoyer les données sur le socket
    const char* data = serializedFiles.c_str();
    int dataSize = strlen(data);

    if (send(socketFd, data, dataSize, 0) == -1) {
        cerr << "Erreur lors de l'envoi des données sur le socket" << endl;
        writeLog("Erreur lors de l'envoi des données sur le socket");
    } else {
        cout << "Données envoyées avec succès sur le socket" << endl;
        writeLog("Données envoyées avec succès sur le socket");
  
    }
}

int main(int argc, char* argv[]) {
    while (true) {
    // création du socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Erreur lors de la création du socket." << endl;
        writeLog("Erreur lors de la création du socket.");
        return 1;
    }

    // spécification de l'adresse
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    

    // Convert IP address from command line argument to binary form
    if (argc < 3) {
        cerr << "Utilisation: ./client @ip port" << endl;
        writeLog("Utilisation: ./client @ip port");
        return -1;
    }

    const char* ipAddress = argv[1];
    if (inet_pton(AF_INET, ipAddress, &(serverAddress.sin_addr.s_addr)) <= 0) {
        cerr << "Adresse IP invalide" << endl;
        writeLog("Adresse IP invalide");
        return -1;
    }

    int port = stoi(argv[2]);
    serverAddress.sin_port = htons(port);

    // envoi de la demande de connexion
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Erreur lors de la connexion au serveur." << endl << flush;
        writeLog("Erreur lors de la connexion au serveur.");
        close(clientSocket);
        return 1;
    } else {
        writeLog("connexion avec le serveur ");
    }


    vector<File> files = getFileInfo("data");
    // Envoyer les données sur le socket
    sendData(clientSocket, files);

    string directoryPath = "data";  // Répertoire des fichiers


    while(true){
        bool changeDetected = watchDirectory(directoryPath);
        if (changeDetected) {
            cout << "Changement détecté dans le dossier." << endl;

            vector<File> files = getFileInfo("data");

            // Envoyer les données sur le socket
            sendData(clientSocket, files);
        } 
    }
    

    }



    return 0;
}