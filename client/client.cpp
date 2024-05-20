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
    ofstream logFile("log.txt", ios::app);
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

void* sendFileToClient(void* arg) {
    DownloadArgs* downloadArgs = static_cast<DownloadArgs*>(arg);
    std::string ipToSend = downloadArgs->ipAddress;
    std::string filename = downloadArgs->filename;
    int serverPort = downloadArgs->port;

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        writeLog("Erreur lors de la création de la socket.");
        pthread_exit(nullptr);
        return nullptr;
    }

    // Configuration de l'adresse du serveur
    sockaddr_in clientAddress{};
    clientAddress.sin_family = AF_INET;
    clientAddress.sin_addr.s_addr = inet_addr(ipToSend.c_str());
    clientAddress.sin_port = htons(serverPort);

    // Connexion au serveur
    bool connected= false;
    while(!connected){
        if (connect(clientSocket, reinterpret_cast<sockaddr*>(&clientAddress), sizeof(clientAddress)) == -1) {
            writeLog("Erreur lors de la connexion au serveur."); 
        }else{
            connected =true;
        }
    }

    // Envoi de la commande STOR au serveur
    std::string command = "STOR " + filename;
    if (send(clientSocket, command.c_str(), command.size(), 0) == -1) {
        writeLog("Erreur lors de l'envoi de la commande.");
        close(clientSocket);
        return reinterpret_cast<void*>(1);
    }

    std::string fullPath = "data/" + filename;

    if (!std::filesystem::exists(fullPath)) {
        writeLog("ERROR: le fichier que je dois envoyer nes pas dans mon repertoire");
        close(clientSocket);
        return reinterpret_cast<void*>(1);
    }

    // Ouvrir le fichier en mode binaire pour l'envoi des données
    std::ifstream file(fullPath, std::ios::binary);
    if (!file) {
        writeLog("ERROR: je narrive pas à ouvrir le fichier à envoyer");
        close(clientSocket);
        return reinterpret_cast<void*>(1);
    }

    // Envoi des données du fichier au client
    char buffer[BUFFER_SIZE];
    while (file.read(buffer, sizeof(buffer))) {
        ssize_t bytesSent = send(clientSocket, buffer, file.gcount(), 0);
        if (bytesSent == -1) {
            writeLog("ERROR: Erreur lors de l'envoi des données du fichier.");
            file.close();
            close(clientSocket);
            return reinterpret_cast<void*>(1);
        }
    }

    // Fermeture du fichier et de la socket
    file.close();
    close(clientSocket);

    writeLog("Envoi du fichier terminé.");
    pthread_exit(nullptr);
    return nullptr;
}



void* sendFile(void *arg){
    int clientSocket = *reinterpret_cast<int*>(arg);

    while(true){
        
        // Recevoir la demande et les informations du fichier du client
        char buffer[1024]; // Définir une taille de tampon appropriée
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived == -1) {
            writeLog("Erreur lors de la réception des données.");
        } else {
            
            // Extraire les données reçues dans des variables
            string receivedData(buffer, bytesReceived); // Convertir le tampon en une chaîne
            size_t firstNewline = receivedData.find('\n');
            std::string request = receivedData.substr(0, firstNewline);
            if (request.find("SEND_FILE") != std::string::npos) {
                size_t secondNewline = receivedData.find('\n', firstNewline + 1);
                size_t thirdNewline = receivedData.find('\n', secondNewline + 1);

                // Extraire chaque partie de la chaîne receivedData en fonction de la position des caractères de nouvelle ligne
                
                std::string fileInfo = receivedData.substr(firstNewline + 1, secondNewline - firstNewline - 1);
                std::string ipToSend = receivedData.substr(secondNewline + 1, thirdNewline - secondNewline - 1);
                
     
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


                std::cout << "Nom du fichier : " << filename << std::endl;
                std::cout << "Taille du fichier : " << fileSize << " octets" << std::endl;




                pthread_t clientThread;
                int port = 1250;
                // Créer une instance de ThreadArgs pour stocker les valeurs
                DownloadArgs* shareArgs = new DownloadArgs;
                shareArgs->clientSocket = clientSocket;
                shareArgs->fileSize = fileSize;
                shareArgs->port = port;
                shareArgs->ipAddress = ipToSend;
                shareArgs->filename = filename;



                sendFileToClient(shareArgs);


            }
        }
    }

    close(clientSocket);
    return nullptr;

}

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
        writeLog("INFO: Data received from server: " + string(buffer));

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

        fileList_mutex.lock();
        while (getline(ss, line)) {
            if (lineNumber != 0 && line != "END_OF_FILE") {
                ListFile file;
                file.file_info = line;
                file.file_number = lineNumber;
                fileList.push_back(file);

                cout << file.file_number << ". " << file.file_info << endl;
            }
            lineNumber++;
        }
        fileList_mutex.unlock();
    } else {
        writeLog("No valid file list found in the response");
    }
}


void* startReceive(void* arg) {
    DownloadArgs* downloadArgs = static_cast<DownloadArgs*>(arg);
    //int mySocket = downloadArgs->clientSocket;
    std::string myIP = downloadArgs->ipAddress;
    std::string filename = downloadArgs->filename;
    int port = downloadArgs->port;

    int mySocket = socket(AF_INET, SOCK_STREAM, 0);
    if (mySocket == -1) {
        writeLog("ERROR: Échec de la création du socket.");
        return nullptr;
    }
    else{
        writeLog("INFO: the socket that receive file created successful");
        cout<< "socket crée avec succés"<<endl;
    }

    sockaddr_in myAddress;

    // Conversion de l'adresse IP en représentation binaire
    if (inet_pton(AF_INET, myIP.c_str(), &myAddress.sin_addr.s_addr) <= 0) {
        writeLog("Erreur lors de la conversion de l'adresse IP.");
        return nullptr;
    }

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
    std::cout << "Socket liée à l'adresse du serveur." << std::endl;

    // Mettre la socket en mode écoute
    if (listen(mySocket, 1) == -1) {
        std::cerr << "Erreur lors de la mise en écoute de la socket." << std::endl;
        close(mySocket);
        return nullptr;
    }
    std::cout << "Socket en mode écoute." << std::endl;

    while (true) {
        // Accepter une connexion entrante
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(mySocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressLength);
        if (clientSocket == -1) {
            std::cerr << "Erreur lors de l'acceptation de la connexion." << std::endl;
            continue;
        }
        std::cout << "Connexion de l'envoyeur." << std::endl;
        writeLog("INFO: le client disposant du fichier que j'ai demandé vient de se connecter");

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
        if (command.substr(0, 4) != "STOR") {
            std::cerr << "Commande STOR attendue." << std::endl;
            close(clientSocket);
            continue;
        }

        // Extraire le nom du fichier à partir de la commande STOR
        std::string receivedFilename = command.substr(5);
        std::cout << "Réception du fichier : " << receivedFilename << std::endl;
        writeLog("INFO: reception du fichier'"+ receivedFilename +"';");
        // Ouvrir un fichier en écriture pour recevoir les données
            std::string fullPath = "download/" + filename;

       

        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Erreur lors de l'ouverture du fichier." << std::endl;
            close(clientSocket);
            continue;
        }
        std::cout << "Fichier ouvert en écriture." << std::endl;

        // Recevoir les données du fichier et les écrire dans le fichier
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
            if (bytesRead == -1) {
                std::cerr << "Erreur lors de la réception des données du fichier." << std::endl;
                file.close();
                close(clientSocket);
                break;
            }
            if (bytesRead == 0) {
                std::cout <<"Réception des données du fichier terminée." << std::endl;
                writeLog("INFO: Réception des données du fichier terminée.");
                break;
            }
            file.write(buffer, bytesRead);
        }

        // Fermer le fichier
        file.close();

        // Fermer la socket du client
        close(clientSocket);
        break;
    }

    // Fermer la socket principale
    close(mySocket);
    std::cout << "Socket principale fermée." << std::endl;
    writeLog("INFO: Socket principale fermée.");

    return nullptr;
}

void* download_file(void* arg) {
    ThreadParams* params = static_cast<ThreadParams*>(arg);

    // Accéder aux paramètres du thread
    int clientSocket = params->clientSocket;
    string fileInfo = params->file_info;


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



    const char* request = "DOWNLOAD_FILE";
    int requestSize = strlen(request) + 1;
    const char* fileInfoData = fileInfo.c_str();
    int fileInfoSize = fileInfo.size() + 1;


    // Concaténer la demande et les informations du fichier
    std::string receivedData(request);
    receivedData += "\n";  // Ajouter une nouvelle ligne pour séparer la demande et les informations
    receivedData += fileInfo;


    // Envoyer la demande et les informations du fichier au serveur
    if (send(clientSocket, receivedData.c_str(), receivedData.size(), 0) == -1) {
        perror("Erreur lors de l'envoi de la demande et des informations du fichier");
    }

    const int bufferSize = 1024;
    char buffer[bufferSize];
    std::string ipAddress;

    // Recevoir les données du serveur
    ssize_t bytesRead = recv(clientSocket, buffer, bufferSize, 0);
    if (bytesRead == -1) {
        std::cout << "Erreur lors de la réception des données du serveur." << std::endl;
    } else {
        std::string receivedMessage(buffer, bytesRead);
        if (receivedMessage.find("POSITIVE:") == 0) {
            // La chaîne commence par "POSITIVE:"
            // Extraire l'adresse IP
            ipAddress = receivedMessage.substr(9);
            std::cout << "Le telechargement va commencer dans un instant" << std::endl;
        }else {
            std::cout << "Le fichier demandé n'est plus disponible, veillez recharger la liste et faites une nouvelle demande de telechargement" << std::endl;
            return nullptr;
        }
    }
    delete params; // libérer la mémoire allouée pour la structure ThreadParams

    int port = 1250;
    // Créer une instance de ThreadArgs pour stocker les valeurs
    DownloadArgs* downloadArgs = new DownloadArgs;
    downloadArgs->clientSocket = clientSocket;
    downloadArgs->fileSize = fileSize;
    downloadArgs->port = port;
    downloadArgs->ipAddress = ipAddress;
    downloadArgs->filename = filename;



   startReceive(downloadArgs);


    return nullptr;
}
