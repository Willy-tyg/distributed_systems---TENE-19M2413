#include "client.h"

vector<ListFile> fileList;

std::mutex file_mutex;

std::mutex fileList_mutex;

//function using to write a log into the log file
void writeLog(const string& message) {
    // Get the current date and time
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    char timeStr[80];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

    static mutex fileMutex;  // Static mutex to ensure exclusive access to the log file
    lock_guard<mutex> lock(fileMutex);  // Lock the mutex to guarantee exclusive access

    // Open the log file in append mode if it's not already open
    ofstream logFile("log.cls", ios::app);
    if (!logFile) {
        cerr << "Error opening the log file." << endl;
        return;
    }

    // Write the message and date to the log file
    logFile << "[" << timeStr << "] " << message << endl;
}


//function that take information to a file in the datapath
vector<File> getFileInfo(const string& dataPath) {
    // Liste des fichiers dans le dossier "data"
    file_mutex.lock();
    vector<File> files;

    if (fs::is_directory(dataPath)) {
        fs::directory_iterator endIter;
        fs::directory_iterator iter(dataPath);

        // Boucle sur chaque fichier dans le dossier
        while (iter != endIter) {
            if (fs::is_regular_file(*iter)) { // Check if it's a regular file
                string fileName = iter->path().filename().string();
                int fileSize = fs::file_size(*iter);
                
                // Write file information to the File structure
                File f;
                f.file_name = fileName;
                f.file_size = fileSize;

                files.push_back(f); // Add file information to the vector
            }
            // Move to the next item in the directory
            iter++;
        }

    } else {
        // Si le dossier n'existe pas, écrire un message d'erreur dans le journal
        // ou gérer l'erreur d'une autre manière appropriée.
        writeLog("FATAL: the directory '"+ dataPath +"' does not exist");
        return vector<File>(); // Retourner un vecteur vide
    }
    file_mutex.unlock();
    return files; // Retourner le vecteur de structures File
}

// function to serialize
string serialize(const vector<File>& files) {
    string serializedData;
    file_mutex.lock();
    // browse each element of the vector
    for (const File& file : files) {
        // add the name of the file and the size
        serializedData += file.file_name + "," + to_string(file.file_size) + ";";
    }
    file_mutex.unlock();
    return serializedData;
}


//function to send the data to a server
void sendData(int socketFd, const vector<File>& files) {
    // Sérialiser le vecteur
    string serializedFiles = serialize(files);

    const char* request = "DECLARE_FILE";

    std::string dataToSend(request);
    dataToSend += "\n";  // Ajouter une nouvelle ligne pour les fichiers serialisés
    dataToSend += serializedFiles;


    // Envoyer la demande et les informations du fichier au serveur
    if (send(socketFd, dataToSend.c_str(), dataToSend.size(), 0) == -1) {
        perror("error when sending the data into the socket");
        writeLog("ERROR: error when sending the data into the socket");
    } else {
        cout << "Data successful sending into the socket" << endl;
        writeLog("INFO: Data successful sending into the socket");
  
    }
}

void* UnderstandingDirectory(void *arg){

    string directoryPath = "data";
    int clientSocket = *reinterpret_cast<int*>(arg);

    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        writeLog("ERROR: Error initializing inotify");
        return nullptr;
    }

    int watchDescriptor = inotify_add_watch(inotifyFd, directoryPath.c_str(), IN_ALL_EVENTS);
    if (watchDescriptor == -1) {
        writeLog("ERROR: Error adding watch to directory: '" +directoryPath+"'.");
        close(inotifyFd);
        return nullptr;
    }

    while(true){
        

        char buffer[4096];
        ssize_t bytesRead;
        bytesRead = read(inotifyFd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            writeLog("ERROR: Error reading from inotify"); 
            return nullptr;
        }

        if (fs::is_directory(directoryPath)) {
            for (char* ptr = buffer; ptr < buffer + bytesRead;) {
                struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
            
                if (event->mask & IN_CREATE){
                    writeLog("File created into '" +directoryPath+"'.");
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesc = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesc);
                }
                if (event->mask & IN_DELETE){
                    writeLog("File delete into'" +directoryPath+"'.");
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesd = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesd);
                }
                if (event->mask & IN_MODIFY){
                    writeLog("File modified into '" +directoryPath+"'.");
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesm = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesm);
                }
                if (event->mask & IN_MOVED_FROM){
                    writeLog("File moved from '" +directoryPath+"'.");
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesf = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesf);
                }
                if (event->mask & IN_MOVED_TO){
                    writeLog("File moved to '" +directoryPath+"'.");
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filest = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filest);
                }

                ptr += sizeof(struct inotify_event) + event->len;
            }
            
        }
        else {
            // Si le dossier n'existe pas, écrire un message d'erreur dans le journal
            // ou gérer l'erreur d'une autre manière appropriée.
            writeLog("FATAL: can't detect change into the directory: '"+ directoryPath +"' directory does not exist");
            return nullptr;
        }



    }
    // Clean up
    close(watchDescriptor);
    close(inotifyFd);

    close(clientSocket);
    return nullptr;

}



constexpr int BUFFER_SIZE = 1024;


// Fonction pour avoir la liste des fichiers pouvant être téléchargés
void getlistfile(int socketFd) {
    const char* request = "GET_FILE_LIST";
    int requestSize = strlen(request) + 1;
    const int bufferSize = 4096;
    char buffer[bufferSize];

    // Envoyer la demande au serveur
    if (send(socketFd, request, requestSize, 0) == -1) {
        writeLog("Error when asking for the list of files");
        return;
    } else {
        writeLog("INFO: Request for file list sent successfully");
    }

    ssize_t bytesReceived;
    string receivedData;

    // Réinitialiser la liste des fichiers avant de la remplir
    fileList_mutex.lock();
    fileList.clear();
    fileList_mutex.unlock();

    while ((bytesReceived = recv(socketFd, buffer, bufferSize - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';
        receivedData += buffer;
        writeLog("INFO: liste des fichiers recus");

        // Vérifier si la réponse contient la fin de la liste
        if (receivedData.find("END_OF_FILE") != string::npos) {
            break;
        }
    }

    if (bytesReceived == -1) {
        writeLog("Error receiving data from the server");
        return;
    } else if (bytesReceived == 0) {
        writeLog("Connection closed by the server");
        return;
    }

    // Traiter les données reçues
    size_t found = receivedData.find("LIST_FILE");
    if (found != string::npos) {
        stringstream ss(receivedData.substr(found));

        string line;
        int lineNumber = 0;
        string owner;
        string file_info;

        fileList_mutex.lock();
        while (getline(ss, line)) {
            if (lineNumber != 0 && line != "END_OF_FILE") {
                ListFile file;


                size_t Pos = line.find(';');
                file_info = line.substr(0, Pos);
                owner = line.substr(Pos + 1);

                file.file_info = file_info;
                file.ipOwner = owner;
                file.file_number = lineNumber;
                fileList.push_back(file);

                cout << file.file_number << ". " << file.file_info  << endl;
            }
            lineNumber++;
        }
        fileList_mutex.unlock();
    } else {
        writeLog("No valid file list found in the response");
    }
}


void* sendFile(void* arg) {
    
    int port = 1250;


    int mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == -1) {
        writeLog("ERROR: Échec de la création du socket.");
        return nullptr;
    }
    else{
        writeLog("INFO: the socket that receive file created successful");
    }

    sockaddr_in myAddress;


    myAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    // Configuration de l'adresse du serveur
    myAddress.sin_family = AF_INET;
    myAddress.sin_port = htons(port);


    int reuse = 1;
    if (setsockopt(mySocket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        return nullptr;
    }

    // Lier la socket à l'adresse du serveur
    if (bind(mySocket, reinterpret_cast<sockaddr*>(&myAddress), sizeof(myAddress)) == -1) {
        std::cerr << "Erreur lors du bind." << std::endl;
        close(mySocket);
        return nullptr;
    }

    // Mettre la socket en mode écoute
    if (listen(mySocket, 1) == -1) {
        std::cerr << "Erreur lors de la mise en écoute de la socket." << std::endl;
        close(mySocket);
        return nullptr;
    }

    while (true) {
        // Accepter une connexion entrante
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(mySocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
        if (clientSocket == -1) {
            std::cerr << "Erreur lors de l'acceptation de la connexion." << std::endl;
            continue;
        }
        writeLog("INFO: un client veut un de mes fichiers");

        // Lire la commande STOR du client
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesRead == -1) {
            std::cerr << "Erreur lors de la réception de la commande." << std::endl;
            close(clientSocket);
            continue;
        }

        std::string command(buffer);
        if (command.substr(0, 3) != "GET") {
            std::cerr << "Commande GET attendue." << std::endl;
            close(clientSocket);
            continue;
        }

        // Extraire le nom du fichier à partir de la commande GET
        std::string receivedFilename = command.substr(4);
        writeLog("INFO: nom du fichier'"+ receivedFilename +"';");
        // Ouvrir un fichier en écriture pour recevoir les données
        std::string fullPath = "data/" + receivedFilename;

        if (!std::filesystem::exists(fullPath)) {
            writeLog("ERROR: le fichier que je dois envoyer n'est pas dans mon repertoire");
            close(clientSocket);
            return reinterpret_cast<void*>(1);
        }

        // Ouvrir le fichier en mode binaire pour l'envoi des données
        std::ifstream file(fullPath, std::ios::binary);
        if (!file) {
            writeLog("ERROR: je n'arrive pas à ouvrir le fichier à envoyer");
            close(clientSocket);
            return reinterpret_cast<void*>(1);
        }

        // Envoi des données du fichier au client
        
        while (file.read(buffer, sizeof(buffer))) {
            ssize_t bytesSent = send(clientSocket, buffer, file.gcount(), 0);
            if (bytesSent == -1) {
                writeLog("ERROR: Erreur lors de l'envoi des données du fichier.");
                file.close();
                close(clientSocket);
                return reinterpret_cast<void*>(1);
            }
        }

        const char* request = "END";
        int requestSize = strlen(request) + 1;

        // Marqueur de fin
        send(clientSocket, request, requestSize, 0);


        // Fermeture du fichier et de la socket
        file.close();
    

        writeLog("Envoi du fichier terminé.");
    }
    return nullptr;
}


void* download_file(void* arg) {

    ThreadParams* params = static_cast<ThreadParams*>(arg);

    // Accéder aux paramètres du thread
    string fileInfo = params->file_info;
    string ipOwner = params->ipOwner;
    int serverPort = 1250;

    size_t octetsIndex = fileInfo.find("octets");

    // Extraire la partie de la chaîne de caractères jusqu'à "octets"
    std::string filenameAndSizeStr = fileInfo.substr(0, octetsIndex-1);

    // Trouver l'indice du dernier espace avant "octets"
    size_t spaceIndex = filenameAndSizeStr.find_last_of(' ');

    // Extraire la taille du fichier à partir de la chaîne de caractères
    std::string sizeStr = filenameAndSizeStr.substr(spaceIndex + 1);

    size_t fileSize;
    std::istringstream iss(sizeStr);
    iss >> fileSize;
    // Extraire le nom du fichier à partir de la chaîne de caractères
    std::string filename = filenameAndSizeStr.substr(0, spaceIndex);

    int rcvSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (rcvSocket == -1) {
        writeLog("Erreur lors de la création de la socket.");
        return nullptr;
    }

    // Configuration de l'adresse du serveur
    sockaddr_in rcvAddress;
    rcvAddress.sin_family = AF_INET;
    rcvAddress.sin_addr.s_addr = inet_addr(ipOwner.c_str());
    rcvAddress.sin_port = htons(serverPort);

    // Connexion au serveur

   
    if (connect(rcvSocket, (struct sockaddr*)&rcvAddress, sizeof(rcvAddress)) == -1) {
        writeLog("Erreur lors de la connexion au serveur."); 
    }else{
        writeLog("INFO: success connect to a server");
    }


    // Envoi de la commande STOR au serveur
    std::string command = "GET " + filename;
    if (send(rcvSocket, command.c_str(), command.size(), 0) == -1) {
        writeLog("Erreur lors de l'envoi de la commande.");
        close(rcvSocket);
        return reinterpret_cast<void*>(1);
    }

    // Ouvrir un fichier en écriture pour recevoir les données
    std::string fullPath = "download/" + filename;

       

    std::ofstream file(fullPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Erreur lors de l'ouverture du fichier." << std::endl;
        close(rcvSocket);
    }
    std::cout << "Début de la réception." << std::endl;

    // Initialisation des variables
    int totalBytes = 0;
    int receivedBytes = 0;
    const int TOTAL_SIZE = fileSize; // Taille totale du fichier 
    // Recevoir les données du fichier et les écrire dans le fichier
    char buffer[BUFFER_SIZE];
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRead = recv(rcvSocket, buffer, sizeof(buffer), 0);
        if (bytesRead == -1) {
            std::cerr << "Erreur lors de la réception des données du fichier." << std::endl;
            file.close();
            close(rcvSocket);
            break;
        }
        if (bytesRead == 0) {
            std::cout << "Réception des données du fichier terminée." << std::endl;
            writeLog("INFO: Réception des données du fichier terminée.");
            break;
        }

        file.write(buffer, bytesRead);
        totalBytes += bytesRead;
        receivedBytes += bytesRead;

        // Afficher la barre de progression
        int progress = (int)(((float)receivedBytes / (float)TOTAL_SIZE) * 100.0f);
        std::cout << "\r[";
        for (int i = 0; i < 100; i++) {
            if (i < progress ) {
                std::cout << "=";
            } else {
                std::cout << " ";
            }
        }
        std::cout << "] " << progress << "% (" << receivedBytes << "/" << TOTAL_SIZE << " bytes)";
        std::cout.flush();

        std::string command(buffer);
        if (command.substr(0, 3) != "END") {
            continue;
        } else {
            break;
        }
    }

    std::cout << "\nFichier reçu avec succès!" << std::endl;
    writeLog("INFO: fin du telechargement!");
    // Fermer le fichier
    file.close();

    // Fermer la socket du client
    close(rcvSocket);

}




