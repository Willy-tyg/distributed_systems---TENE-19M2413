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

    while (true) {
        bytesRead = read(inotifyFd, buffer, sizeof(buffer));
        if (bytesRead == -1) {
            cerr << "Error reading from inotify" << endl;
            break;
        }

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
            // Nom du fichier
            string fileName = iter->path().filename().string();

            // Taille du fichier
            int fileSize = fs::file_size(*iter);
            
            // Écriture dans la structure File
            File f;
            f.file_name = fileName;
            f.file_size = fileSize;

            files.push_back(f);
            // Passage au fichier suivant
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
    vector<File> files = getFileInfo("data");

    // send the data into the socket

    sendData(clientSocket, files);

    
    while(true){
        cout<<"salut"<<endl;
        int changeDetected = watchDirectory(directoryPath);
    

        if (changeDetected==0) {
            cerr << "change detect into the directory"<< directoryPath << endl;
            writeLog("WARNING: Changes detected in the directory '" + directoryPath + "': The data will be sent again.");

            vector<File> files = getFileInfo("data");
            // Envoyer les données sur le socket
            sendData(clientSocket, files);
        } 
    }
    

    // close of socket
    close(clientSocket);

    return 0;
}