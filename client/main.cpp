#include "client.h"

struct File{
    int file_size;
    string file_name;
};


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



//function to detect the change into the directory 'data'
int watchDirectory(const string& directoryPath) {
    int inotifyFd = inotify_init();
    if (inotifyFd == -1) {
        cerr << "Error initializing inotify" << endl;
        writeLog("ERROR: Error initializing inotify");
        return -1;
    }

    int watchDescriptor = inotify_add_watch(inotifyFd, directoryPath.c_str(), IN_ALL_EVENTS);
    if (watchDescriptor == -1) {
        cerr << "Error adding watch to directory: " << directoryPath << endl;
        writeLog("ERROR: Error adding watch to directory: '" +directoryPath+"'.");
        close(inotifyFd);
        return -1;
    }

    char buffer[4096];
    ssize_t bytesRead;
    bytesRead = read(inotifyFd, buffer, sizeof(buffer));
    if (bytesRead == -1) {
        cerr << "Error reading from inotify" << endl;
        return -1;
    }

    if (fs::is_directory(directoryPath)) {
        for (char* ptr = buffer; ptr < buffer + bytesRead;) {
            struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
        
            if (event->mask & IN_CREATE){
                writeLog("File created into '" +directoryPath+"'.");
                return 0;
            }
            if (event->mask & IN_DELETE){
                writeLog("File delete into'" +directoryPath+"'.");
                return 0;
            }
            if (event->mask & IN_MODIFY){
                writeLog("File modified into '" +directoryPath+"'.");
                return 0;
            }
            if (event->mask & IN_MOVED_FROM){
                writeLog("File moved from '" +directoryPath+"'.");
                return 0;
            }
            if (event->mask & IN_MOVED_TO){
                writeLog("File moved to '" +directoryPath+"'.");
                return 0;
            }

            ptr += sizeof(struct inotify_event) + event->len;
        }
        
    }
    else {
        // Si le dossier n'existe pas, écrire un message d'erreur dans le journal
        // ou gérer l'erreur d'une autre manière appropriée.
        writeLog("FATAL: can't detect change into the directory: '"+ directoryPath +"' directory does not exist");
        return -1;
    }

    // Clean up
    close(watchDescriptor);
    close(inotifyFd);

    return -1;
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


int main(){
 
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(clientSocket != -1){
        writeLog("INFO: successfully created socket");
    }
    else{
        writeLog("ERROR: error when creating socket");
        perror("error when creating socket");
    }

    // specify the server address
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(10000);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    // connect to a server
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("error when connect to a server");
        writeLog("ERROR: error when connect to a server");
        close(clientSocket);
        return 1;
    }else{
        writeLog("INFO: success connect to a server");
    }

    string directoryPath = "data";  // directory of file
    vector<File> files = getFileInfo(directoryPath);

    // send the data into the socket

    sendData(clientSocket, files);

    
    while(true){

        int inotifyFd = inotify_init();
        if (inotifyFd == -1) {
            cerr << "Error initializing inotify" << endl;
            writeLog("ERROR: Error initializing inotify");
            return -1;
        }

        int watchDescriptor = inotify_add_watch(inotifyFd, directoryPath.c_str(), IN_ALL_EVENTS);
        if (watchDescriptor == -1) {
            cerr << "Error adding watch to directory: " << directoryPath << endl;
            writeLog("ERROR: Error adding watch to directory: '" +directoryPath+"'.");
            close(inotifyFd);
            return -1;
        }

        char buffer[4096];
        ssize_t bytesRead;
        bytesRead = read(inotifyFd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            cerr << "Error reading from inotify" << endl;
            return -1;
        }

        if (fs::is_directory(directoryPath)) {
            for (char* ptr = buffer; ptr < buffer + bytesRead;) {
                struct inotify_event* event = reinterpret_cast<struct inotify_event*>(ptr);
            
                if (event->mask & IN_CREATE){
                    writeLog("File created into '" +directoryPath+"'.");
                    cerr << "change detect into the directory"<< directoryPath << endl;
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesc = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesc);
                }
                if (event->mask & IN_DELETE){
                    writeLog("File delete into'" +directoryPath+"'.");
                    cerr << "change detect into the directory"<< directoryPath << endl;
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesd = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesd);
                }
                if (event->mask & IN_MODIFY){
                    writeLog("File modified into '" +directoryPath+"'.");
                    cerr << "change detect into the directory"<< directoryPath << endl;
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesm = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesm);
                }
                if (event->mask & IN_MOVED_FROM){
                    writeLog("File moved from '" +directoryPath+"'.");
                    cerr << "change detect into the directory"<< directoryPath << endl;
                    writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

                    vector<File> filesf = getFileInfo(directoryPath);
                    // Envoyer les données sur le socket
                    sendData(clientSocket, filesf);
                }
                if (event->mask & IN_MOVED_TO){
                    writeLog("File moved to '" +directoryPath+"'.");
                    cerr << "change detect into the directory"<< directoryPath << endl;
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
            return -1;
        }

        // Clean up
        close(watchDescriptor);
        close(inotifyFd);

    }

}