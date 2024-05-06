#include "serveur.h"

//mutex pour verouiller le vecteur clientData
std::mutex client_mutex;

//vecteur pour stocker les adresses ip et socket des differents clients
std::vector<ClientData> clients;

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

void supprimerBlocIP(const string& cheminFichier, const string& adresseIP) {
    ifstream fichier(cheminFichier);
    if (!fichier.is_open()) {
        cerr << "Error opening the file." << endl;
        return;
    }

    ofstream fichierTemp("temp.txt"); // Temporary file to store the remaining lines
    if (!fichierTemp.is_open()) {
        cerr << "Error creating the temporary file." << endl;
        return;
    }

    string ligne;
    bool blocTrouve = false;
    bool nouvelleAdresseIP = false;
    // Read the lines of the file
    while (getline(fichier, ligne)) {
        if (blocTrouve) {
            // Write the remaining lines to the temporary file
            if (!nouvelleAdresseIP) {
                if (ligne.find(':') != string::npos) {
                    nouvelleAdresseIP = true;
                    fichierTemp << ligne << endl;
                }
            } else {
                fichierTemp << ligne << endl;
            }
        } else {
            // Check if the line contains the specified IP address followed by a colon
            size_t pos = ligne.find(adresseIP + " :");
            if (pos != string::npos) {
                // If yes, mark the beginning of the block to be deleted
                blocTrouve = true;
            } else {
                // If not, write the line to the temporary file
                fichierTemp << ligne << endl;
            }
        }
    }

    fichier.close();
    fichierTemp.close();

    // Replace the original file with the temporary file
    remove(cheminFichier.c_str());
    rename("temp.txt", cheminFichier.c_str());
}

vector<File> deserialize(const string& serializedData) {
    vector<File> files;

    string dataCopy = serializedData; // Copy of the serialized data

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


int getClientSocketByIP(const std::string& clientIP) {
    client_mutex.lock();

    int clientSocket = -1;  // Valeur par défaut si l'adresse IP n'est pas trouvée

    // Parcourez la structure de données pour rechercher l'IP et récupérer le socket correspondant

    for (const auto& client : clients) {
        if (client.ip == clientIP) {
            clientSocket = client.socket;
            break;
        }

    }
    return clientSocket;
}





void* DownloadFile(void* arg){
    DownloadArgs* downloadArgs = static_cast<DownloadArgs*>(arg);
    int clientSocket = downloadArgs->clientSocket;
    string clientIP = downloadArgs->clientIP;
    string file_to_download= downloadArgs->file_to_download;
    string url_ftp = downloadArgs->url_ftp;

    ifstream infoFile("infos_client.txt");
    if (!infoFile) {
        cerr << "Erreur: Impossible d'ouvrir le fichier infos_client.txt." << endl;
        return nullptr;
    }

    string line;
    string ipAddress;

    // Commence à lire le fichier depuis le début
    infoFile.clear();
    infoFile.seekg(0);

    bool found = false;
    // Lire chaque ligne du fichier
    while (getline(infoFile, line)) {
        // Vérifier si la ligne contient une adresse IP
        if (line.find(":") != string::npos && line.find(".") != string::npos) {
            // Stocker l'adresse IP trouvée
            ipAddress = line;
        }
        // Vérifier si la ligne correspond à la chaîne de recherche
        if (line == file_to_download) {
            // Si la chaîne de recherche est trouvée, extraire l'adresse IP de la ligne
            size_t spacePos = ipAddress.find('string url_ftp = downloadArgs->url_ftp; ');
            ipAddress = ipAddress.substr(0, spacePos);
            found = true;
            break; // Sortir de la boucle une fois que la chaîne est trouvée
        }
    }

    if (!found) {
        ipAddress = ""; // Assigner une valeur vide à ipAddress si la chaîne de recherche n'est pas trouvée
    }

    std::string message; // Message à envoyer au client

    if (!ipAddress.empty()) {
        message = "POSITIVE"; // Message positif si une adresse IP est trouvée
        std::cout << "adresse IP de l'hôte qui détient le fichier '" << file_to_download << "' : " << ipAddress << std::endl;
    } else {
        message = "NEGATIVE"; // Message négatif si aucune adresse IP n'est trouvée
        std::cout << "Aucune adresse IP trouvée pour le fichier '" << file_to_download << "'." << std::endl;
        return nullptr;
    }

    // Envoi du message au client pour lui dire si le fichier est disponible ou pas
    if (send(clientSocket, message.c_str(), message.size(), 0) == -1) {
        std::cerr << "Erreur lors de l'envoi du message au client." << std::endl;
    }

   
    int targetSocket = getClientSocketByIP(clientIP);
    cout<<"socket du client qui detient le fichier"<< file_to_download << ":" <<targetSocket<<endl;
    //  Envoyer un message au client detenteur du fichier pur qu'il le partage avec le demandeur
    const char* request = "SEND_FILE";
    int requestSize = strlen(request) + 1;
    const char* file_to_send = file_to_download.c_str();
    int fileInfoSize = file_to_download.size() + 1;
    std::string ipToSend = clientIP;
    int ipToSendSize =ipAddress.size()+1;
    int url_ftp_size = url_ftp.size()+1; // Obtient la taille de la chaîne de caractères url_ftp




    // Concaténer la demande et les informations du fichier
    std::string requestData(request);
    requestData += "\n";  // Ajouter une nouvelle ligne pour séparer la demande et les informations
    requestData += file_to_download;
    requestData += "\n";  // Ajouter une nouvelle ligne 
    requestData += ipToSend;
    requestData += "\n";  // Ajouter une nouvelle ligne 
    requestData += url_ftp;

    // Envoyer la demande et les informations du fichier au serveur
    if (send(targetSocket, requestData.c_str(), requestData.size(), 0) == -1) {
        perror("Erreur lors de l'envoi de la demande et des informations du fichier");
    }

    return nullptr;
}



void* sendFileList(void* arg) {
    ThreadArgs* threadArgs = static_cast<ThreadArgs*>(arg);
    int clientSocket = threadArgs->clientSocket;
    string clientIP = threadArgs->clientIP;

    vector<File> files;
    ifstream fichier("infos_client.txt"); // Ouvrir le fichier

    if (!fichier.is_open()) {
        cerr << "Error opening the file." << endl;
        return nullptr;
    } else {
        cout << "File opened successfully." << endl;
    }

    ofstream fichierTemp("temp.txt"); // Fichier temporaire pour stocker les lignes restantes
    if (!fichierTemp.is_open()) {
        cerr << "Error creating the temporary file." << endl;
        fichier.close();
        close(clientSocket);
        pthread_exit(nullptr);
    }

    string ligne;
    bool blocTrouve = false;
    bool nouvelleAdresseIP = false;

    // Lire les lignes du fichier
    while (getline(fichier, ligne)) {
        if (blocTrouve) {
            // Write the remaining lines to the temporary file
            if (!nouvelleAdresseIP) {
                if (ligne.find(':') != string::npos) {
                    nouvelleAdresseIP = true;
                }
            } else {
                if (ligne.find(':') != string::npos) {
                    // Don't copy address
                } else if (!ligne.empty()) {
                    // If not empty, write the line to the temporary file
                    fichierTemp << ligne << endl;
                }
            }
        } else {
            // Check if the line contains the specified IP address followed by a colon
            size_t pos = ligne.find(string(clientIP) + " :");                    
            if (pos != string::npos) {
                // If yes, mark the beginning of the block to be deleted
                blocTrouve = true;
            } else {
                if (ligne.find(':') != string::npos) {
                    // Don't copy address
                } else if (!ligne.empty()) {
                    // If not empty, write the line to the temporary file
                    fichierTemp << ligne << endl;
                }
            }
        }
    }

    fichierTemp.close();
    fichier.close();

    // Ouvrir le fichier temporaire pour lire son contenu
    ifstream tempFile("temp.txt");
    if (!tempFile.is_open()) {
        cerr << "Error opening the temporary file." << endl;
        close(clientSocket);
        pthread_exit(nullptr);
    } else {
        cout << "Temporary file opened successfully." << endl;
    }

    // Lire le contenu du fichier temporaire
    string fileContents((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
    tempFile.close();

    // Envoyer le contenu du fichier au client
    ssize_t bytesSent = send(clientSocket, fileContents.c_str(), fileContents.size(), 0);
    if (bytesSent == -1) {
        cerr << "Error sending file contents to the client." << endl;
        close(clientSocket);
        pthread_exit(nullptr);
    } else {
        cout << "File contents sent successfully to the client." << endl;
    }

    // Supprimer le fichier temporaire
    if (remove("temp.txt") != 0) {
        cerr << "Error deleting temporary file." << endl;
    } else {
        cout << "Temporary file deleted successfully." << endl;
    }

    return nullptr;
}


void* handleClient(void* arg) {
    int* argPtr = (int*)arg;
    int clientSocket = *argPtr;
    delete argPtr;

    // Get the client's IP address and port number    
    sockaddr_in clientAddress;
    socklen_t clientAddressLength = sizeof(clientAddress);
    if (getpeername(clientSocket, (struct sockaddr*)&clientAddress, &clientAddressLength) == -1) {
        cerr << "error when obtaining the ip address of client" << endl;
        writeLog("ERROR: error when obtaining the ip address of client");
        close(clientSocket);
        pthread_exit(NULL);
    }
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddress.sin_port); // Convert the port number to an integer
    string filePath = "infos_client.txt";

    client_mutex.lock();

    ClientData client;
    client.ip = clientIP;
    client.socket = clientSocket;

    clients.push_back(client);

    client_mutex.unlock();


    while (true) {
        char buffer[4096];
        ssize_t bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            cerr << "Erreur lors de la réception des données du client " << clientIP << ":" << clientPort << "." << endl;
            close(clientSocket);
            pthread_exit(NULL);
        }

        // Convertir les données reçues en une chaîne
        std::string receivedData(buffer, bytesReceived);

        // Vérifier si le tampon contient plusieurs lignes
        size_t newlinePos = receivedData.find('\n');
        if (newlinePos != std::string::npos) {
            pthread_t thread;
            // Le tampon contient plusieurs lignes
            // Extraire la demande et les informations du fichier

            // Trouver la position du premier saut de ligne
            size_t newlinePos = receivedData.find('\n');

            // Extraire la demande du serveur
            std::string request = receivedData.substr(0, newlinePos);

            // Extraire le reste de la réponse (les informations du fichier, le nom d'utilisateur et le mot de passe)
            std::string remainingData = receivedData.substr(newlinePos + 1);

            // Trouver la position du deuxième saut de ligne (séparateur entre les informations du fichier et le nom d'utilisateur)
            newlinePos = remainingData.find('\n');

            // Extraire les informations du fichier
            std::string fileInfo = remainingData.substr(0, newlinePos);

            // Extraire le reste de la réponse (le nom d'utilisateur et le mot de passe)
            std::string remainingData2 = remainingData.substr(newlinePos + 1);

            // Trouver la position du troisième saut de ligne (séparateur entre le nom d'utilisateur et le mot de passe)
            newlinePos = remainingData2.find('\n');

            // Extraire le nom d'utilisateur
            std::string username_ftp = remainingData2.substr(0, newlinePos);

            // Extraire le mot de passe
            std::string password_ftp = remainingData2.substr(newlinePos + 1);
            
            int port = 21; // Port FTP

            // Formater l'URL FTP
            std::string url_ftp = "ftp://" + username_ftp + ":" + password_ftp + "@" + clientIP + ":" + std::to_string(port) + "/";

            // Afficher la demande et les informations du fichier
            cout << "Demande : " << request << endl;
            cout << "Informations du fichier : " << fileInfo << endl;

            if (request.find("DOWNLOAD_FILE") != std::string::npos) {
                    // Créer une instance de ThreadArgs pour stocker les valeurs
                    DownloadArgs* downloadArgs = new DownloadArgs;
                    downloadArgs->clientSocket = clientSocket;
                    downloadArgs->bytesReceived = bytesReceived;
                    downloadArgs->clientPort = clientPort;
                    downloadArgs->clientIP = inet_ntoa(clientAddress.sin_addr);
                    downloadArgs->file_to_download = fileInfo;
                    downloadArgs->url_ftp = url_ftp;


                    if (pthread_create(&thread, nullptr, DownloadFile, downloadArgs) != 0) {
                        cerr << "Erreur lors de la création du thread." << endl;
                        writeLog("ERROR: error when creating tdu thread qui se chargera d'initier le telechargement");
                        close(clientSocket);
                        continue;
                    }
                    else{
                        writeLog("INFO: thread successful");
                    }

                }
        } else {

            std::string request(buffer, bytesReceived);
            pthread_t thread;
                    
            if (request.find("GET_FILE_LIST") != std::string::npos) {
                // Créer une instance de ThreadArgs pour stocker les valeurs
                ThreadArgs* threadArgs = new ThreadArgs;
                threadArgs->clientSocket = clientSocket;
                threadArgs->bytesReceived = bytesReceived;
                threadArgs->clientPort = clientPort;
                threadArgs->clientIP = inet_ntoa(clientAddress.sin_addr);

                if (pthread_create(&thread, nullptr, sendFileList, threadArgs) != 0) {
                    cerr << "Erreur lors de la création du thread." << endl;
                    writeLog("ERROR: error when creating the thread");
                    close(clientSocket);
                    continue;
                }
                else{
                    writeLog("INFO: thread successful");
                }

            }else{
                supprimerBlocIP(filePath, clientIP);
                // Receipt the data of client

                // Convert data receives to a string
                string receivedData(buffer, bytesReceived);

                // Deserialized data
                vector<File> receivedFiles = deserialize(receivedData);
                
                // Remove the IP block from the file
                supprimerBlocIP(filePath, clientIP);
                
                static mutex fileMutex;  // Static mutex to ensure exclusive access to the log file
                lock_guard<mutex> lock(fileMutex);  // Lock the mutex to guarantee exclusive access

                // Open the log file in append mode if it's not already open
                // Écrire les données dans un fichier
                ofstream fichier("infos_client.txt", ios::app);
                if (!fichier) {
                    cerr << "Error opening infos client file." << endl;
                    writeLog("ERROR: Error when opening infos client file.");
                }

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
                    
                    cout << "receipt data of client "<< clientIP << endl;
                    writeLog("INFO: receipt data of client .");

                } else {
                    cerr << "Erreur lors de l'ouverture du fichier infos_client.txt" << endl;
                    writeLog("ERROR: Error when opening infos client file.");

                }
                fichier.close();
            

            }
        }
    }

    // Fermer le socket client
    close(clientSocket);
    
    pthread_exit(NULL);
}

