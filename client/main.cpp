#include "client.h"

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

//vecteur pour contenir les info sur tous les fichiers disponible chez lz serveur
vector<ListFile> fileList;

//fonction pour gerer les logs
void displayLogs() {
    ifstream file("log.txt"); // Open the log file

    if (file.is_open()) {
        string line;

        while (getline(file, line)) {
            cout << line << endl; // Print each line to cout
        }

        file.close(); // Close the file
    } else {
        cout << "Unable to open log file." << endl;
    }
}

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


// function to serialize
string serialize(const vector<File>& files) {
    string serializedData;

    // browse each element of the vector
    for (const File& file : files) {
        // add the name of the file and the size
        serializedData += file.file_name + "," + to_string(file.file_size) + ";";
    }

    return serializedData;
}


vector<File> getFileInfo(const string& dataPath) {
    // Liste des fichiers dans le dossier "data"
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

    return files; // Retourner le vecteur de structures File
}





void sendData(int socketFd, const vector<File>& files) {
    // Sérialiser le vecteur
    string serializedFiles = serialize(files);

    // Envoyer les données sur le socket
    const char* data = serializedFiles.c_str();
    int dataSize = strlen(data);

    if (send(socketFd, data, dataSize, 0) == -1) {
        perror("error when sending the data into the socket");
        writeLog("ERROR: error when sending the data into the socket");
    } else {
        cout << "Data successful sending into the socket" << endl;
        writeLog("INFO: Data successful sending into the socket");
  
    }
}

void* download_file(void* arg) {
    ThreadParams* params = static_cast<ThreadParams*>(arg);

    // Accéder aux paramètres du thread
    int clientSocket = params->clientSocket;
    string file_info = params->file_info;


    const char* request = "DOWNLOAD_FILE";
    int requestSize = strlen(request) + 1;
    const char* fileInfoData = file_info.c_str();
    int fileInfoSize = file_info.size() + 1;
    string username_ftp;
    string password_ftp;
    cout<<"entrez le nom de votre utilisateur ftp"<<endl;
    cin>>username_ftp;
    cout<<"entrez le mot de passe de votre utilisateur ftp"<<endl;
    cin>>password_ftp;


    

    // Concaténer la demande et les informations du fichier
    std::string receivedData(request);
    receivedData += "\n";  // Ajouter une nouvelle ligne pour séparer la demande et les informations
    receivedData += file_info;
    receivedData += "\n"; 
    receivedData += username_ftp;
    receivedData += "\n"; 
    receivedData += password_ftp;


    // Envoyer la demande et les informations du fichier au serveur
    if (send(clientSocket, receivedData.c_str(), receivedData.size(), 0) == -1) {
        perror("Erreur lors de l'envoi de la demande et des informations du fichier");
    }

    const int bufferSize = 1024;
    char buffer[bufferSize];


    // Recevoir les données du serveur
    ssize_t bytesRead = recv(clientSocket, buffer, bufferSize, 0);
    if (bytesRead == -1) {
        std::cout << "Erreur lors de la réception des données du serveur." << std::endl;
    } else {
        std::string receivedMessage(buffer, bytesRead);
        if (receivedMessage == "POSITIVE") {
            std::cout << "Le telechargement va commencer dans un instant" << std::endl;
        } else {
            std::cout << "Le fichier demandé n'est plus disponible, veillez recharger la liste et faites une nouvelle demande de telechargement" << std::endl;
        }
    }
    delete params; // N'oubliez pas de libérer la mémoire allouée pour la structure ThreadParams

    return nullptr;
}


//fonction pour envoyer un fichier a un client
// Fonction pour avoir la liste des fichiers pouvant etre telechargé
void* shareFile(void* arg) {
   
    int socketFd = *reinterpret_cast<int*>(arg);
    while(true){

        // Recevoir la demande et les informations du fichier du client
        char buffer[1024]; // Définir une taille de tampon appropriée
        int bytesReceived = recv(socketFd, buffer, sizeof(buffer), 0);
        if (bytesReceived == -1) {
            cerr << "Erreur lors de la réception des données." << endl;
            close(socketFd);
            return nullptr;
        }
         cout<< "je suis charge de transmettre"<<endl;
        // Extraire les données reçues dans des variables
        string receivedData(buffer, bytesReceived); // Convertir le tampon en une chaîne
        size_t firstNewline = receivedData.find('\n');
        size_t secondNewline = receivedData.find('\n', firstNewline + 1);
        size_t thirdNewline = receivedData.find('\n', secondNewline + 1);

        // Extraire chaque partie de la chaîne receivedData en fonction de la position des caractères de nouvelle ligne
        std::string request = receivedData.substr(0, firstNewline);
        std::string file_to_download = receivedData.substr(firstNewline + 1, secondNewline - firstNewline - 1);
        std::string ipToSend = receivedData.substr(secondNewline + 1, thirdNewline - secondNewline - 1);
        std::string url_ftp = receivedData.substr(thirdNewline + 1);


        CURL *curl;
        CURLcode res;
        std::string local_file = file_to_download;
        std::string remote_file = ".";

                // Trouver la position du premier double deux-points (:) dans l'URL FTP
        size_t colonPos = url_ftp.find(':');

        // Extraire le nom d'utilisateur de l'URL FTP
        std::string username = url_ftp.substr(url_ftp.find("//") + 2, colonPos - (url_ftp.find("//") + 2));

        // Trouver la position du premier arobase (@) dans l'URL FTP
        size_t atPos = url_ftp.find('@');

        // Extraire le mot de passe de l'URL FTP
        std::string password = url_ftp.substr(colonPos + 1, atPos - colonPos - 1);


        url_ftp += remote_file + "/" + local_file;

        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url_ftp.c_str());
            curl_easy_setopt(curl, CURLOPT_USERNAME, username);
            curl_easy_setopt(curl, CURLOPT_PASSWORD, password);
            curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

            FILE *file = fopen(local_file.c_str(), "rb");
            if(file) {
                struct stat file_stat;
                stat(local_file.c_str(), &file_stat);
                curl_easy_setopt(curl, CURLOPT_INFILESIZE, file_stat.st_size);

                curl_easy_setopt(curl, CURLOPT_READDATA, file);
                curl_easy_setopt(curl, CURLOPT_READFUNCTION, fread);

                res = curl_easy_perform(curl);
                if(res!= CURLE_OK) {
                    std::cerr << "Erreur lors de l'envoi du fichier : " << curl_easy_strerror(res) << std::endl;
                } else {
                    std::cout << "Fichier envoyé avec succès!" << std::endl;
                }

                fclose(file);
            } else {
                std::cerr << "Erreur lors de l'ouverture du fichier local." << std::endl;
            }

            curl_easy_cleanup(curl);
        }
        curl_global_cleanup();
        
    }
    close(socketFd);

    return nullptr;
}


// Fonction pour avoir la liste des fichiers pouvant etre telechargé
void* getlistfile(void* arg) {
    int socketFd = *reinterpret_cast<int*>(arg);

    const char* request = "GET_FILE_LIST";
    int requestSize = strlen(request) + 1;
    ssize_t bytesReceived;
    const int bufferSize = 4096;
    char buffer[bufferSize];

    // Envoyer la demande au serveur
    if (send(socketFd, request, requestSize, 0) == -1) {
        perror("Error when asking for the list of files");
    }

    std::string receivedData;

    while ((bytesReceived = recv(socketFd, buffer, bufferSize - 1, 0)) > 0) {
        buffer[bytesReceived] = '\0';

        stringstream ss(buffer);
        string ligne;
        int numeroLigne = 1;

        while (getline(ss, ligne)) {
            ListFile file;
            file.file_info = ligne;
            file.file_number = numeroLigne;
            fileList.push_back(file);

            cout << file.file_number << ". " << file.file_info << endl;
            numeroLigne++;
        }

        receivedData += buffer;
    }

    close(socketFd);

    return nullptr;
}

int main() {
    // Initialisation de la connexion au serveur
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket != -1) {
        writeLog("INFO: successfully created socket");
    } else {
        writeLog("ERROR: error when creating socket");
        perror("error when creating socket");
        return -1;
    }

    // Spécifier l'adresse du serveur et se connecter à celui-ci
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = inet_addr("192.168.98.97");

    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("error when connect to a server");
        writeLog("ERROR: error when connect to a server");
        close(clientSocket); // Fermer le descripteur de fichier du socket
        return -1;
    } else {
        writeLog("INFO: success connect to a server");
    }

    vector<File> files;

    while (true) {
        int option;
        int file_number;
        bool file_found = false;
        string file_info;

        string directoryPath = "data";

        cout << "Menu:" << endl;
        cout << "1. declarer mes fichiers" << endl;
        cout << "2. voir les fichiers disponible" << endl;
        cout << "3. Afficher mes logs" << endl;
        cout << "4. Telecharger un fichier" << endl;
        cout << "5. Quitter" << endl;
        cout << endl;
        cout << "Choisissez une option : ";
        writeLog("INFO: le client doit choisir une option:");
        cin >> option;
        ThreadParams* params = new ThreadParams;

        switch (option) {
            case 1:
                writeLog("option choisie: 1-declarer mes fichiers");
                // Afficher les fichiers au niveau du serveur
                // Envoyer les données initiales au socket
                files = getFileInfo(directoryPath);
                sendData(clientSocket, files);
                cout << endl;

                pthread_t share;
                if (pthread_create(&share, nullptr, shareFile, &clientSocket) != 0) {
                    cerr << "Erreur lors de la création du thread." << endl;
                    // Éventuellement, enregistrer l'erreur dans un fichier de log
                }

              
                break;
            case 2:
                // Envoyer les données sur le socket
                writeLog("option choisie: 2-voir les fichiers disponible");
                // Créer un thread pour gérer le client
                pthread_t thread;
                if (pthread_create(&thread, nullptr, getlistfile, &clientSocket) != 0) {
                    cerr << "Erreur lors de la création du thread." << endl;
                    // Éventuellement, enregistrer l'erreur dans un fichier de log
                }
                sleep(1);
                break;
            case 3:
                writeLog("option choisie: 3-Afficher mes logs");
                displayLogs();
                break;
            case 4:
                writeLog("option choisie: 4-Telecharger un fichier");
                cout << "Entrez le numéro du fichier à télécharger : ";
                cin >> file_number;

                // Parcourir la liste des fichiers pour trouver celui correspondant au numéro
                for (const ListFile& file : fileList) {
                    if (file.file_number == file_number) {
                        file_found = true;
                        file_info = file.file_info;
                        break;
                    }
                }

                // Vérifier si le fichier a été trouvé et afficher les informations
                if (file_found) {
                    cout << "Informations du fichier " << file_number << " : " << file_info << endl;
                } else {
                    cout << "Fichier non trouvé! veillez recharger la liste des fichiers, bien vérifier le numéro et réessayer" << endl;
                    cout<<""<<endl;
                    break;
                }

                cout << endl;

                
                params->clientSocket = clientSocket;
                params->file_info = file_info;

                // Créer le thread avec les paramètres
                pthread_t download;
                if (pthread_create(&download, nullptr, download_file, params) != 0) {
                    cerr << "Erreur lors de la création du thread." << endl;
                    // Éventuellement, enregistrer l'erreur dans un fichier de log
                }
                sleep(10);
               
                break;
            case 5:
                writeLog("option choisie: 5-quitter!");
                writeLog("fin d'éxécution; à bientôt!");
                close(clientSocket);
                return 0;
                break;
            default:
                writeLog("option choisie: option non disponible");
                cout << "Option invalide. Veuillez choisir une option valide." << endl;
                cout << endl;
                break;
        }
    }

    // Nettoyage
    close(clientSocket);

    return 0;
}
