#include "serveur.h"


//mutex pour verouiller le vecteur clientData
std::mutex client_mutex;

//mutex pour verouiller le fichier infos_client.txt
std::mutex info_mutex;


//mutex pour verrouiller les fichiers temporaires
std::mutex temp_mutex;
std::mutex temp2_mutex;

//mutex pour verouiller le vecteur File
std::mutex file_mutex;



std::vector<ClientData> clients;


const size_t INITIAL_BUFFER_SIZE = 4096;

//log function
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

//function to deserialize data
vector<File> deserialize(const string& serializedData) {
    vector<File> files;

    file_mutex.lock();
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
    file_mutex.unlock();
    return files;
}


//function to remove client and their file

void supprimerBlocIP(const string& cheminFichier, const string& adresseIP) {

    info_mutex.lock();
    ifstream fichier(cheminFichier);
    if (!fichier.is_open()) {
        cerr << "Error opening the file." << endl;
                writeLog("ERROR: Error creating the temporary file.");

        return;
    }

    temp_mutex.lock();
    ofstream fichierTemp("temp.txt"); // Temporary file to store the remaining lines
    if (!fichierTemp.is_open()) {
        cerr << "Error creating the temporary file." << endl;
        writeLog("ERROR: Error creating the temporary file.");

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
    temp_mutex.unlock();

    // Replace the original file with the temporary file
    remove(cheminFichier.c_str());
    rename("temp.txt", cheminFichier.c_str());
    info_mutex.unlock();
}


//function to send the list of file to a client when asking

void* sendFileList(void* arg) {
    ThreadArgs* threadArgs = static_cast<ThreadArgs*>(arg);
    int clientSocket = threadArgs->clientSocket;
    string clientIP = threadArgs->clientIP;


    info_mutex.lock();
    ifstream fichier("infos_client.txt"); // Ouvrir le fichier

    if (!fichier.is_open()) {
        cerr << "Error opening the file." << endl;
        return nullptr;
    } else {
        cout << "File opened successfully." << endl;
    }

    temp2_mutex.lock();

    ofstream fichierTemp("temp2.txt"); // Fichier temporaire pour stocker les lignes restantes
    if (!fichierTemp.is_open()) {
        cerr << "Error creating the temporary file." << endl;
        fichier.close();
        close(clientSocket);
    }

    string nouvelleAdresseIP;
    string ligne;

    // Lire les lignes du fichier
    while (getline(fichier, ligne)) {
        
        if (ligne.find(':') != string::npos) {
            size_t spacePos = ligne.find(' ');
            nouvelleAdresseIP = ligne.substr(0, spacePos);
            
        } else if (!ligne.empty()) {
            // If not empty, write the line to the temporary file
            fichierTemp << ligne<<";"<<nouvelleAdresseIP<< endl;
        } 
    }


    fichierTemp.close();
    fichier.close();

    info_mutex.unlock();

    // Ouvrir le fichier temporaire pour lire son contenu
    
    ifstream tempFile("temp2.txt");
    if (!tempFile.is_open()) {
        cerr << "Error opening the temporary file." << endl;
        close(clientSocket);
    } else {
        cout << "Temporary file opened successfully." << endl;
    }

    // Lire le contenu du fichier temporaire
    string fileContents((istreambuf_iterator<char>(tempFile)), istreambuf_iterator<char>());
    tempFile.close();
    temp2_mutex.unlock();

    // Ajouter une première ligne au contenu du fichier
    std::string firstLine = "LIST_FILE";
    fileContents.insert(0, firstLine + "\n");

    // Ajouter une ligne à la fin de fileContents
    std::string newLineToAdd = "END_OF_FILE";
    fileContents += newLineToAdd + "\n";

    // Envoyer le contenu du fichier au client
    ssize_t bytesSent = send(clientSocket, fileContents.c_str(), fileContents.size(), 0);
    if (bytesSent == -1) {
        cerr << "Error sending file list to the client " << endl;
        writeLog("ERROR: Error sending file list to the client '"+ clientIP +"'");
        close(clientSocket);
    } else {
        cout << "File contents sent successfully to the client "<<clientIP << endl;
        writeLog("INFO : File contents sent successfully to the client '"+ clientIP +"'");
    }

    // Supprimer le fichier temporaire
    if (remove("temp2.txt") != 0) {
        writeLog( "ERROR: Error deleting temporary file.");
    } 
    return nullptr;
}




//function to receive client
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
                // Le client s'est déconnecté
                cout<<" "<<endl;
                cerr << "Le client " << clientIP << " s'est déconnecté." << endl;


                client_mutex.lock();
                // Parcourir le vecteur clients
                for (auto it = clients.begin(); it != clients.end(); ++it) {
                    // Vérifier si l'adresse IP correspond
                    if (it->ip == clientIP) {
                        // Supprimer l'entrée du vecteur
                        clients.erase(it);
                        break; // Sortir de la boucle après avoir supprimé l'entrée
                    }
                }
                client_mutex.unlock();

                string filePath = "infos_client.txt";

                    // Remove the IP block from the file
                supprimerBlocIP(filePath, clientIP);

                close(clientSocket);

                pthread_exit(NULL);  
        }else{

        

            // Convertir les données reçues en une chaîne
            std::string receivedData(buffer, bytesReceived);

            // Vérifier si le tampon contient plusieurs lignes
            size_t newlinePos = receivedData.find('\n');
            if (newlinePos != std::string::npos) {
                
                // Le tampon contient plusieurs lignes
                // Extraire la demande et les informations du fichier

                // Trouver la position du premier saut de ligne
                size_t newlinePos = receivedData.find('\n');

                // Extraire la demande du client
                std::string request = receivedData.substr(0, newlinePos);
                cout<<" "<<endl;
                cout<<"request : "<<request<<endl;
                if (request.find("DECLARE_FILE") != std::string::npos) {
                    // Deserialized data
                    // Extraire le reste de la chaîne
                    std::string clientFiles = receivedData.substr(newlinePos + 1);
                    vector<File> receivedFiles = deserialize(clientFiles);
                    
                    // Remove the IP block from the file
                    supprimerBlocIP(filePath, clientIP);
                    info_mutex.lock();
                    ofstream fichier("infos_client.txt", ios::app);
                    if (!fichier) {
                        cerr << "Error opening infos client file." << endl;
                        writeLog("ERROR: Error when opening infos client file.");
                    }

                    
                    
                    // Open the log file in append mode if it's not already open
                    // Écrire les données dans un fichier

                    if (fichier.is_open()) {
                        // Écrire les informations de l'adresse IP et du port du client
                        fichier << clientIP << " : " << clientPort << endl;

                        // Écrire les informations des fichiers dans le fichier
                        for (const auto& file : receivedFiles) {
                            fichier << file.name << " " << file.size << " octets" << endl;
                        }

                        // Ajouter un retour à la ligne après chaque ensemble d'informations
                        fichier << endl;
                        info_mutex.unlock();

                        // Fermer le fichier
                        fichier.close();
                        
                        cout << "receipt data of client "<< clientIP << endl;
                        writeLog("INFO: receipt data of client .");

                    } else {
                        cerr << "Erreur lors de l'ouverture du fichier infos_client.txt" << endl;
                        writeLog("ERROR: Error when opening infos client file.");

                    }
                    

                }else{

                }


            }else{

                // demande du client
                std::string request = receivedData;
                cout<<" "<<endl;
                cout<<"request : "<<request<<endl;
                if (request.find("GET_FILE_LIST") != std::string::npos) {

                    // Créer une instance de ThreadArgs pour stocker les valeurs
                    ThreadArgs* threadArgs = new ThreadArgs;
                    threadArgs->clientSocket = clientSocket;
                    threadArgs->clientIP = inet_ntoa(clientAddress.sin_addr);

                    sendFileList(threadArgs);
                }
            }
        }
    }
    close(clientSocket);
    pthread_exit(NULL);

}

